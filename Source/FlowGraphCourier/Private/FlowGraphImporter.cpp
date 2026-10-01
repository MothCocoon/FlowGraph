// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphImporter.h"
#include "FlowLogChannels.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/FlowNodeBlueprint.h"
#include "Nodes/FlowNodeAddOnBlueprint.h"
#include "Engine/Blueprint.h"
#include "Types/FlowNamedDataPinProperty.h"
#include "Types/FlowDataPinValue.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include "UObject/TextProperty.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Graph/FlowGraph.h"

UFlowAsset* UFlowGraphImporter::ImportFlowGraphFromDocument(
	const FString& TargetAssetPath,
	const FString& AssetClassPath,
	bool bWorldBound,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const TArray<FFlowGraphParsedConnection>& ParsedConnections,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	FString PackageName = FPackageName::ObjectPathToPackageName(TargetAssetPath);
	// GetShortName only splits on '/', so it must run on the already-dot-stripped PackageName -
	// running it directly on TargetAssetPath leaves a literal ".ObjectName" suffix on the FName
	// when a caller passes a fully-qualified dotted object path (e.g. FTopLevelAssetPath::ToString()).
	FString AssetName = FPackageName::GetShortName(PackageName);

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		OutErrorMessage = FString::Printf(TEXT("Failed to create package: %s"), *PackageName);
		return nullptr;
	}

	// Use the document's concrete asset class. An explicit class that cannot be loaded must fail
	// rather than creating a base FlowAsset with different graph rules.
	UClass* ResolvedAssetClass = UFlowAsset::StaticClass();
	if (!AssetClassPath.IsEmpty() && !AssetClassPath.Equals(TEXT("None"), ESearchCase::IgnoreCase))
	{
		UClass* LoadedAssetClass = LoadObject<UClass>(nullptr, *AssetClassPath);
		if (LoadedAssetClass && LoadedAssetClass->IsChildOf(UFlowAsset::StaticClass()))
		{
			ResolvedAssetClass = LoadedAssetClass;
		}
		else
		{
			OutErrorMessage = FString::Printf(TEXT("AssetClass is not a resolvable FlowAsset class: %s"), *AssetClassPath);
			return nullptr;
		}
	}

	UFlowAsset* FlowAsset = NewObject<UFlowAsset>(Package, ResolvedAssetClass, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	if (!FlowAsset)
	{
		OutErrorMessage = FString::Printf(TEXT("Failed to create FlowAsset: %s"), *AssetName);
		return nullptr;
	}

	FlowAsset->AssetGuid = FGuid::NewGuid();
	FlowAsset->bWorldBound = bWorldBound;

	FAssetRegistryModule::AssetCreated(FlowAsset);

	TMap<FGuid, UFlowNode*> NodeMap;
	if (!CreateFlowNodes(FlowAsset, ParsedNodes, NodeMap, OutErrorMessage))
	{
		return nullptr;
	}

	if (!CreateNodeAddOns(NodeMap, ParsedNodes, OutErrorMessage))
	{
		return nullptr;
	}

#if WITH_EDITOR
	// Auto data pins depend on a node's addons, so CreateFlowNodes' set above is stale for a node
	// that just got one. Regenerate before SetupConnections wires against them below.
	for (const TPair<FGuid, UFlowNode*>& NodeEntry : NodeMap)
	{
		if (UFlowNode* Node = NodeEntry.Value)
		{
			Node->TryUpdateAutoDataPins();
		}
	}
#endif

	if (!SetupConnections(FlowAsset, NodeMap, ParsedConnections, ParsedNodes, OutErrorMessage))
	{
		return nullptr;
	}

#if WITH_EDITOR
	// Rebuild CustomInputs/CustomOutputs editor arrays so FlowNode_SubGraph context pins regenerate.
	FlowAsset->RebuildCustomInterfaceLists();
#endif

	Package->MarkPackageDirty();

	return FlowAsset;
}

bool UFlowGraphImporter::PopulateFlowAssetFromDocument(
	UFlowAsset* TargetAsset,
	bool bWorldBound,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const TArray<FFlowGraphParsedConnection>& ParsedConnections,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	if (!TargetAsset)
	{
		OutErrorMessage = TEXT("TargetAsset is null");
		return false;
	}

	if (UFlowGraph* FlowGraph = Cast<UFlowGraph>(TargetAsset->GetGraph()))
	{
		FlowGraph->Nodes.Empty();
	}

	FMapProperty* NodesProperty = FindFProperty<FMapProperty>(UFlowAsset::StaticClass(), TEXT("Nodes"));
	if (!NodesProperty)
	{
		OutErrorMessage = TEXT("Could not find Nodes property on UFlowAsset");
		return false;
	}

	void* NodesPtr = NodesProperty->ContainerPtrToValuePtr<void>(TargetAsset);
	FScriptMapHelper NodesMapHelper(NodesProperty, NodesPtr);
	NodesMapHelper.EmptyValues();

	TargetAsset->bWorldBound = bWorldBound;

	TMap<FGuid, UFlowNode*> NodeMap;
	if (!CreateFlowNodes(TargetAsset, ParsedNodes, NodeMap, OutErrorMessage))
	{
		return false;
	}

	if (!CreateNodeAddOns(NodeMap, ParsedNodes, OutErrorMessage))
	{
		return false;
	}

#if WITH_EDITOR
	// See the matching comment in ImportFlowGraphFromDocument.
	for (const TPair<FGuid, UFlowNode*>& NodeEntry : NodeMap)
	{
		if (UFlowNode* Node = NodeEntry.Value)
		{
			Node->TryUpdateAutoDataPins();
		}
	}
#endif

	if (!SetupConnections(TargetAsset, NodeMap, ParsedConnections, ParsedNodes, OutErrorMessage))
	{
		return false;
	}

#if WITH_EDITOR
	TargetAsset->RebuildCustomInterfaceLists();
#endif

	TargetAsset->GetPackage()->MarkPackageDirty();

	return true;
}

bool UFlowGraphImporter::CreateFlowNodes(UFlowAsset* FlowAsset, const TArray<FFlowGraphParsedNode>& ParsedNodes, TMap<FGuid, UFlowNode*>& OutNodeMap, FString& OutErrorMessage)
{
	// Use reflection to access the private Nodes map
	FMapProperty* NodesProperty = FindFProperty<FMapProperty>(UFlowAsset::StaticClass(), TEXT("Nodes"));
	if (!NodesProperty)
	{
		OutErrorMessage = TEXT("Could not find Nodes property on UFlowAsset");
		return false;
	}

	void* NodesPtr = NodesProperty->ContainerPtrToValuePtr<void>(FlowAsset);
	FScriptMapHelper NodesMapHelper(NodesProperty, NodesPtr);

	for (const FFlowGraphParsedNode& ParsedNode : ParsedNodes)
	{
		UClass* NodeClass = ResolveNodeClass(ParsedNode.NodeType);
		if (!NodeClass)
		{
			OutErrorMessage = FString::Printf(TEXT("Could not find node class: %s"), *ParsedNode.NodeType);
			return false;
		}

		UFlowNode* NewNode = NewObject<UFlowNode>(FlowAsset, NodeClass, NAME_None, RF_Transactional);
		if (!NewNode)
		{
			OutErrorMessage = FString::Printf(TEXT("Failed to create node of type: %s"), *ParsedNode.NodeType);
			return false;
		}

		NewNode->SetGuid(ParsedNode.NodeGuid);

		const int32 NewIndex = NodesMapHelper.AddDefaultValue_Invalid_NeedsRehash();
		uint8* PairPtr = NodesMapHelper.GetPairPtr(NewIndex);

		if (FStructProperty* KeyProperty = CastField<FStructProperty>(NodesProperty->KeyProp))
		{
			void* KeyPtr = KeyProperty->ContainerPtrToValuePtr<void>(PairPtr);
			KeyProperty->Struct->CopyScriptStruct(KeyPtr, &ParsedNode.NodeGuid);
		}

		if (FObjectProperty* ValueProperty = CastField<FObjectProperty>(NodesProperty->ValueProp))
		{
			ValueProperty->SetObjectPropertyValue_InContainer(PairPtr, NewNode);
		}

		NodesMapHelper.Rehash();

		OutNodeMap.Add(ParsedNode.NodeGuid, NewNode);

		// Set properties
		if (!SetNodeProperties(NewNode, ParsedNode.Properties, OutErrorMessage))
		{
			return false;
		}

#if WITH_EDITOR
		// Rebuild the pins a node generates from its own descriptor properties. The document never
		// carries that state - the exporter writes editable properties only, on the contract that the
		// node regenerates the rest. UFlowAsset::RegisterNode does this for a node created through the
		// editor, but this importer writes into the private Nodes map directly and so never reaches it.
		// Runs after SetNodeProperties because the descriptors are its input, and before
		// ApplyDeclaredPins, which is idempotent and only fills in what is still missing.
		NewNode->TryUpdateAutoDataPins();
#endif

		ApplyDeclaredPins(NewNode, ParsedNode);
	}

	return true;
}

bool UFlowGraphImporter::SetNodeProperties(UFlowNode* FlowNode, const TMap<FString, FString>& Properties, FString& OutErrorMessage)
{
	for (const TPair<FString, FString>& PropertyPair : Properties)
	{
		FProperty* Property = FlowNode->GetClass()->FindPropertyByName(*PropertyPair.Key);
		if (!Property)
		{
			Property = FindFProperty<FProperty>(FlowNode->GetClass(), *PropertyPair.Key);
		}

		void* ValuePtr = nullptr;
		if (Property)
		{
			ValuePtr = Property->ContainerPtrToValuePtr<void>(FlowNode);
		}
		else if (PropertyPair.Key.Contains(TEXT(".")))
		{
			FString PathError;
			if (!ResolvePropertyPath(FlowNode->GetClass(), FlowNode, PropertyPair.Key, Property, ValuePtr, PathError))
			{
				OutErrorMessage = FString::Printf(TEXT("Failed to resolve dotted property path '%s' on node %s: %s"),
					*PropertyPair.Key, *FlowNode->GetClass()->GetName(), *PathError);
				UE_LOG(LogFlow, Error, TEXT("%s"), *OutErrorMessage);

				return false;
			}
		}
		else
		{
			OutErrorMessage = FString::Printf(TEXT("Could not find property %s on node %s"), *PropertyPair.Key, *FlowNode->GetClass()->GetName());
			UE_LOG(LogFlow, Error, TEXT("%s"), *OutErrorMessage);

			return false;
		}

		FString InnerError;
		if (!SetPropertyFromString(Property, ValuePtr, PropertyPair.Value, InnerError, FlowNode))
		{
			OutErrorMessage = FString::Printf(TEXT("Failed to set property '%s' on node %s (GUID %s): %s"),
				*PropertyPair.Key, *FlowNode->GetClass()->GetName(), *FlowNode->GetGuid().ToString(), *InnerError);
			UE_LOG(LogFlow, Error, TEXT("%s"), *OutErrorMessage);
			return false;
		}
	}

	return true;
}

bool UFlowGraphImporter::SetupConnections(UFlowAsset* FlowAsset, const TMap<FGuid, UFlowNode*>& NodeMap, const TArray<FFlowGraphParsedConnection>& Connections, const TArray<FFlowGraphParsedNode>& ParsedNodes, FString& OutErrorMessage)
{
	const FFlowGraphDeclaredOutputPinTypes DeclaredOutputPinTypes = BuildDeclaredOutputPinTypes(ParsedNodes);

	FMapProperty* ConnectionsProperty = FindFProperty<FMapProperty>(UFlowNode::StaticClass(), TEXT("Connections"));
	if (!ConnectionsProperty)
	{
		OutErrorMessage = TEXT("Could not find Connections property on UFlowNode");
		UE_LOG(LogFlow, Error, TEXT("FlowGraphImporter: %s"), *OutErrorMessage);
		return false;
	}

	for (const FFlowGraphParsedConnection& Connection : Connections)
	{
		UFlowNode* SourceNode = NodeMap.FindRef(Connection.SourceNodeGuid);
		UFlowNode* TargetNode = NodeMap.FindRef(Connection.TargetNodeGuid);

		if (!SourceNode)
		{
			OutErrorMessage = FString::Printf(TEXT("Could not find source node: %s"), *Connection.SourceNodeGuid.ToString());
			UE_LOG(LogFlow, Error, TEXT("FlowGraphImporter: %s"), *OutErrorMessage);
			return false;
		}
		if (!TargetNode)
		{
			OutErrorMessage = FString::Printf(TEXT("Could not find target node: %s"), *Connection.TargetNodeGuid.ToString());
			UE_LOG(LogFlow, Error, TEXT("FlowGraphImporter: %s"), *OutErrorMessage);
			return false;
		}

		bool bIsExecPin = false;
		if (!ResolveSourcePin(SourceNode, Connection.SourcePinName, DeclaredOutputPinTypes, bIsExecPin))
		{
			UE_LOG(LogFlow, Warning, TEXT("FlowGraphImporter: Could not resolve source pin %s on node %s - skipping connection"),
				*Connection.SourcePinName.ToString(), *SourceNode->GetClass()->GetName());
			continue;
		}

		if (!bIsExecPin)
		{
			EnsureTargetDataPinExists(TargetNode, Connection.TargetPinName);
		}

		// Exec connections key on the source output pin; data connections key on the target input pin.
		UFlowNode* NodeToModify    = bIsExecPin ? SourceNode : TargetNode;
		FName      KeyName         = bIsExecPin ? Connection.SourcePinName : Connection.TargetPinName;
		FConnectedPin ConnectedPin = bIsExecPin
			? FConnectedPin(Connection.TargetNodeGuid, Connection.TargetPinName)
			: FConnectedPin(Connection.SourceNodeGuid, Connection.SourcePinName);

		if (!WriteConnectionToNode(NodeToModify, KeyName, ConnectedPin, ConnectionsProperty))
		{
			UE_LOG(LogFlow, Warning, TEXT("FlowGraphImporter: WriteConnectionToNode failed for key %s on %s"),
				*KeyName.ToString(), *NodeToModify->GetClass()->GetName());
		}
	}

	return true;
}

FFlowGraphDeclaredOutputPinTypes UFlowGraphImporter::BuildDeclaredOutputPinTypes(const TArray<FFlowGraphParsedNode>& ParsedNodes)
{
	FFlowGraphDeclaredOutputPinTypes Result;
	for (const FFlowGraphParsedNode& ParsedNode : ParsedNodes)
	{
		TMap<FName, FDeclaredPinInfo>& PinTypeMap = Result.FindOrAdd(ParsedNode.NodeGuid);
		for (const FString& PinDecl : ParsedNode.OutputPins)
		{
			FName ParsedPinName;
			FString ParsedPinType;
			FString ParsedSubCatPath;
			if (ParsePinDecl(PinDecl, ParsedPinName, ParsedPinType, ParsedSubCatPath))
			{
				FDeclaredPinInfo& Info = PinTypeMap.FindOrAdd(ParsedPinName);
				// OutputPins is always an exec-pin array on UFlowNode, so an omitted type means Exec.
				Info.TypeStr          = ParsedPinType.IsEmpty() ? FString(TEXT("Exec")) : ParsedPinType;
				Info.SubCategoryPath  = ParsedSubCatPath;
			}
		}
	}
	return Result;
}

bool UFlowGraphImporter::ResolveSourcePin(UFlowNode* SourceNode, FName SourcePinName, const FFlowGraphDeclaredOutputPinTypes& DeclaredOutputPinTypes, bool& bOutIsExecPin)
{
	for (const FFlowPin& Pin : SourceNode->GetOutputPins())
	{
		if (Pin.PinName == SourcePinName)
		{
			bOutIsExecPin = Pin.IsExecPin();
			return true;
		}
	}

	// Fall back to declared pins from the import text (handles dynamic pins absent from CDO).
	// ApplyDeclaredPins should have already created the pin during CreateFlowNodes; this path is
	// reached only when a connection references a pin that wasn't in the declared list either
	// (fully dynamic, not exported - rare). Use the declared type if available.
	FArrayProperty* OutputPinsProperty = FindFProperty<FArrayProperty>(UFlowNode::StaticClass(), TEXT("OutputPins"));

	const TMap<FName, FDeclaredPinInfo>* DeclaredPins = DeclaredOutputPinTypes.Find(SourceNode->GetGuid());
	if (DeclaredPins)
	{
		const FDeclaredPinInfo* Info = DeclaredPins->Find(SourcePinName);
		if (Info)
		{
			bOutIsExecPin = Info->TypeStr.Equals(TEXT("Exec"), ESearchCase::IgnoreCase);
			if (OutputPinsProperty)
			{
				FScriptArrayHelper Helper(OutputPinsProperty, OutputPinsProperty->ContainerPtrToValuePtr<void>(SourceNode));
				FFlowPin* NewPin = reinterpret_cast<FFlowPin*>(Helper.GetRawPtr(Helper.AddValue()));
				NewPin->PinName    = SourcePinName;
				NewPin->PinToolTip = SourcePinName.ToString();
				NewPin->SetPinTypeName(FFlowPinTypeName(*Info->TypeStr));
				if (!Info->SubCategoryPath.IsEmpty())
				{
					if (UObject* SubCatObject = LoadObject<UObject>(nullptr, *Info->SubCategoryPath))
					{
						NewPin->SetPinSubCategoryObject(SubCatObject);
					}
				}
			}
			return true;
		}
	}

	// Last resort: inject as wildcard data output.
	UE_LOG(LogFlow, Warning, TEXT("FlowGraphImporter: Source pin %s not found on %s - adding as wildcard data output"),
		*SourcePinName.ToString(), *SourceNode->GetClass()->GetName());

	if (OutputPinsProperty)
	{
		FScriptArrayHelper Helper(OutputPinsProperty, OutputPinsProperty->ContainerPtrToValuePtr<void>(SourceNode));
		FFlowPin* NewPin = reinterpret_cast<FFlowPin*>(Helper.GetRawPtr(Helper.AddValue()));
		NewPin->PinName    = SourcePinName;
		NewPin->PinToolTip = SourcePinName.ToString();
		NewPin->SetPinTypeName(FFlowPinTypeName(TEXT("wildcard")));
		bOutIsExecPin = false;
		return true;
	}

	UE_LOG(LogFlow, Error, TEXT("FlowGraphImporter: OutputPins property not found on UFlowNode"));
	return false;
}

bool UFlowGraphImporter::EnsureTargetDataPinExists(UFlowNode* TargetNode, FName TargetPinName)
{
	for (const FFlowPin& Pin : TargetNode->GetInputPins())
	{
		if (Pin.PinName == TargetPinName)
		{
			return true;
		}
	}

	// The pin should already exist if ApplyDeclaredPins ran (called from CreateFlowNodes). This
	// fallback only fires for a connection referencing an input pin not listed in InputPins: at
	// all (fully-dynamic pin not present in the export). Inject as wildcard - it is the best we
	// can do without type information.
	UE_LOG(LogFlow, Warning, TEXT("FlowGraphImporter: Target pin %s not found on %s - adding as wildcard data input"),
		*TargetPinName.ToString(), *TargetNode->GetClass()->GetName());

	FArrayProperty* InputPinsProperty = FindFProperty<FArrayProperty>(UFlowNode::StaticClass(), TEXT("InputPins"));
	if (!InputPinsProperty)
	{
		UE_LOG(LogFlow, Error, TEXT("FlowGraphImporter: InputPins property not found on UFlowNode"));
		return false;
	}

	FScriptArrayHelper Helper(InputPinsProperty, InputPinsProperty->ContainerPtrToValuePtr<void>(TargetNode));
	FFlowPin* NewPin = reinterpret_cast<FFlowPin*>(Helper.GetRawPtr(Helper.AddValue()));
	NewPin->PinName    = TargetPinName;
	NewPin->PinToolTip = TargetPinName.ToString();
	NewPin->SetPinTypeName(FFlowPinTypeName(TEXT("wildcard")));
	return true;
}

bool UFlowGraphImporter::WriteConnectionToNode(UFlowNode* NodeToModify, FName KeyName, const FConnectedPin& ConnectedPin, FMapProperty* ConnectionsProperty)
{
	void* ConnectionsPtr = ConnectionsProperty->ContainerPtrToValuePtr<void>(NodeToModify);
	FScriptMapHelper MapHelper(ConnectionsProperty, ConnectionsPtr);

	// KeyName may already have a value - e.g. re-wiring an exec output pin that was already
	// connected to something else (single-target). TMap requires unique keys, so blindly appending
	// a new pair here (rather than updating the existing one) would leave two pairs sharing the
	// same key, corrupting the map (later Find/FindRef calls can then return either pair, or
	// garbage, depending on hash bucket layout).
	uint8* PairPtr = nullptr;
	if (FNameProperty* KeyPropForFind = CastField<FNameProperty>(ConnectionsProperty->KeyProp))
	{
		for (int32 Index = 0; Index < MapHelper.GetMaxIndex(); ++Index)
		{
			if (!MapHelper.IsValidIndex(Index))
			{
				continue;
			}

			uint8* CandidatePairPtr = MapHelper.GetPairPtr(Index);
			if (KeyPropForFind->GetPropertyValue_InContainer(CandidatePairPtr) == KeyName)
			{
				PairPtr = CandidatePairPtr;
				break;
			}
		}
	}

	if (!PairPtr)
	{
		const int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
		PairPtr = MapHelper.GetPairPtr(NewIndex);

		if (FNameProperty* KeyProp = CastField<FNameProperty>(ConnectionsProperty->KeyProp))
		{
			KeyProp->SetPropertyValue_InContainer(PairPtr, KeyName);
		}

		MapHelper.Rehash();
	}

	if (FStructProperty* ValueProp = CastField<FStructProperty>(ConnectionsProperty->ValueProp))
	{
		*static_cast<FConnectedPin*>(ValueProp->ContainerPtrToValuePtr<void>(PairPtr)) = ConnectedPin;
	}

	return true;
}

UClass* UFlowGraphImporter::ResolveNodeClassForQuery(const FString& NodeType)
{
	UClass* NodeClass = nullptr;

	// Full path (contains /Script/ or a dot) - try direct load
	if (NodeType.Contains(TEXT("/Script/")) || NodeType.Contains(TEXT(".")))
	{
		NodeClass = LoadObject<UClass>(nullptr, *NodeType);
	}

	// Short name - try /Script/Flow prefix
	if (!NodeClass)
	{
		NodeClass = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Flow.%s"), *NodeType));
	}
	if (!NodeClass)
	{
		NodeClass = LoadObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Flow.%s"), *NodeType));
	}

	// Last resort: as-is
	if (!NodeClass)
	{
		NodeClass = FindObject<UClass>(nullptr, *NodeType);
	}

	return NodeClass;
}

bool UFlowGraphImporter::IsClassFromNonFlowBlueprint(const UClass* InClass, FString& OutReason)
{
	OutReason.Reset();

	if (!InClass)
	{
		return false;
	}

#if WITH_EDITORONLY_DATA
	// Native and AngelScript classes carry no generating Blueprint, so there is nothing to police.
	// Fail open for them rather than guessing, so a missing ClassGeneratedBy can never block a
	// legitimate compiled-in node type.
	const UBlueprint* GeneratingBlueprint = Cast<UBlueprint>(InClass->ClassGeneratedBy);
	if (!GeneratingBlueprint)
	{
		return false;
	}

	// UFlowNodeBlueprint and UFlowNodeAddOnBlueprint are siblings under UBlueprint, not a shared
	// base, so both must be named explicitly - testing only the node form would reject every addon.
	if (GeneratingBlueprint->IsA<UFlowNodeBlueprint>() || GeneratingBlueprint->IsA<UFlowNodeAddOnBlueprint>())
	{
		return false;
	}

	OutReason = FString::Printf(
		TEXT("'%s' is generated by '%s', a plain %s rather than a FlowNodeBlueprint or ")
		TEXT("FlowNodeAddOnBlueprint. It compiles, but the Flow palette and the Courier catalog both ")
		TEXT("gather by asset class, so neither can see it. Recreate it with the ")
		TEXT("CreateFlowNodeBlueprint op."),
		*InClass->GetPathName(),
		*GeneratingBlueprint->GetPathName(),
		*GeneratingBlueprint->GetClass()->GetName());
	return true;
#else
	return false;
#endif // WITH_EDITORONLY_DATA
}

UClass* UFlowGraphImporter::ResolveNodeClass(const FString& NodeType)
{
	UClass* NodeClass = ResolveNodeClassForQuery(NodeType);

	FString RejectionReason;
	if (IsClassFromNonFlowBlueprint(NodeClass, RejectionReason))
	{
		UE_LOG(LogFlow, Warning, TEXT("ResolveNodeClass rejected '%s': %s"), *NodeType, *RejectionReason);
		return nullptr;
	}

	return NodeClass;
}

bool UFlowGraphImporter::CreateNodeAddOns(const TMap<FGuid, UFlowNode*>& NodeMap, const TArray<FFlowGraphParsedNode>& ParsedNodes, FString& OutErrorMessage)
{
	for (const FFlowGraphParsedNode& ParsedNode : ParsedNodes)
	{
		if (ParsedNode.AddOns.IsEmpty())
		{
			continue;
		}

		UFlowNode* OwnerNode = NodeMap.FindRef(ParsedNode.NodeGuid);
		if (!OwnerNode)
		{
			OutErrorMessage = FString::Printf(TEXT("CreateNodeAddOns: Could not find node for GUID %s"), *ParsedNode.NodeGuid.ToString());
			return false;
		}

		if (!CreateAddOnsRecursive(OwnerNode, ParsedNode.AddOns, OutErrorMessage))
		{
			return false;
		}
	}

	return true;
}

bool UFlowGraphImporter::CreateAddOnsRecursive(UFlowNodeBase* Owner, const TArray<FFlowGraphParsedNodeAddOn>& ParsedAddOns, FString& OutErrorMessage)
{
	if (ParsedAddOns.IsEmpty())
	{
		return true;
	}

	FArrayProperty* AddOnsProperty = FindFProperty<FArrayProperty>(UFlowNodeBase::StaticClass(), TEXT("AddOns"));
	if (!AddOnsProperty)
	{
		OutErrorMessage = TEXT("CreateAddOnsRecursive: Could not find AddOns property on UFlowNodeBase");
		return false;
	}

	FObjectProperty* ElemProp = CastField<FObjectProperty>(AddOnsProperty->Inner);
	if (!ElemProp)
	{
		OutErrorMessage = TEXT("CreateAddOnsRecursive: AddOns array element property is not an object property");
		return false;
	}

	for (const FFlowGraphParsedNodeAddOn& ParsedAddOn : ParsedAddOns)
	{
		if (ParsedAddOn.bIsDeleteMarker)
		{
			// Meaningless for a brand new owner - nothing to delete yet. Only relevant for
			// FFlowGraphReconciler::ReconcileNodeAddOns on an already-existing owner.
			continue;
		}

		UClass* AddOnClass = nullptr;

		if (ParsedAddOn.AddOnType.Contains(TEXT("/Script/")) || ParsedAddOn.AddOnType.Contains(TEXT(".")))
		{
			AddOnClass = LoadObject<UClass>(nullptr, *ParsedAddOn.AddOnType);
		}

		if (!AddOnClass)
		{
			AddOnClass = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Flow.%s"), *ParsedAddOn.AddOnType));
		}

		if (!AddOnClass || !AddOnClass->IsChildOf(UFlowNodeAddOn::StaticClass()))
		{
			OutErrorMessage = FString::Printf(TEXT("CreateAddOnsRecursive: Could not find AddOn class: %s"), *ParsedAddOn.AddOnType);
			return false;
		}

		// NewObject on an abstract class trips an engine-level
		// ensure() rather than returning null, which crashes the editor session mid-import.
		if (AddOnClass->HasAnyClassFlags(CLASS_Abstract))
		{
			OutErrorMessage = FString::Printf(TEXT("CreateAddOnsRecursive: AddOn class is abstract and cannot be instantiated: %s"), *ParsedAddOn.AddOnType);
			return false;
		}

		UFlowNodeAddOn* NewAddOn = NewObject<UFlowNodeAddOn>(Owner, AddOnClass, NAME_None, RF_Transactional);
		if (!NewAddOn)
		{
			OutErrorMessage = FString::Printf(TEXT("CreateAddOnsRecursive: Failed to create AddOn of type: %s"), *ParsedAddOn.AddOnType);
			return false;
		}

		NewAddOn->SetGuid(ParsedAddOn.AddOnGuid.IsValid() ? ParsedAddOn.AddOnGuid : FGuid::NewGuid());

		if (!SetAddOnProperties(NewAddOn, ParsedAddOn.Properties, OutErrorMessage))
		{
			return false;
		}

		// Append to Owner's AddOns array via reflection (AddOns is protected). Re-acquire the
		// helper each iteration - a sibling append below could have reallocated the array.
		void* AddOnsPtr = AddOnsProperty->ContainerPtrToValuePtr<void>(Owner);
		FScriptArrayHelper AddOnsHelper(AddOnsProperty, AddOnsPtr);
		const int32 NewIndex = AddOnsHelper.AddValue();
		ElemProp->SetObjectPropertyValue(AddOnsHelper.GetRawPtr(NewIndex), NewAddOn);

		if (!CreateAddOnsRecursive(NewAddOn, ParsedAddOn.AddOns, OutErrorMessage))
		{
			return false;
		}
	}

	return true;
}

#if WITH_EDITORONLY_DATA
void UFlowGraphImporter::SyncAddOnDataPinValueDirections(UFlowNodeAddOn* AddOn)
{
	if (!AddOn)
	{
		return;
	}

	const UFlowNodeAddOn* CDO = Cast<UFlowNodeAddOn>(AddOn->GetClass()->GetDefaultObject());
	if (!CDO)
	{
		return;
	}

	static const UScriptStruct* NamedDataPinPropertyStruct = FFlowNamedDataPinProperty::StaticStruct();

	for (TFieldIterator<FStructProperty> PropIt(AddOn->GetClass()); PropIt; ++PropIt)
	{
		FStructProperty* StructProp = *PropIt;
		if (!StructProp || !StructProp->Struct->IsChildOf(NamedDataPinPropertyStruct))
		{
			continue;
		}

		FFlowNamedDataPinProperty* InstanceProp = StructProp->ContainerPtrToValuePtr<FFlowNamedDataPinProperty>(AddOn);
		const FFlowNamedDataPinProperty* CDOProp = StructProp->ContainerPtrToValuePtr<FFlowNamedDataPinProperty>(CDO);

		if (!InstanceProp || !CDOProp)
		{
			continue;
		}

		FFlowDataPinValue* InstanceValue = InstanceProp->DataPinValue.GetMutablePtr<FFlowDataPinValue>();
		const FFlowDataPinValue* CDOValue = CDOProp->DataPinValue.GetPtr<FFlowDataPinValue>();

		if (InstanceValue && CDOValue)
		{
			InstanceValue->bIsInputPin = CDOValue->bIsInputPin;
		}
	}
}
#endif

bool UFlowGraphImporter::SetAddOnProperties(UFlowNodeAddOn* AddOn, const TMap<FString, FString>& Properties, FString& OutErrorMessage)
{
	for (const TPair<FString, FString>& Pair : Properties)
	{
		FProperty* Property = AddOn->GetClass()->FindPropertyByName(*Pair.Key);
		if (!Property)
		{
			Property = FindFProperty<FProperty>(AddOn->GetClass(), *Pair.Key);
		}

		void* ValuePtr = nullptr;
		if (Property)
		{
			ValuePtr = Property->ContainerPtrToValuePtr<void>(AddOn);
		}
		else if (Pair.Key.Contains(TEXT(".")))
		{
			FString PathError;
			if (!ResolvePropertyPath(AddOn->GetClass(), AddOn, Pair.Key, Property, ValuePtr, PathError))
			{
				OutErrorMessage = FString::Printf(TEXT("Failed to resolve dotted property path '%s' on addon %s: %s"),
					*Pair.Key, *AddOn->GetClass()->GetName(), *PathError);
				UE_LOG(LogFlow, Error, TEXT("%s"), *OutErrorMessage);

				return false;
			}
		}
		else
		{
			OutErrorMessage = FString::Printf(TEXT("SetAddOnProperties: Could not find property %s on %s"), *Pair.Key, *AddOn->GetClass()->GetName());
			UE_LOG(LogFlow, Error, TEXT("%s"), *OutErrorMessage);

			return false;
		}

		FString InnerError;
		if (!SetPropertyFromString(Property, ValuePtr, Pair.Value, InnerError, AddOn))
		{
			OutErrorMessage = FString::Printf(TEXT("Failed to set property '%s' on addon %s (GUID %s): %s"),
				*Pair.Key, *AddOn->GetClass()->GetName(), *AddOn->GetGuid().ToString(), *InnerError);
			UE_LOG(LogFlow, Error, TEXT("%s"), *OutErrorMessage);
			return false;
		}
	}

#if WITH_EDITORONLY_DATA
	SyncAddOnDataPinValueDirections(AddOn);
#endif

	return true;
}

bool UFlowGraphImporter::ResolvePropertyPath(UStruct* OwnerStruct, void* ContainerPtr, const FString& PropertyPath, FProperty*& OutProperty, void*& OutValuePtr, FString& OutErrorMessage)
{
	TArray<FString> Segments;
	PropertyPath.ParseIntoArray(Segments, TEXT("."), true);
	if (Segments.IsEmpty())
	{
		OutErrorMessage = FString::Printf(TEXT("Empty property path '%s'"), *PropertyPath);
		return false;
	}

	UStruct* CurrentOwnerStruct = OwnerStruct;
	void* CurrentContainerPtr = ContainerPtr;

	// Set when the previous segment resolved to an FArrayProperty - the next segment must be a
	// numeric index into it rather than a property name on CurrentOwnerStruct.
	FArrayProperty* PendingArrayProperty = nullptr;
	void* PendingArrayContainerPtr = nullptr;

	FProperty* ResolvedProperty = nullptr;
	void* ResolvedValuePtr = nullptr;

	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
	{
		const FString& Segment = Segments[SegmentIndex];
		const bool bIsLastSegment = (SegmentIndex == Segments.Num() - 1);

		if (PendingArrayProperty)
		{
			if (!Segment.IsNumeric())
			{
				OutErrorMessage = FString::Printf(TEXT("Expected an array index after '%s' in path '%s', got '%s'"),
					*PendingArrayProperty->GetName(), *PropertyPath, *Segment);
				return false;
			}

			FScriptArrayHelper ArrayHelper(PendingArrayProperty, PendingArrayContainerPtr);
			const int32 ArrayIndex = FCString::Atoi(*Segment);
			if (!ArrayHelper.IsValidIndex(ArrayIndex))
			{
				OutErrorMessage = FString::Printf(TEXT("Array index %d out of bounds (size %d) for '%s' in path '%s'"),
					ArrayIndex, ArrayHelper.Num(), *PendingArrayProperty->GetName(), *PropertyPath);
				return false;
			}

			ResolvedProperty = PendingArrayProperty->Inner;
			ResolvedValuePtr = ArrayHelper.GetRawPtr(ArrayIndex);
			PendingArrayProperty = nullptr;
		}
		else
		{
			if (!CurrentOwnerStruct)
			{
				OutErrorMessage = FString::Printf(TEXT("Cannot resolve '%s' in path '%s' - preceding segment is not a struct"),
					*Segment, *PropertyPath);
				return false;
			}

			FProperty* Property = CurrentOwnerStruct->FindPropertyByName(*Segment);
			if (!Property)
			{
				Property = FindFProperty<FProperty>(CurrentOwnerStruct, *Segment);
			}

			if (!Property)
			{
				OutErrorMessage = FString::Printf(TEXT("Could not find property '%s' (in path '%s') on %s"),
					*Segment, *PropertyPath, *CurrentOwnerStruct->GetName());
				return false;
			}

			ResolvedProperty = Property;
			ResolvedValuePtr = Property->ContainerPtrToValuePtr<void>(CurrentContainerPtr);
		}

		if (bIsLastSegment)
		{
			break;
		}

		if (FStructProperty* StructProperty = CastField<FStructProperty>(ResolvedProperty))
		{
			CurrentOwnerStruct = StructProperty->Struct;
			CurrentContainerPtr = ResolvedValuePtr;
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(ResolvedProperty))
		{
			PendingArrayProperty = ArrayProperty;
			PendingArrayContainerPtr = ResolvedValuePtr;
			CurrentOwnerStruct = nullptr;
		}
		else
		{
			OutErrorMessage = FString::Printf(TEXT("Cannot descend into '%s' (in path '%s') - property is a %s, which is neither a struct nor an array"),
				*Segment, *PropertyPath, *ResolvedProperty->GetClass()->GetName());
			return false;
		}
	}

	OutProperty = ResolvedProperty;
	OutValuePtr = ResolvedValuePtr;
	return true;
}

// Splits "/Script/Foo.Bar(A=1,B=(C=2))" into its class path and its field block, so an instanced
// object property can be authored as a class plus inline field values. Returns false for a plain
// object path, which keeps the pre-existing LoadObject behaviour untouched.
static bool TryParseClassWithFields(const FString& ValueString, FString& OutClassPath, FString& OutFields)
{
	const FString Trimmed = ValueString.TrimStartAndEnd();
	if (!Trimmed.EndsWith(TEXT(")")))
	{
		return false;
	}

	int32 OpenIndex = INDEX_NONE;
	if (!Trimmed.FindChar(TEXT('('), OpenIndex) || OpenIndex <= 0)
	{
		return false;
	}

	OutClassPath = Trimmed.Left(OpenIndex).TrimStartAndEnd();
	OutFields = Trimmed.Mid(OpenIndex + 1, Trimmed.Len() - OpenIndex - 2);

	return !OutClassPath.IsEmpty();
}

// Splits a field block on commas that sit outside any parentheses or quoted string, so a nested
// struct value such as Configuration=(Name="A.B") survives as one field rather than being
// torn in half at its inner comma.
static void SplitFieldsAtTopLevel(const FString& Fields, TArray<FString>& OutFields)
{
	int32 Depth = 0;
	bool bInQuotes = false;
	FString Current;

	for (int32 CharIndex = 0; CharIndex < Fields.Len(); ++CharIndex)
	{
		const TCHAR Character = Fields[CharIndex];
		const bool bIsEscaped = bInQuotes && CharIndex > 0 && Fields[CharIndex - 1] == TEXT('\\');

		if (Character == TEXT('"') && !bIsEscaped)
		{
			bInQuotes = !bInQuotes;
		}
		else if (!bInQuotes && (Character == TEXT('(') || Character == TEXT('[')))
		{
			++Depth;
		}
		else if (!bInQuotes && (Character == TEXT(')') || Character == TEXT(']')))
		{
			--Depth;
		}
		else if (!bInQuotes && Depth == 0 && Character == TEXT(','))
		{
			OutFields.Add(Current.TrimStartAndEnd());
			Current.Reset();

			continue;
		}

		Current.AppendChar(Character);
	}

	const FString Last = Current.TrimStartAndEnd();
	if (!Last.IsEmpty())
	{
		OutFields.Add(Last);
	}
}

// True when Property is meant to own the object it points at, rather than reference a shared asset.
// Only these may be authored as a class plus fields; a plain object reference must keep resolving
// through LoadObject so an ordinary asset pointer is never silently replaced by a fresh instance.
static bool IsInstancedObjectProperty(const FObjectProperty& ObjectProperty)
{
	if (ObjectProperty.HasAnyPropertyFlags(CPF_InstancedReference | CPF_ExportObject))
	{
		return true;
	}

	return ObjectProperty.PropertyClass && ObjectProperty.PropertyClass->HasAnyClassFlags(CLASS_EditInlineNew);
}

bool UFlowGraphImporter::SetPropertyFromString(FProperty* Property, void* ValuePtr, const FString& ValueString, FString& OutErrorMessage,
	UObject* OwnerForInstancedSubobjects)
{
	if (!Property || !ValuePtr)
	{
		OutErrorMessage = TEXT("Invalid property or value pointer");
		return false;
	}

	// Handle different property types
	if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		bool bValue = ValueString.Equals(TEXT("true"), ESearchCase::IgnoreCase) ||
		              ValueString.Equals(TEXT("1"));
		BoolProperty->SetPropertyValue(ValuePtr, bValue);
		return true;
	}
	else if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
	{
		int32 Value = FCString::Atoi(*ValueString);
		IntProperty->SetPropertyValue(ValuePtr, Value);
		return true;
	}
	else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
	{
		float Value = FCString::Atof(*ValueString);
		FloatProperty->SetPropertyValue(ValuePtr, Value);
		return true;
	}
	else if (FStrProperty* StrProperty = CastField<FStrProperty>(Property))
	{
		// Strip surrounding quotes and unescape - mirror of the exporter's escaping.
		FString CleanString = ValueString;
		if (CleanString.Len() >= 2 && CleanString.StartsWith(TEXT("\"")) && CleanString.EndsWith(TEXT("\"")))
		{
			CleanString = CleanString.Mid(1, CleanString.Len() - 2);
		}

		StrProperty->SetPropertyValue(ValuePtr, CleanString.ReplaceEscapedCharWithChar());
		return true;
	}
	else if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
	{
		// Remove quotes if present
		FString CleanString = ValueString;
		if (CleanString.StartsWith(TEXT("\"")) && CleanString.EndsWith(TEXT("\"")))
		{
			CleanString = CleanString.Mid(1, CleanString.Len() - 2);
		}

		NameProperty->SetPropertyValue(ValuePtr, FName(*CleanString));
		return true;
	}
	else if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		// Handle NSLOCTEXT format: NSLOCTEXT("[namespace]", "key", "value")
		if (ValueString.StartsWith(TEXT("NSLOCTEXT")))
		{
			// Extract the display string (third parameter)
			int32 LastQuoteStart = ValueString.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			int32 SecondLastQuoteStart = ValueString.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromEnd, LastQuoteStart - 1);

			if (LastQuoteStart != INDEX_NONE && SecondLastQuoteStart != INDEX_NONE)
			{
				FString DisplayString = ValueString.Mid(SecondLastQuoteStart + 1, LastQuoteStart - SecondLastQuoteStart - 1);
				TextProperty->SetPropertyValue(ValuePtr, FText::FromString(DisplayString));
				return true;
			}
		}
		else
		{
			// Simple string-to-text: strip quotes and unescape.
			FString CleanString = ValueString;
			if (CleanString.StartsWith(TEXT("\"")) && CleanString.EndsWith(TEXT("\"")))
			{
				CleanString = CleanString.Mid(1, CleanString.Len() - 2);
			}
			TextProperty->SetPropertyValue(ValuePtr, FText::FromString(CleanString.ReplaceEscapedCharWithChar()));
			return true;
		}
	}
	else if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		// Check if this is a TInstancedStruct - it needs special handling
		if (StructProperty->Struct->GetName().StartsWith(TEXT("InstancedStruct")))
		{
			// TInstancedStruct has its own ImportText that handles the (StructType,Fields) format
			const TCHAR* Buffer = *ValueString;
			if (Property->ImportText_Direct(Buffer, ValuePtr, nullptr, PPF_None))
			{
				return true;
			}
			else
			{
				OutErrorMessage = FString::Printf(TEXT("Failed to import TInstancedStruct property from text: %s"), *ValueString);
				UE_LOG(LogFlow, Warning, TEXT("FlowGraphImporter: %s"), *OutErrorMessage);
				return false;
			}
		}
		else
		{
			// Regular struct - use the struct's ImportText
			if (StructProperty->Struct->ImportText(*ValueString, ValuePtr, nullptr, PPF_None, nullptr, StructProperty->Struct->GetName()))
			{
				return true;
			}
			else
			{
				OutErrorMessage = FString::Printf(TEXT("Failed to import struct property from text: %s"), *ValueString);
				return false;
			}
		}
	}
	else if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		// An instanced subobject does not exist until something creates it, so LoadObject can never
		// resolve one. Accept "ClassPath(Field=Value,...)" for these, mirroring the shape the
		// TInstancedStruct branch above already uses, and construct the subobject under its owner.
		FString SubobjectClassPath;
		FString SubobjectFields;
		if (TryParseClassWithFields(ValueString, SubobjectClassPath, SubobjectFields))
		{
			if (!IsInstancedObjectProperty(*ObjectProperty))
			{
				OutErrorMessage = FString::Printf(
					TEXT("Property '%s' is a plain object reference, not an instanced subobject - supply an object path rather than '%s'"),
					*ObjectProperty->GetName(), *ValueString);
				return false;
			}

			if (!IsValid(OwnerForInstancedSubobjects))
			{
				OutErrorMessage = FString::Printf(
					TEXT("Property '%s' needs an owning object to construct an instanced subobject; this call site supplied none"),
					*ObjectProperty->GetName());
				return false;
			}

			UClass* SubobjectClass = LoadClass<UObject>(nullptr, *SubobjectClassPath);
			if (!SubobjectClass)
			{
				OutErrorMessage = FString::Printf(TEXT("Failed to load class '%s' for instanced subobject property '%s'"),
					*SubobjectClassPath, *ObjectProperty->GetName());
				return false;
			}

			// Reuse the existing subobject when the class is unchanged, so a re-applied document
			// preserves subobject identity the way UpsertNode preserves a node's GUID.
			UObject* Subobject = ObjectProperty->GetObjectPropertyValue(ValuePtr);
			if (!IsValid(Subobject) || Subobject->GetClass() != SubobjectClass)
			{
				Subobject = NewObject<UObject>(OwnerForInstancedSubobjects, SubobjectClass, NAME_None, RF_Transactional);
				ObjectProperty->SetObjectPropertyValue(ValuePtr, Subobject);
			}

			TArray<FString> Fields;
			SplitFieldsAtTopLevel(SubobjectFields, Fields);

			for (const FString& Field : Fields)
			{
				FString FieldName;
				FString FieldValue;
				if (!Field.Split(TEXT("="), &FieldName, &FieldValue))
				{
					OutErrorMessage = FString::Printf(TEXT("Malformed field '%s' for instanced subobject property '%s'"),
						*Field, *ObjectProperty->GetName());
					return false;
				}

				FieldName.TrimStartAndEndInline();

				FProperty* FieldProperty = Subobject->GetClass()->FindPropertyByName(*FieldName);
				if (!FieldProperty)
				{
					OutErrorMessage = FString::Printf(TEXT("Class '%s' has no property '%s'"),
						*SubobjectClass->GetName(), *FieldName);
					return false;
				}

				void* FieldValuePtr = FieldProperty->ContainerPtrToValuePtr<void>(Subobject);

				FString FieldError;
				if (!SetPropertyFromString(FieldProperty, FieldValuePtr, FieldValue.TrimStartAndEnd(), FieldError, Subobject))
				{
					OutErrorMessage = FString::Printf(TEXT("Failed to set '%s' on instanced subobject of '%s': %s"),
						*FieldName, *SubobjectClass->GetName(), *FieldError);
					return false;
				}
			}

			return true;
		}

		// Try to load the object
		UObject* Object = LoadObject<UObject>(nullptr, *ValueString);
		if (Object)
		{
			ObjectProperty->SetObjectPropertyValue(ValuePtr, Object);
			return true;
		}
		else
		{
			OutErrorMessage = FString::Printf(TEXT("Failed to load object: %s"), *ValueString);
			return false;
		}
	}
	else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		const TCHAR* Buffer = *ValueString;
		if (ArrayProperty->ImportText_Direct(Buffer, ValuePtr, nullptr, PPF_None))
		{
			return true;
		}
		else
		{
			OutErrorMessage = FString::Printf(TEXT("Failed to import array property from text: %s"), *ValueString);
			UE_LOG(LogFlow, Warning, TEXT("FlowGraphImporter: %s"), *OutErrorMessage);
			return false;
		}
	}

	// For other types, try generic ImportText
	if (Property->ImportText_Direct(*ValueString, ValuePtr, nullptr, PPF_None))
	{
		return true;
	}

	OutErrorMessage = FString::Printf(TEXT("Unsupported property type: %s"), *Property->GetClass()->GetName());
	return false;
}

static FName ParsePinNameToken(const FString& RawToken)
{
	// A pin name is normally free text with incidental whitespace trimmed - but exactly one class
	// declares an intentionally-blank pin name (a single-space-named exec pin, seen in some
	// project-specific nodes), and trimming that down to an empty string collapses it to NAME_None, which
	// then fails to resolve against the real pin. If trimming would erase an otherwise non-empty
	// token, the token *was* meaningful whitespace - fall back to a single space rather than losing
	// it, matching that class's own sentinel value.
	const FString Trimmed = RawToken.TrimStartAndEnd();
	if (Trimmed.IsEmpty() && !RawToken.IsEmpty())
	{
		return FName(TEXT(" "));
	}
	return FName(*Trimmed);
}

bool UFlowGraphImporter::ParsePinDecl(const FString& PinDecl, FName& OutPinName, FString& OutPinType, FString& OutSubCategoryPath)
{
	// Trim only the trailing edge here, for the same reason ParseNodes's pin-line extraction does -
	// the leading edge can legitimately be (part of) an intentionally-blank pin's name, and a full
	// trim would erase it before the " [" name/type boundary below is ever located.
	FString Trimmed = PinDecl;
	Trimmed.TrimEndInline();
	if (Trimmed.IsEmpty())
	{
		return false;
	}

	FString PinName;
	FString BracketContent;
	if (Trimmed.Split(TEXT(" ["), &PinName, &BracketContent))
	{
		BracketContent.RemoveFromEnd(TEXT("]"));
		BracketContent = BracketContent.TrimStartAndEnd();

		// Sub-category form: "PinType:/Object/Path" - split on first ':'
		FString TypePart;
		FString SubCatPart;
		if (BracketContent.Split(TEXT(":"), &TypePart, &SubCatPart))
		{
			OutPinType          = TypePart.TrimStartAndEnd();
			OutSubCategoryPath  = SubCatPart.TrimStartAndEnd();
		}
		else
		{
			OutPinType         = BracketContent;
			OutSubCategoryPath = FString();
		}
		OutPinName = ParsePinNameToken(PinName);
	}
	else
	{
		// No " [Type]" suffix at all (bare name) - this shape never carries the blank-pin ambiguity
		// (the real format always includes a type annotation), so a normal full trim is correct here.
		OutPinName         = FName(*Trimmed.TrimStartAndEnd());
		OutPinType         = FString();
		OutSubCategoryPath = FString();
	}

	return !OutPinName.IsNone();
}

void UFlowGraphImporter::ApplyDeclaredPins(UFlowNode* NewNode, const FFlowGraphParsedNode& ParsedNode)
{
	FArrayProperty* InputPinsProperty  = FindFProperty<FArrayProperty>(UFlowNode::StaticClass(), TEXT("InputPins"));
	FArrayProperty* OutputPinsProperty = FindFProperty<FArrayProperty>(UFlowNode::StaticClass(), TEXT("OutputPins"));

	auto AddPinIfAbsent = [](FArrayProperty* PinsProperty, UFlowNode* Node, FName PinName, const FString& PinType, const FString& SubCategoryPath)
	{
		if (!PinsProperty)
		{
			return;
		}

		// Skip if the pin is already present (CDO-declared or previously applied).
		FScriptArrayHelper Helper(PinsProperty, PinsProperty->ContainerPtrToValuePtr<void>(Node));
		for (int32 i = 0; i < Helper.Num(); ++i)
		{
			const FFlowPin* Existing = reinterpret_cast<const FFlowPin*>(Helper.GetRawPtr(i));
			if (Existing && Existing->PinName == PinName)
			{
				return;
			}
		}

		FFlowPin* NewPin = reinterpret_cast<FFlowPin*>(Helper.GetRawPtr(Helper.AddValue()));
		NewPin->PinName    = PinName;
		NewPin->PinToolTip = PinName.ToString();
		// InputPins/OutputPins are always exec-pin arrays on UFlowNode, so an omitted type here means
		// Exec, not wildcard (wildcard is only correct for the separate data-pin declaration path).
		const FString EffectiveType = PinType.IsEmpty() ? FString(TEXT("Exec")) : PinType;
		NewPin->SetPinTypeName(FFlowPinTypeName(*EffectiveType));
		if (!SubCategoryPath.IsEmpty())
		{
			// Explicit sub-category path from export - load and apply directly.
			if (UObject* SubCatObject = LoadObject<UObject>(nullptr, *SubCategoryPath))
			{
				NewPin->SetPinSubCategoryObject(SubCatObject);
			}
		}
	};

	for (const FString& PinDecl : ParsedNode.InputPins)
	{
		FName PinName;
		FString PinType;
		FString SubCategoryPath;
		if (ParsePinDecl(PinDecl, PinName, PinType, SubCategoryPath) && !PinName.IsNone())
		{
			// Apply every declared pin, exec included. AddPinIfAbsent already no-ops when the CDO
			// provides the pin; a dynamic-pin class whose exec pins come from reflection (not a
			// literal CDO array) has no such entry, so unconditionally skipping exec pins here
			// silently dropped them for those classes.
			AddPinIfAbsent(InputPinsProperty, NewNode, PinName, PinType, SubCategoryPath);
		}
	}

	for (const FString& PinDecl : ParsedNode.OutputPins)
	{
		FName PinName;
		FString PinType;
		FString SubCategoryPath;
		if (ParsePinDecl(PinDecl, PinName, PinType, SubCategoryPath) && !PinName.IsNone())
		{
			// See the InputPins loop above - same reasoning applies to output exec pins.
			AddPinIfAbsent(OutputPinsProperty, NewNode, PinName, PinType, SubCategoryPath);
		}
	}
}
