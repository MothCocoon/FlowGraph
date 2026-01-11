// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowAsset.h"

#include "FlowLogChannels.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Asset/FlowAssetParams.h"
#include "Asset/FlowAssetParamsUtils.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/Graph/FlowNode_CustomInput.h"
#include "Nodes/Graph/FlowNode_CustomOutput.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"
#include "Types/FlowAutoDataPinsWorkingData.h"
#include "Types/FlowDataPinValue.h"
#include "Types/FlowStructUtils.h"

#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Modules/ModuleManager.h"
#include "SourceControlHelpers.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"

FString UFlowAsset::ValidationError_NodeClassNotAllowed = TEXT("Node class {0} is not allowed in this asset.");
FString UFlowAsset::ValidationError_NullNodeInstance = TEXT("Node with GUID {0} is NULL");
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowAsset)

UFlowAsset::UFlowAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bWorldBound(true)
#if WITH_EDITORONLY_DATA
	, FlowGraph(nullptr)
#endif
	, AllowedNodeClasses({UFlowNodeBase::StaticClass()})
	, AllowedInSubgraphNodeClasses({UFlowNode_SubGraph::StaticClass()})
	, bStartNodePlacedAsGhostNode(false)
	, TemplateAsset(nullptr)
	, FinishPolicy(EFlowFinishPolicy::Keep)
{
	if (!AssetGuid.IsValid())
	{
		AssetGuid = FGuid::NewGuid();
	}

	ExpectedOwnerClass = UFlowSettings::Get()->GetDefaultExpectedOwnerClass();
}

#if WITH_EDITOR
void UFlowAsset::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UFlowAsset* This = CastChecked<UFlowAsset>(InThis);
	Collector.AddReferencedObject(This->FlowGraph, This);

	Super::AddReferencedObjects(InThis, Collector);
}

void UFlowAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomInputs)
		|| PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomOutputs)))
	{
		OnSubGraphReconstructionRequested.ExecuteIfBound();
	}
}

void UFlowAsset::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		AssetGuid = FGuid::NewGuid();
		Nodes.Empty();
	}
}

void UFlowAsset::PostLoad()
{
	Super::PostLoad();

	const UPackage* Package = GetPackage();
	if (IsValid(Package) && !FPackageName::IsTempPackage(Package->GetPathName()))
	{
		// If we removed or moved a flow node blueprint (and there is no redirector) we might loose the reference to it resulting
		// in null pointers in the Nodes FGUID->UFlowNode* Map. So here we iterate over all the Nodes and remove all pairs that
		// are nulled out.

		TSet<FGuid> NodesToRemoveGUID;

		for (const TPair<FGuid, UFlowNode*>& Node : GetNodes())
		{
			if (!IsValid(Node.Value))
			{
				NodesToRemoveGUID.Emplace(Node.Key);
			}
		}

		for (const FGuid& Guid : NodesToRemoveGUID)
		{
			UnregisterNode(Guid);
		}
	
		ReconcileBaseAssetParams(FFlowAssetParamsUtils::GetLastSavedTimestampForObject(this));
	}
}

void UFlowAsset::PreSaveRoot(FObjectPreSaveRootContext ObjectSaveContext)
{
	ReconcileBaseAssetParams(FDateTime::Now());
}

void UFlowAsset::ReconcileBaseAssetParams(const FDateTime& AssetLastSavedTimestamp)
{
	if (BaseAssetParams.AssetPtr.IsNull())
	{
		return;
	}

	UFlowAssetParams* BaseAssetParamsPtr = BaseAssetParams.AssetPtr.LoadSynchronous();
	if (!IsValid(BaseAssetParamsPtr))
	{
		UE_LOG(LogFlow, Error, TEXT("Failed to load BaseAssetParams: %s"), *BaseAssetParams.AssetPtr.ToString());
		return;
	}

	IFlowNamedPropertiesSupplierInterface* NamedPropertiesSupplier = Cast<IFlowNamedPropertiesSupplierInterface>(GetDefaultEntryNode());
	if (!NamedPropertiesSupplier)
	{
		UE_LOG(LogFlow, Error, TEXT("No NamedPropertiesSupplier (e.g., Start node) found in FlowAsset: %s"), *GetPathName());
		return;
	}

	TArray<FFlowNamedDataPinProperty>& MutableStartNodeProperties = NamedPropertiesSupplier->GetMutableNamedProperties();
	const EFlowReconcilePropertiesResult ReconcileResult =
		BaseAssetParamsPtr->ReconcilePropertiesWithStartNode(AssetLastSavedTimestamp, this, MutableStartNodeProperties);

	if (EFlowReconcilePropertiesResult_Classifiers::IsErrorResult(ReconcileResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Failed to reconcile BaseAssetParams for %s: %s"),
			*BaseAssetParamsPtr->GetPathName(), *UEnum::GetDisplayValueAsText(ReconcileResult).ToString());
	}
}

UFlowAssetParams* UFlowAsset::GenerateParamsFromStartNode()
{
	if (BaseAssetParams.AssetPtr.IsValid())
	{
		UE_LOG(LogFlow, Warning, TEXT("BaseAssetParams already exists for %s: %s"), *GetPathName(), *BaseAssetParams.AssetPtr.ToString());
		return BaseAssetParams.AssetPtr.LoadSynchronous();
	}

	// Get the Start node
	IFlowNamedPropertiesSupplierInterface* NamedPropertiesSupplier = Cast<IFlowNamedPropertiesSupplierInterface>(GetDefaultEntryNode());
	if (!NamedPropertiesSupplier)
	{
		UE_LOG(LogFlow, Error, TEXT("No valid Start node found for generating params in %s"), *GetPathName());
		return nullptr;
	}

	// Determine the params asset name
	const FString ParamsAssetName = GenerateParamsAssetName();
	if (ParamsAssetName.IsEmpty())
	{
		UE_LOG(LogFlow, Error, TEXT("Generated empty params asset name for %s"), *GetPathName());
		return nullptr;
	}

	// Create the params asset
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	const FString PackagePath = FPackageName::GetLongPackagePath(GetPackage()->GetPathName());
	FString UniquePackageName, UniqueAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(PackagePath + TEXT("/") + ParamsAssetName, TEXT(""), UniquePackageName, UniqueAssetName);

	UFlowAssetParams* NewParams = Cast<UFlowAssetParams>(
		AssetToolsModule.Get().CreateAsset(UniqueAssetName, PackagePath, UFlowAssetParams::StaticClass(), nullptr));
	if (!IsValid(NewParams))
	{
		UE_LOG(LogFlow, Error, TEXT("Failed to create Flow Asset Params: %s"), *UniqueAssetName);
		return nullptr;
	}

	// Reconfigure with the new properties
	NewParams->ConfigureFlowAssetParams(this, nullptr, NamedPropertiesSupplier->GetMutableNamedProperties());

	// Source control integration
	if (USourceControlHelpers::IsAvailable())
	{
		const FString FileName = USourceControlHelpers::PackageFilename(NewParams->GetPathName());
		if (!USourceControlHelpers::CheckOutOrAddFile(FileName))
		{
			UE_LOG(LogFlow, Warning, TEXT("Failed to check out/add %s; saved in-memory only"), *NewParams->GetPathName());
		}
	}

	// Assign to BaseAssetParams and sync Content Browser
	BaseAssetParams.AssetPtr = NewParams;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().AssetCreated(NewParams);
	
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<UObject*> AssetsToSync = { NewParams };
	ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync, true);

	return NewParams;
}

FString UFlowAsset::GenerateParamsAssetName() const
{
	const FString FlowAssetName = GetName();

	const int32 UnderscoreIndex = FlowAssetName.Find(TEXT("_"), ESearchCase::CaseSensitive);

	if (UnderscoreIndex != INDEX_NONE)
	{
		const FString Prefix = FlowAssetName.Left(UnderscoreIndex);
		const FString Suffix = FlowAssetName.Mid(UnderscoreIndex + 1);
		return FString::Printf(TEXT("%sParams_%s"), *Prefix, *Suffix);
	}
	else
	{
		return FlowAssetName + TEXT("Params");
	}
}

EDataValidationResult UFlowAsset::ValidateAsset(FFlowMessageLog& MessageLog)
{
	// validate nodes
	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (IsValid(Node.Value))
		{
			FText FailureReason;
			if (!IsNodeOrAddOnClassAllowed(Node.Value->GetClass(), &FailureReason))
			{
				const FString ErrorMsg =
					FailureReason.IsEmpty()
						? FString::Format(*ValidationError_NodeClassNotAllowed, {*Node.Value->GetClass()->GetName()})
						: FailureReason.ToString();

				MessageLog.Error(*ErrorMsg, Node.Value);
			}

			Node.Value->ValidationLog.Messages.Empty();
			if (Node.Value->ValidateNode() == EDataValidationResult::Invalid)
			{
				MessageLog.Messages.Append(Node.Value->ValidationLog.Messages);
			}
		}
		else
		{
			const FString ErrorMsg = FString::Format(*ValidationError_NullNodeInstance, {*Node.Key.ToString()});
			MessageLog.Error(*ErrorMsg, this);
		}
	}

	return MessageLog.Messages.Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

bool UFlowAsset::IsNodeOrAddOnClassAllowed(const UClass* FlowNodeOrAddOnClass, FText* OutOptionalFailureReason) const
{
	if (!IsValid(FlowNodeOrAddOnClass))
	{
		return false;
	}

	if (!CanFlowNodeClassBeUsedByFlowAsset(*FlowNodeOrAddOnClass))
	{
		return false;
	}

	if (!CanFlowAssetUseFlowNodeClass(*FlowNodeOrAddOnClass))
	{
		return false;
	}

	// Confirm plugin reference restrictions are being respected
	if (!CanFlowAssetReferenceFlowNode(*FlowNodeOrAddOnClass, OutOptionalFailureReason))
	{
		return false;
	}

	return true;
}

bool UFlowAsset::CanFlowNodeClassBeUsedByFlowAsset(const UClass& FlowNodeClass) const
{
	UFlowNode* NodeDefaults = Cast<UFlowNode>(FlowNodeClass.GetDefaultObject());
	if (!NodeDefaults)
	{
		check(FlowNodeClass.IsChildOf<UFlowNodeAddOn>());

		// AddOns don't have the AllowedAssetClasses/DeniedAssetClasses 
		// (yet?  maybe we move it up to the base?)
		return true;
	}

	// UFlowNode class limits which UFlowAsset class can use it
	const TArray<TSubclassOf<UFlowAsset>>& DeniedAssetClasses = NodeDefaults->DeniedAssetClasses;
	for (const UClass* DeniedAssetClass : DeniedAssetClasses)
	{
		if (DeniedAssetClass && GetClass()->IsChildOf(DeniedAssetClass))
		{
			return false;
		}
	}

	const TArray<TSubclassOf<UFlowAsset>>& AllowedAssetClasses = NodeDefaults->AllowedAssetClasses;
	if (AllowedAssetClasses.Num() > 0)
	{
		bool bAllowedInAsset = false;
		for (const UClass* AllowedAssetClass : AllowedAssetClasses)
		{
			if (AllowedAssetClass && GetClass()->IsChildOf(AllowedAssetClass))
			{
				bAllowedInAsset = true;
				break;
			}
		}
		if (!bAllowedInAsset)
		{
			return false;
		}
	}

	return true;
}

bool UFlowAsset::CanFlowAssetUseFlowNodeClass(const UClass& FlowNodeClass) const
{
	// UFlowAsset class can limit which UFlowNodeBase classes can be used
	if (IsFlowNodeClassInDeniedClasses(FlowNodeClass))
	{
		return false;
	}

	if (!IsFlowNodeClassInAllowedClasses(FlowNodeClass))
	{
		return false;
	}

	return true;
}

bool UFlowAsset::IsFlowNodeClassInDeniedClasses(const UClass& FlowNodeClass) const
{
	for (const TSubclassOf<UFlowNodeBase>& DeniedNodeClass : DeniedNodeClasses)
	{
		if (DeniedNodeClass && FlowNodeClass.IsChildOf(DeniedNodeClass))
		{
			// Subclasses of a DeniedNodeClass can opt back in to being allowed
			if (!IsFlowNodeClassInAllowedClasses(FlowNodeClass, DeniedNodeClass))
			{
				return true;
			}
		}
	}

	return false;
}

bool UFlowAsset::IsFlowNodeClassInAllowedClasses(const UClass& FlowNodeClass,
                                                 const TSubclassOf<UFlowNodeBase>& RequiredAncestor) const
{
	if (AllowedNodeClasses.Num() > 0)
	{
		bool bAllowedInAsset = false;
		for (const TSubclassOf<UFlowNodeBase>& AllowedNodeClass : AllowedNodeClasses)
		{
			// If a RequiredAncestor is provided, the AllowedNodeClass must be a subclass of the RequiredAncestor
			if (AllowedNodeClass && FlowNodeClass.IsChildOf(AllowedNodeClass) && (!RequiredAncestor || AllowedNodeClass->IsChildOf(RequiredAncestor)))
			{
				bAllowedInAsset = true;

				break;
			}
		}

		if (!bAllowedInAsset)
		{
			return false;
		}
	}

	return true;
}

bool UFlowAsset::CanFlowAssetReferenceFlowNode(const UClass& FlowNodeClass, FText* OutOptionalFailureReason) const
{
	if (!GEditor || !IsValid(&FlowNodeClass))
	{
		return false;
	}

	// Confirm plugin reference restrictions are being respected
	FAssetReferenceFilterContext AssetReferenceFilterContext;
	AssetReferenceFilterContext.AddReferencingAsset(FAssetData(this));
	const TSharedPtr<IAssetReferenceFilter> FlowAssetReferenceFilter = GEditor->MakeAssetReferenceFilter(AssetReferenceFilterContext);
	if (FlowAssetReferenceFilter.IsValid())
	{
		const FAssetData FlowNodeAssetData(&FlowNodeClass);
		if (!FlowAssetReferenceFilter->PassesFilter(FlowNodeAssetData, OutOptionalFailureReason))
		{
			return false;
		}
	}

	return true;
}

UFlowNode* UFlowAsset::CreateNode(const UClass* NodeClass, UEdGraphNode* GraphNode)
{
	UFlowNode* NewNode = NewObject<UFlowNode>(this, NodeClass, NAME_None, RF_Transactional);
	NewNode->SetGraphNode(GraphNode);

	RegisterNode(GraphNode->NodeGuid, NewNode);
	return NewNode;
}

void UFlowAsset::RegisterNode(const FGuid& NewGuid, UFlowNode* NewNode)
{
	NewNode->SetGuid(NewGuid);
	Nodes.Emplace(NewGuid, NewNode);

	HarvestNodeConnections();

	if (TryUpdateManagedFlowPinsForNode(*NewNode))
	{
		(void) NewNode->OnReconstructionRequested.ExecuteIfBound();
	}
}

void UFlowAsset::UnregisterNode(const FGuid& NodeGuid)
{
	Nodes.Remove(NodeGuid);
	Nodes.Compact();

	HarvestNodeConnections();

	MarkPackageDirty();
}

void UFlowAsset::HarvestNodeConnections(UFlowNode* TargetNode)
{
	TArray<UFlowNode*> TargetNodes;

	if (IsValid(TargetNode))
	{
		TargetNodes.Reserve(1);
		TargetNodes.Add(TargetNode);
	}
	else
	{
		TargetNodes.Reserve(Nodes.Num());
		for (const TPair<FGuid, UFlowNode*>& Pair : ObjectPtrDecay(Nodes))
		{
			TargetNodes.Add(Pair.Value);
		}
	}

	// Remove any invalid nodes
	for (auto NodeIt = TargetNodes.CreateIterator(); NodeIt; ++NodeIt)
	{
		if (*NodeIt == nullptr)
		{
			NodeIt.RemoveCurrent();
			Modify();
		}
	}

	for (UFlowNode* FlowNode : TargetNodes)
	{
		bool bNodeDirty = false;

		TMap<FName, FConnectedPin> FoundConnections;
		const TArray<UEdGraphPin*>& GraphNodePins = FlowNode->GetGraphNode()->Pins;

		for (const UEdGraphPin* ThisPin : GraphNodePins)
		{
			const bool bIsExecPin = FFlowPin::IsExecPinCategory(ThisPin->PinType.PinCategory);
			const bool bIsDataPin = !bIsExecPin;
			const bool bIsOutputPin = (ThisPin->Direction == EGPD_Output);
			const bool bIsInputPin = (ThisPin->Direction == EGPD_Input);
			const bool bHasAtLeastOneConnection = ThisPin->LinkedTo.Num() > 0;

			if (bIsExecPin && bIsOutputPin && bHasAtLeastOneConnection)
			{
				// For Exec Pins, harvest the 0th connection (we should have only 1 connection, because of schema rules)
				if (const UEdGraphPin* LinkedPin = ThisPin->LinkedTo[0])
				{
					const UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
					FoundConnections.Add(ThisPin->PinName, FConnectedPin(LinkedNode->NodeGuid, LinkedPin->PinName));
				}
			}
			else if (bIsDataPin && bIsInputPin && bHasAtLeastOneConnection)
			{
				// For Data Pins, harvest the 0th connection (we should have only 1 connection, because of schema rules)
				if (const UEdGraphPin* LinkedPin = ThisPin->LinkedTo[0])
				{
					const UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
					FoundConnections.Add(ThisPin->PinName, FConnectedPin(LinkedNode->NodeGuid, LinkedPin->PinName));
				}
			}
		}

		// This check exists to ensure that we don't mark graph dirty, if none of connections changed
		{
			const TMap<FName, FConnectedPin>& OldConnections = FlowNode->Connections;
			if (FoundConnections.Num() != OldConnections.Num())
			{
				bNodeDirty = true;
			}
			else
			{
				for (const TPair<FName, FConnectedPin>& FoundConnection : FoundConnections)
				{
					if (const FConnectedPin* OldConnection = OldConnections.Find(FoundConnection.Key))
					{
						if (FoundConnection.Value != *OldConnection)
						{
							bNodeDirty = true;
							break;
						}
					}
					else
					{
						bNodeDirty = true;
						break;
					}
				}
			}
		}

		if (bNodeDirty)
		{
			FlowNode->SetFlags(RF_Transactional);
			FlowNode->Modify();

			FlowNode->SetConnections(FoundConnections);
			FlowNode->PostEditChange();
		}
	}
}
	
bool UFlowAsset::TryGetDefaultForInputPinName(const FStructProperty& StructProperty, const void* Container, FString& OutString)
{
	// We also look in the USTRUCT for DefaultForInputFlowPin
	const FString* DefaultForInputFlowPinName = StructProperty.Struct->FindMetaData(FFlowPin::MetadataKey_DefaultForInputFlowPin);

	if (DefaultForInputFlowPinName)
	{
		OutString = *DefaultForInputFlowPinName;

		return true;
	}

	// For blueprint use, we allow the Value structs to set input pins via editor-only data

	const FFlowDataPinValue* DataPinValue = FlowStructUtils::CastStructValue<FFlowDataPinValue>(StructProperty, Container);
	if (DataPinValue && DataPinValue->IsInputPin())
	{
		OutString.Empty();

		return true;
	}

	return false;
}

bool UFlowAsset::TryUpdateManagedFlowPinsForNode(UFlowNode& FlowNode)
{
	// Set up the working data struct
	FFlowAutoDataPinsWorkingData WorkingData =
		FFlowAutoDataPinsWorkingData(
			FlowNode.GetAutoInputDataPins(),
			FlowNode.GetAutoOutputDataPins());

	// Allow the node to auto-generate data pins
	FlowNode.AutoGenerateDataPins(WorkingData);

	// If the auto-generated data pins array changed, it counts as dirty as well
	const bool bAutoInputDataPinsChanged = WorkingData.DidAutoInputDataPinsChange();
	const bool bAutoOutputDataPinsChanged = WorkingData.DidAutoOutputDataPinsChange();

	if (bAutoInputDataPinsChanged || bAutoOutputDataPinsChanged)
	{
		FlowNode.SetFlags(RF_Transactional);
		FlowNode.Modify();

		// Lock-in the data that changed.
		if (bAutoInputDataPinsChanged || bAutoOutputDataPinsChanged)
		{
			if (bAutoInputDataPinsChanged)
			{
				FlowNode.SetAutoInputDataPins(WorkingData.AutoInputDataPinsNext);
			}

			if (bAutoOutputDataPinsChanged)
			{
				FlowNode.SetAutoOutputDataPins(WorkingData.AutoOutputDataPinsNext);
			}
		}

		FlowNode.PostEditChange();

		return true;
	}

	return false;
}

#endif

UFlowNode* UFlowAsset::GetDefaultEntryNode() const
{
	UFlowNode* FirstStartNode = nullptr;

	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (UFlowNode_Start* StartNode = Cast<UFlowNode_Start>(Node.Value))
		{
			if (StartNode->GatherConnectedNodes().Num() > 0)
			{
				return StartNode;
			}
			else if (FirstStartNode == nullptr)
			{
				FirstStartNode = StartNode;
			}
		}
	}

	// If none of the found start nodes have connections, fallback to the first start node we found
	return FirstStartNode;
}

#if WITH_EDITOR
void UFlowAsset::AddCustomInput(const FName& EventName)
{
	if (!CustomInputs.Contains(EventName))
	{
		CustomInputs.Add(EventName);
	}
}

void UFlowAsset::RemoveCustomInput(const FName& EventName)
{
	if (CustomInputs.Contains(EventName))
	{
		CustomInputs.Remove(EventName);
	}
}

void UFlowAsset::AddCustomOutput(const FName& EventName)
{
	if (!CustomOutputs.Contains(EventName))
	{
		CustomOutputs.Add(EventName);
	}
}

void UFlowAsset::RemoveCustomOutput(const FName& EventName)
{
	if (CustomOutputs.Contains(EventName))
	{
		CustomOutputs.Remove(EventName);
	}
}
#endif // WITH_EDITOR

UFlowNode_CustomInput* UFlowAsset::TryFindCustomInputNodeByEventName(const FName& EventName) const
{
	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (UFlowNode_CustomInput* CustomInput = Cast<UFlowNode_CustomInput>(Node.Value))
		{
			if (CustomInput->GetEventName() == EventName)
			{
				return CustomInput;
			}
		}
	}

	return nullptr;
}

UFlowNode_CustomOutput* UFlowAsset::TryFindCustomOutputNodeByEventName(const FName& EventName) const
{
	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (UFlowNode_CustomOutput* CustomOutput = Cast<UFlowNode_CustomOutput>(Node.Value))
		{
			if (CustomOutput->GetEventName() == EventName)
			{
				return CustomOutput;
			}
		}
	}

	return nullptr;
}

TArray<FName> UFlowAsset::GatherCustomInputNodeEventNames() const
{
	// Runtime-safe gathering of the CustomInputs (which is editor-only data)
	//  from the actual flow nodes
	TArray<FName> Results;

	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (UFlowNode_CustomInput* CustomInput = Cast<UFlowNode_CustomInput>(Node.Value))
		{
			Results.Add(CustomInput->GetEventName());
		}
	}

	return Results;
}

TArray<FName> UFlowAsset::GatherCustomOutputNodeEventNames() const
{
	// Runtime-safe gathering of the CustomOutputs (which is editor-only data)
	//  from the actual flow nodes
	TArray<FName> Results;

	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (UFlowNode_CustomOutput* CustomOutput = Cast<UFlowNode_CustomOutput>(Node.Value))
		{
			Results.Add(CustomOutput->GetEventName());
		}
	}

	return Results;
}

TArray<UFlowNode*> UFlowAsset::GetNodesInExecutionOrder(UFlowNode* FirstIteratedNode, const TSubclassOf<UFlowNode> FlowNodeClass)
{
	TArray<UFlowNode*> FoundNodes;
	GetNodesInExecutionOrder<UFlowNode>(FirstIteratedNode, FoundNodes);

	// filter out nodes by class
	for (int32 i = FoundNodes.Num() - 1; i >= 0; i--)
	{
		if (!FoundNodes[i]->GetClass()->IsChildOf(FlowNodeClass))
		{
			FoundNodes.RemoveAt(i);
		}
	}
	FoundNodes.Shrink();

	return FoundNodes;
}

TArray<UFlowNode*> UFlowAsset::GatherNodesConnectedToAllInputs() const
{
	TSet<TObjectKey<UFlowNode>> IteratedNodes;
	TArray<UFlowNode*> ConnectedNodes;

	// Nodes connected to the Start node
	UFlowNode* DefaultEntryNode = GetDefaultEntryNode();
	GetNodesInExecutionOrder_Recursive(DefaultEntryNode, IteratedNodes, ConnectedNodes);

	// Nodes connected to Custom Input node(s)
	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (UFlowNode_CustomInput* CustomInput = Cast<UFlowNode_CustomInput>(Node.Value))
		{
			GetNodesInExecutionOrder_Recursive(CustomInput, IteratedNodes, ConnectedNodes);
		}
	}

	return ConnectedNodes;
}

TArray<FConnectedPin> UFlowAsset::GatherPinsConnectedToPin(const FConnectedPin& Pin) const
{
	TArray<FConnectedPin> ConnectedPins;

	// Connections are only stored on one of the Nodes they connect depending on pin type.
	// As such, we need to iterate all Nodes to find all possible Connections for the Pin.
	for (const auto& GuidNodePair : Nodes)
	{
		if (IsValid(GuidNodePair.Value))
		{
			ConnectedPins.Append(GuidNodePair.Value->GetKnownConnectionsToPin(Pin));
		}
	}
	
	return ConnectedPins;
}

TArray<UFlowNode*> UFlowAsset::GetAllNodes() const
{
	TArray<TObjectPtr<UFlowNode>> AllNodes;
	AllNodes.Reserve(Nodes.Num());
	Nodes.GenerateValueArray(AllNodes);

	return ObjectPtrDecay(AllNodes);
}

void UFlowAsset::AddInstance(UFlowAsset* Instance)
{
	ActiveInstances.Add(Instance);
}

int32 UFlowAsset::RemoveInstance(UFlowAsset* Instance)
{
#if WITH_EDITOR
	if (InspectedInstance.IsValid() && InspectedInstance.Get() == Instance)
	{
		SetInspectedInstance(NAME_None);
	}
#endif

	ActiveInstances.Remove(Instance);
	return ActiveInstances.Num();
}

void UFlowAsset::ClearInstances()
{
#if WITH_EDITOR
	if (InspectedInstance.IsValid())
	{
		SetInspectedInstance(NAME_None);
	}
#endif

	for (int32 i = ActiveInstances.Num() - 1; i >= 0; i--)
	{
		if (ActiveInstances.IsValidIndex(i) && ActiveInstances[i])
		{
			ActiveInstances[i]->FinishFlow(EFlowFinishPolicy::Keep);
		}
	}

	ActiveInstances.Empty();
}

#if WITH_EDITOR
void UFlowAsset::GetInstanceDisplayNames(TArray<TSharedPtr<FName>>& OutDisplayNames) const
{
	for (const UFlowAsset* Instance : ActiveInstances)
	{
		OutDisplayNames.Emplace(MakeShareable(new FName(Instance->GetDisplayName())));
	}
}

void UFlowAsset::SetInspectedInstance(const FName& NewInspectedInstanceName)
{
	if (NewInspectedInstanceName.IsNone())
	{
		InspectedInstance = nullptr;
	}
	else
	{
		for (UFlowAsset* ActiveInstance : ActiveInstances)
		{
			if (ActiveInstance && ActiveInstance->GetDisplayName() == NewInspectedInstanceName)
			{
				if (!InspectedInstance.IsValid() || InspectedInstance != ActiveInstance)
				{
					InspectedInstance = ActiveInstance;
				}
				break;
			}
		}
	}

	BroadcastDebuggerRefresh();
}

void UFlowAsset::BroadcastDebuggerRefresh() const
{
	RefreshDebuggerEvent.Broadcast();
}

void UFlowAsset::BroadcastRuntimeMessageAdded(const TSharedRef<FTokenizedMessage>& Message) const
{
	RuntimeMessageEvent.Broadcast(this, Message);
}
#endif // WITH_EDITOR

void UFlowAsset::InitializeInstance(const TWeakObjectPtr<UObject> InOwner, UFlowAsset& InTemplateAsset)
{
	check(!IsInstanceInitialized());

	Owner = InOwner;
	TemplateAsset = &InTemplateAsset;

	for (TPair<FGuid, TObjectPtr<UFlowNode>>& Node : Nodes)
	{
		UFlowNode* NewNodeInstance = NewObject<UFlowNode>(this, Node.Value->GetClass(), NAME_None, RF_Transient, Node.Value, false, nullptr);
		Node.Value = NewNodeInstance;

		if (UFlowNode_CustomInput* CustomInput = Cast<UFlowNode_CustomInput>(NewNodeInstance))
		{
			if (!CustomInput->EventName.IsNone())
			{
				CustomInputNodes.Emplace(CustomInput);
			}
		}

		NewNodeInstance->InitializeInstance();
	}
}

void UFlowAsset::DeinitializeInstance()
{
	if (IsInstanceInitialized())
	{
		for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
		{
			if (IsValid(Node.Value))
			{
				Node.Value->DeinitializeInstance();
			}
		}

		const int32 ActiveInstancesLeft = TemplateAsset->RemoveInstance(this);
		if (ActiveInstancesLeft == 0 && GetFlowSubsystem())
		{
			GetFlowSubsystem()->RemoveInstancedTemplate(TemplateAsset);
		}

		TemplateAsset = nullptr;
	}
}

void UFlowAsset::PreStartFlow()
{
	ResetNodes();

#if WITH_EDITOR
	check(IsInstanceInitialized());

	if (TemplateAsset->ActiveInstances.Num() == 1)
	{
		// this instance is the only active one, set it directly as Inspected Instance
		TemplateAsset->SetInspectedInstance(GetDisplayName());
	}
	else
	{
		// request to refresh list to show newly created instance
		TemplateAsset->BroadcastDebuggerRefresh();
	}
#endif
}

void UFlowAsset::StartFlow(IFlowDataPinValueSupplierInterface* DataPinValueSupplier)
{
	PreStartFlow();

	if (UFlowNode* ConnectedEntryNode = GetDefaultEntryNode())
	{
		RecordedNodes.Add(ConnectedEntryNode);

		if (IFlowNodeWithExternalDataPinSupplierInterface* ExternalPinSuppliedNode = Cast<IFlowNodeWithExternalDataPinSupplierInterface>(ConnectedEntryNode))
		{
			ExternalPinSuppliedNode->SetDataPinValueSupplier(DataPinValueSupplier);
		}

		ConnectedEntryNode->TriggerFirstOutput(true);
	}
}

void UFlowAsset::FinishFlow(const EFlowFinishPolicy InFinishPolicy, const bool bRemoveInstance /*= true*/)
{
	FinishPolicy = InFinishPolicy;

	// end execution of this asset and all of its nodes
	for (UFlowNode* Node : ActiveNodes)
	{
		Node->Deactivate();
	}
	ActiveNodes.Empty();

	// flush preloaded content
	for (UFlowNode* PreloadedNode : PreloadedNodes)
	{
		PreloadedNode->TriggerFlush();
	}
	PreloadedNodes.Empty();

	// provides option to finish game-specific logic prior to removing asset instance 
	if (bRemoveInstance)
	{
		DeinitializeInstance();
	}
}

bool UFlowAsset::HasStartedFlow() const
{
	return RecordedNodes.Num() > 0;
}

AActor* UFlowAsset::TryFindActorOwner() const
{
	const UActorComponent* OwnerAsComponent = Cast<UActorComponent>(GetOwner());
	if (IsValid(OwnerAsComponent))
	{
		return Cast<AActor>(OwnerAsComponent->GetOwner());
	}

	return nullptr;
}

TWeakObjectPtr<UFlowAsset> UFlowAsset::GetFlowInstance(UFlowNode_SubGraph* SubGraphNode) const
{
	return ActiveSubGraphs.FindRef(SubGraphNode);
}

void UFlowAsset::TriggerCustomInput_FromSubGraph(UFlowNode_SubGraph* SubGraphNode, const FName& EventName) const
{
	// NOTE (gtaylor) Custom Input nodes cannot currently add data pins (like Start or DefineProperties nodes can)
	// but we may want to allow them to source parameters, so I am providing the subgraph node as the 
	// IFlowDataPinValueSupplierInterface when triggering the node (even though it's not used at this time).

	const TWeakObjectPtr<UFlowAsset> FlowInstance = ActiveSubGraphs.FindRef(SubGraphNode);
	if (FlowInstance.IsValid())
	{
		FlowInstance->TriggerCustomInput(EventName, SubGraphNode);
	}
}

void UFlowAsset::TriggerCustomInput(const FName& EventName, IFlowDataPinValueSupplierInterface* DataPinValueSupplier)
{
	for (UFlowNode_CustomInput* CustomInputNode : CustomInputNodes)
	{
		if (CustomInputNode->EventName == EventName)
		{
			RecordedNodes.Add(CustomInputNode);

			// NOTE (gtaylor) Custom Input nodes cannot currently add data pins (like Start or DefineProperties nodes can)
			// but we may want to allow them to source parameters, so I am providing the subgraph node as the 
			// IFlowDataPinValueSupplierInterface when triggering the node (even though it's not used at this time).

			if (IFlowNodeWithExternalDataPinSupplierInterface* ExternalPinSuppliedNode = Cast<IFlowNodeWithExternalDataPinSupplierInterface>(CustomInputNode))
			{
				ExternalPinSuppliedNode->SetDataPinValueSupplier(DataPinValueSupplier);
			}

			CustomInputNode->ExecuteInput(EventName);
		}
	}
}

void UFlowAsset::TriggerCustomOutput(const FName& EventName)
{
	if (NodeOwningThisAssetInstance.IsValid())
	{
		// it's a SubGraph
		NodeOwningThisAssetInstance->TriggerOutput(EventName);
	}
	else
	{
		// it's a Root Flow, so the intention here might be to call event on the Flow Component
		if (UFlowComponent* FlowComponent = Cast<UFlowComponent>(GetOwner()))
		{
			FlowComponent->DispatchRootFlowCustomEvent(this, EventName);
		}
	}
}

void UFlowAsset::TriggerInput(const FGuid& NodeGuid, const FName& PinName, const FConnectedPin& FromPin)
{
	if (UFlowNode* Node = Nodes.FindRef(NodeGuid))
	{
		if (!ActiveNodes.Contains(Node))
		{
			ActiveNodes.Add(Node);
			RecordedNodes.Add(Node);
		}

		Node->TriggerInput(PinName);
	}
}

void UFlowAsset::FinishNode(UFlowNode* Node)
{
	if (ActiveNodes.Contains(Node))
	{
		ActiveNodes.Remove(Node);

		// if graph reached Finish and this asset instance was created by SubGraph node
		if (Node->CanFinishGraph())
		{
			if (NodeOwningThisAssetInstance.IsValid())
			{
				NodeOwningThisAssetInstance.Get()->TriggerFirstOutput(true);

				return;
			}

			// if this instance is a Root Flow, we need to deregister it from the subsystem first
			if (Owner.IsValid())
			{
				const TSet<UFlowAsset*>& RootFlowInstances = GetFlowSubsystem()->GetRootInstancesByOwner(Owner.Get());
				if (RootFlowInstances.Contains(this))
				{
					GetFlowSubsystem()->FinishRootFlow(Owner.Get(), TemplateAsset, EFlowFinishPolicy::Keep);

					return;
				}
			}

			FinishFlow(EFlowFinishPolicy::Keep);
		}
	}
}

void UFlowAsset::ResetNodes()
{
	for (UFlowNode* Node : RecordedNodes)
	{
		Node->ResetRecords();
	}

	RecordedNodes.Empty();
}

UFlowSubsystem* UFlowAsset::GetFlowSubsystem() const
{
	return Cast<UFlowSubsystem>(GetOuter());
}

FName UFlowAsset::GetDisplayName() const
{
	return GetFName();
}

UFlowNode_SubGraph* UFlowAsset::GetNodeOwningThisAssetInstance() const
{
	return NodeOwningThisAssetInstance.Get();
}

UFlowAsset* UFlowAsset::GetParentInstance() const
{
	return NodeOwningThisAssetInstance.IsValid() ? NodeOwningThisAssetInstance.Get()->GetFlowAsset() : nullptr;
}

FFlowAssetSaveData UFlowAsset::SaveInstance(TArray<FFlowAssetSaveData>& SavedFlowInstances)
{
	FFlowAssetSaveData AssetRecord;
	AssetRecord.WorldName = IsBoundToWorld() ? GetWorld()->GetName() : FString();
	AssetRecord.InstanceName = GetName();

	// opportunity to collect data before serializing asset
	OnSave();

	// iterate nodes
	TArray<UFlowNode*> NodesInExecutionOrder;
	GetNodesInExecutionOrder<UFlowNode>(GetDefaultEntryNode(), NodesInExecutionOrder);
	for (UFlowNode* Node : NodesInExecutionOrder)
	{
		if (Node && Node->ActivationState == EFlowNodeState::Active)
		{
			// iterate SubGraphs
			if (UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(Node))
			{
				const TWeakObjectPtr<UFlowAsset> SubFlowInstance = GetFlowInstance(SubGraphNode);
				if (SubFlowInstance.IsValid())
				{
					const FFlowAssetSaveData SubAssetRecord = SubFlowInstance->SaveInstance(SavedFlowInstances);
					SubGraphNode->SavedAssetInstanceName = SubAssetRecord.InstanceName;
				}
			}

			FFlowNodeSaveData NodeRecord;
			Node->SaveInstance(NodeRecord);

			AssetRecord.NodeRecords.Emplace(NodeRecord);
		}
	}

	// serialize asset
	FMemoryWriter MemoryWriter(AssetRecord.AssetData, true);
	FFlowArchive Ar(MemoryWriter);
	Serialize(Ar);

	// write archive to SaveGame
	SavedFlowInstances.Emplace(AssetRecord);

	return AssetRecord;
}

void UFlowAsset::LoadInstance(const FFlowAssetSaveData& AssetRecord)
{
	FMemoryReader MemoryReader(AssetRecord.AssetData, true);
	FFlowArchive Ar(MemoryReader);
	Serialize(Ar);

	PreStartFlow();

	// iterate graph "from the end", backward to execution order
	// prevents issue when the preceding node would instantly fire output to a not-yet-loaded node
	for (int32 i = AssetRecord.NodeRecords.Num() - 1; i >= 0; i--)
	{
		if (UFlowNode* Node = Nodes.FindRef(AssetRecord.NodeRecords[i].NodeGuid))
		{
			Node->LoadInstance(AssetRecord.NodeRecords[i]);
		}
	}

	OnLoad();
}

void UFlowAsset::OnActivationStateLoaded(UFlowNode* Node)
{
	if (Node->ActivationState != EFlowNodeState::NeverActivated)
	{
		RecordedNodes.Emplace(Node);
	}

	if (Node->ActivationState == EFlowNodeState::Active)
	{
		ActiveNodes.Emplace(Node);
	}
}

void UFlowAsset::OnSave_Implementation()
{
}

void UFlowAsset::OnLoad_Implementation()
{
}

bool UFlowAsset::IsBoundToWorld_Implementation()
{
	return bWorldBound;
}

#if WITH_EDITOR
void UFlowAsset::LogError(const FString& MessageToLog, const UFlowNodeBase* Node) const
{
	// this is runtime log which is should be only called on runtime instances of asset
	if (TemplateAsset)
	{
		UE_LOG(LogFlow, Log, TEXT("Attempted to use Runtime Log on asset instance %s"), *MessageToLog);
	}

	if (RuntimeLog.Get())
	{
		const TSharedRef<FTokenizedMessage> TokenizedMessage = RuntimeLog.Get()->Error(*MessageToLog, Node);
		BroadcastRuntimeMessageAdded(TokenizedMessage);
	}
}

void UFlowAsset::LogWarning(const FString& MessageToLog, const UFlowNodeBase* Node) const
{
	// this is runtime log which is should be only called on runtime instances of asset
	if (TemplateAsset)
	{
		UE_LOG(LogFlow, Log, TEXT("Attempted to use Runtime Log on asset instance %s"), *MessageToLog);
	}

	if (RuntimeLog.Get())
	{
		const TSharedRef<FTokenizedMessage> TokenizedMessage = RuntimeLog.Get()->Warning(*MessageToLog, Node);
		BroadcastRuntimeMessageAdded(TokenizedMessage);
	}
}

void UFlowAsset::LogNote(const FString& MessageToLog, const UFlowNodeBase* Node) const
{
	// this is runtime log which is should be only called on runtime instances of asset
	if (TemplateAsset)
	{
		UE_LOG(LogFlow, Log, TEXT("Attempted to use Runtime Log on asset instance %s"), *MessageToLog);
	}

	if (RuntimeLog.Get())
	{
		const TSharedRef<FTokenizedMessage> TokenizedMessage = RuntimeLog.Get()->Note(*MessageToLog, Node);
		BroadcastRuntimeMessageAdded(TokenizedMessage);
	}
}
#endif
