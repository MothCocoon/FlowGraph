// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "FlowAsset.h"

#include "FlowLogChannels.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"
#include "Types/FlowInjectComponentsManager.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Asset/FlowAssetParams.h"
#include "Asset/FlowAssetParamsUtils.h"
#include "Interfaces/FlowGraphOutputDataReceiverInterface.h"
#include "Types/FlowNamedDataPinProperty.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/Graph/FlowNode_CustomInput.h"
#include "Nodes/Graph/FlowNode_CustomOutput.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"
#include "Policies/FlowPinConnectionPolicy.h"
#include "Policies/FlowPreloadPolicy.h"
#include "Types/FlowDataPinValue.h"
#include "Types/FlowStructUtils.h"

#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Algo/AnyOf.h"

#if WITH_EDITOR
#include "Nodes/Graph/FlowNode_SetGraphOutput.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "Misc/DataValidation.h"
#include "IContentBrowserSingleton.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Modules/ModuleManager.h"
#include "SourceControlHelpers.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"

FString UFlowAsset::ValidationError_NodeClassNotAllowed = TEXT("Node class {0} is not allowed in this asset.");
FString UFlowAsset::ValidationError_AddOnNodeClassNotAllowed = TEXT("AddOn Node class {0} is not allowed in this asset.");
FString UFlowAsset::ValidationError_NullNodeInstance = TEXT("Node with GUID {0} is NULL");
FString UFlowAsset::ValidationError_NullAddOnNodeInstance = TEXT("Node with GUID {0} has NULL AddOn(s)");
FString UFlowAsset::ValidationError_ConnectedToMissingNode = TEXT("Pin {0} on Node with GUID {1} is connected to Node with Guid {2}, which does not exist in {3}. {4}");
FString UFlowAsset::ValidationError_AdviseResave = TEXT("Open the Flow Asset in editor and resave it.");
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowAsset)

UFlowAsset::UFlowAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, AllowedNodeClasses({UFlowNodeBase::StaticClass()})
	, AllowedInSubgraphNodeClasses({UFlowNode_SubGraph::StaticClass()})
{
	if (!AssetGuid.IsValid())
	{
		AssetGuid = FGuid::NewGuid();
	}

	ExpectedOwnerClass = GetDefault<UFlowSettings>()->GetDefaultExpectedOwnerClass();
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

	const FName ChangedPropertyName = PropertyChangedEvent.GetPropertyName();
	const FName ChangedMemberPropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropertyChangedEvent.Property && (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomInputs)
		|| ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomOutputs)
		|| ChangedMemberPropertyName == GET_MEMBER_NAME_CHECKED(UFlowAsset, OutputDataPinDeclarations)))
	{
		OnSubGraphReconstructionRequested.ExecuteIfBound();
	}

	if (PropertyChangedEvent.Property && ChangedMemberPropertyName == GET_MEMBER_NAME_CHECKED(UFlowAsset, OutputDataPinDeclarations))
	{
		for (const TPair<FGuid, UFlowNode*>& NodePair : GetNodes())
		{
			UFlowNode_SetGraphOutput* SetOutputNode = Cast<UFlowNode_SetGraphOutput>(NodePair.Value);
			if (IsValid(SetOutputNode) && SetOutputNode->TryUpdateAutoDataPins())
			{
				SetOutputNode->OnReconstructionRequested.ExecuteIfBound();
			}
		}
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
		// If we removed or moved a flow node blueprint (and there is no redirector) we might lose the reference to it resulting
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
	(void)RepairDuplicateAddOnGuids();
}

void UFlowAsset::SetupForEditing()
{
	InitializePinConnectionPolicy();

	// Initialize any customizable Policies before we instantiate nodes
	InitializePreloadPolicy();
}

bool UFlowAsset::IsNodeOrAddOnClassAllowed(const UClass* FlowNodeOrAddOnClass, FText* OutOptionalFailureReason) const
{
	if (!IsValid(FlowNodeOrAddOnClass))
	{
		return false;
	}

	if (FlowNodeOrAddOnClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		if (OutOptionalFailureReason)
		{
			*OutOptionalFailureReason = FText::Format(
				NSLOCTEXT("FlowAsset", "UnsaveableNodeClass",
					"Class '{0}' cannot be saved because it is abstract, deprecated, or superseded. "
					"Replace this node or add-on before cooking; its saved reference would be null."),
				FText::FromString(FlowNodeOrAddOnClass->GetPathName()));
		}
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
	const UFlowNode* NodeDefaults = Cast<UFlowNode>(FlowNodeClass.GetDefaultObject());
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

bool UFlowAsset::IsFlowNodeClassInAllowedClasses(const UClass& FlowNodeClass, const TSubclassOf<UFlowNodeBase>& RequiredAncestor) const
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

bool UFlowAsset::ShouldValidateOnSave() const
{
	return GetDefault<UFlowSettings>()->bValidateAssetOnSave;
}

EDataValidationResult UFlowAsset::IsDataValid(FDataValidationContext& Context) const
{
	FFlowMessageLog LogResults;
	const EDataValidationResult Result = ValidateAsset(LogResults);

	for (const TSharedRef<FTokenizedMessage>& Message : LogResults.Messages)
	{
		Context.AddMessage(Message);
	}

	return Result;
}

EDataValidationResult UFlowAsset::ValidateAsset(FFlowMessageLog& MessageLog) const
{
	// A node may report Invalid with warnings only or no messages
	EDataValidationResult NodesResult = EDataValidationResult::NotValidated;

	// validate runtime nodes
	for (const TPair<FGuid, UFlowNode*>& NodeIt : ObjectPtrDecay(Nodes))
	{
		UFlowNode* Node = NodeIt.Value;

		if (IsValid(Node))
		{
			// Check if this node class is still allowed in this asset class
			FText FailureReason;
			if (!IsNodeOrAddOnClassAllowed(Node->GetClass(), &FailureReason))
			{
				const FString ErrorMsg = !FailureReason.IsEmpty() ? FailureReason.ToString()
					: FString::Format(*ValidationError_NodeClassNotAllowed, {*Node->GetClass()->GetName()});
				MessageLog.Error(*ErrorMsg, Node);
			}

			// Validate node connections
			for (const TPair<FName, FConnectedPin>& Connection : Node->Connections)
			{
				if (!Nodes.Contains(Connection.Value.NodeGuid))
				{
					const FString ErrorMsg = FString::Format(*ValidationError_ConnectedToMissingNode, 
						{*Connection.Key.ToString(), *NodeIt.Key.ToString(), *Connection.Value.NodeGuid.ToString(), GetPathName(), ValidationError_AdviseResave});

					MessageLog.Error(*ErrorMsg, Node);
				}
			}

			// Validate node itself
			NodesResult = CombineDataValidationResults(NodesResult, ValidateNode(*Node, MessageLog));

			// Validate node AddOns
			NodesResult = CombineDataValidationResults(NodesResult, ValidateAddOnChildren(*Node, MessageLog));
		}
		else
		{
			const FString ErrorMsg = FString::Format(*ValidationError_NullNodeInstance, {*NodeIt.Key.ToString()});
			MessageLog.Error(*ErrorMsg, this);
		}
	}

	// validate editor's graph
	OnValidateGraph.ExecuteIfBound(MessageLog);

	// if at least one error has been logged : mark the asset as invalid
	for (const TSharedRef<FTokenizedMessage>& Msg : MessageLog.Messages)
	{
		if (Msg->GetSeverity() == EMessageSeverity::Error)
		{
			return EDataValidationResult::Invalid;
		}
	}

	if (NodesResult == EDataValidationResult::Invalid)
	{
		return EDataValidationResult::Invalid;
	}

	// otherwise, the asset is considered valid (even with warnings or notes)
	return EDataValidationResult::Valid;
}

EDataValidationResult UFlowAsset::ValidateNode(UFlowNodeBase& FlowNodeBase, FFlowMessageLog& MessageLog)
{
	FlowNodeBase.ValidationLog.Messages.Empty();
	const EDataValidationResult NodeResult = FlowNodeBase.ValidateNode();
	MessageLog.Messages.Append(FlowNodeBase.ValidationLog.Messages);

	return NodeResult;
}

EDataValidationResult UFlowAsset::ValidateAddOnChildren(UFlowNodeBase& OwnerNode, FFlowMessageLog& MessageLog) const
{
	EDataValidationResult ChildrenResult = EDataValidationResult::NotValidated;

	const TArray<UFlowNodeAddOn*>& Children = OwnerNode.GetFlowNodeAddOnChildren();
	for (UFlowNodeAddOn* Child : Children)
	{
		if (IsValid(Child))
		{
			TArray<UFlowNodeAddOn*> OtherChildren;
			OtherChildren.Reserve(Children.Num() - 1);
			for (UFlowNodeAddOn* OtherChild : Children)
			{
				if (IsValid(OtherChild) && OtherChild != Child)
				{
					OtherChildren.Add(OtherChild);
				}
			}

			if (OwnerNode.CheckAcceptFlowNodeAddOnChild(Child, OtherChildren) == EFlowAddOnAcceptResult::Reject)
			{
				const FString ErrorMsg = FString::Printf(
					TEXT("AddOn '%s' is not a valid child of '%s' with its current sibling set."),
					*Child->GetClass()->GetName(),
					*OwnerNode.GetClass()->GetName());
				MessageLog.Error(*ErrorMsg, &OwnerNode);
			}

			ChildrenResult = CombineDataValidationResults(ChildrenResult, ValidateAddOnTree(*Child, MessageLog));
		}
		else
		{
			const FString ErrorMsg = FString::Format(*ValidationError_NullAddOnNodeInstance, {*OwnerNode.GetGuid().ToString()});
			MessageLog.Error(*ErrorMsg, this);
		}
	}

	return ChildrenResult;
}

EDataValidationResult UFlowAsset::ValidateAddOnTree(UFlowNodeAddOn& AddOn, FFlowMessageLog& MessageLog) const
{
	// Filter unauthorized addon nodes
	FText FailureReason;
	if (!IsNodeOrAddOnClassAllowed(AddOn.GetClass(), &FailureReason))
	{
		const FString ErrorMsg =
			FailureReason.IsEmpty()
				? FString::Format(*ValidationError_AddOnNodeClassNotAllowed, {*AddOn.GetClass()->GetName()})
				: FailureReason.ToString();

		MessageLog.Error(*ErrorMsg, AddOn.GetFlowNodeSelfOrOwner());
	}

	// Validate AddOn
	const EDataValidationResult AddOnResult = ValidateNode(AddOn, MessageLog);

	return CombineDataValidationResults(AddOnResult, ValidateAddOnChildren(AddOn, MessageLog));
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

	HarvestNodeConnections(NewNode);

	if (NewNode->TryUpdateAutoDataPins())
	{
		(void)NewNode->OnReconstructionRequested.ExecuteIfBound();
	}
}

void UFlowAsset::UnregisterNode(const FGuid& NodeGuid)
{
	Nodes.Remove(NodeGuid);
	Nodes.Compact();

	HarvestNodeConnections();

	(void)MarkPackageDirty();
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
		const UEdGraphNode* EdGraphNode = FlowNode->GetGraphNode();

		if (!IsValid(EdGraphNode))
		{
			// A node may exist in Nodes map without a GraphNode if the importer has populated
			// the asset before RegraphFlowAsset has finished linking runtime nodes to their
			// editor counterparts.
			// Such nodes have no pins to harvest yet. Skip them.
			continue;
		}

		const TArray<UEdGraphPin*>& GraphNodePins = EdGraphNode->Pins;

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

#endif

TArray<UFlowNode*> UFlowAsset::GetAllNodes() const
{
	TArray<TObjectPtr<UFlowNode>> AllNodes;
	AllNodes.Reserve(Nodes.Num());
	Nodes.GenerateValueArray(AllNodes);

	return ObjectPtrDecay(AllNodes);
}

TArray<UFlowNode*> UFlowAsset::GetNodesInExecutionOrder(UFlowNode* FirstIteratedNode, const TSubclassOf<UFlowNode> FlowNodeClass) const
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

int32 UFlowAsset::RepairDuplicateAddOnGuids() const
{
	TSet<FGuid> SeenAddOnGuids;
	int32 NumRepaired = 0;

	TFunction<void(UFlowNodeAddOn&)> RepairAddOnTree = [this, &SeenAddOnGuids, &NumRepaired, &RepairAddOnTree](UFlowNodeAddOn& AddOn)
	{
		bool bAlreadyInSet = false;
		SeenAddOnGuids.Add(AddOn.GetGuid(), &bAlreadyInSet);

		if (bAlreadyInSet)
		{
			const FGuid OldGuid = AddOn.GetGuid();
			const FGuid NewGuid = FGuid::NewGuid();
			AddOn.SetGuid(NewGuid);
			SeenAddOnGuids.Add(NewGuid);
			++NumRepaired;

			UE_LOG(LogFlow, Log, TEXT("RepairDuplicateAddOnGuids: %s AddOn %s had duplicate Guid %s, re-minted to %s"),
				*GetPathName(), *AddOn.GetName(), *OldGuid.ToString(), *NewGuid.ToString());
		}

		for (UFlowNodeAddOn* Child : AddOn.GetFlowNodeAddOnChildren())
		{
			if (IsValid(Child))
			{
				RepairAddOnTree(*Child);
			}
		}
	};

	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (!IsValid(Node.Value))
		{
			continue;
		}

		for (UFlowNodeAddOn* AddOn : Node.Value->GetFlowNodeAddOnChildren())
		{
			if (IsValid(AddOn))
			{
				RepairAddOnTree(*AddOn);
			}
		}
	}

	return NumRepaired;
}

TArray<UFlowNode*> UFlowAsset::GatherNodesConnectedToAllInputs() const
{
	TSet<TObjectKey<UFlowNode>> IteratedNodes;
	TArray<UFlowNode*> ConnectedNodes;

	// Nodes connected to the Start node
	if (UFlowNode* DefaultEntryNode = GetDefaultEntryNode())
	{
		GetNodesInExecutionOrder_Recursive(DefaultEntryNode, IteratedNodes, ConnectedNodes);
	}

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

#if WITH_EDITOR
void UFlowAsset::RebuildCustomInterfaceLists()
{
	CustomInputs.Reset();
	CustomOutputs.Reset();

	for (const TPair<FGuid, UFlowNode*>& Pair : ObjectPtrDecay(Nodes))
	{
		if (const UFlowNode_CustomInput* InputNode = Cast<UFlowNode_CustomInput>(Pair.Value))
		{
			if (!InputNode->GetEventName().IsNone())
			{
				CustomInputs.AddUnique(InputNode->GetEventName());
			}
		}
		else if (const UFlowNode_CustomOutput* OutputNode = Cast<UFlowNode_CustomOutput>(Pair.Value))
		{
			if (!OutputNode->GetEventName().IsNone())
			{
				CustomOutputs.AddUnique(OutputNode->GetEventName());
			}
		}
	}
}

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

#if WITH_EDITOR
void UFlowAsset::InitializePinConnectionPolicy()
{
	const FInstancedStruct& SourceStruct = GetDefault<UFlowSettings>()->PinConnectionPolicy;
	if (ensure(SourceStruct.IsValid()))
	{
		PinConnectionPolicy.InitializeAsScriptStruct(SourceStruct.GetScriptStruct(), SourceStruct.GetMemory());
	}
}
#endif

const FFlowPinConnectionPolicy& UFlowAsset::GetPinConnectionPolicy() const
{
	// Runtime instances delegate to their template, which holds the serialized policy
	if (!PinConnectionPolicy.IsValid() && IsValid(TemplateAsset))
	{
		return TemplateAsset->GetPinConnectionPolicy();
	}

	// Graceful fallback: if PinConnectionPolicy was never initialized (asset predates this feature,
	// or was never opened in editor), read directly from Project Settings at runtime.
	if (!PinConnectionPolicy.IsValid())
	{
		const FFlowPinConnectionPolicy* SettingsPolicy = GetDefault<UFlowSettings>()->GetPinConnectionPolicy();
		ensureAlways(SettingsPolicy);
		if (SettingsPolicy)
		{
			return *SettingsPolicy;
		}
	}

	check(PinConnectionPolicy.IsValid());
	return PinConnectionPolicy.Get();
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

#if WITH_EDITOR
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
	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
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

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().AssetCreated(NewParams);

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	const TArray<UObject*> AssetsToSync = {NewParams};
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
#endif

void UFlowAsset::AddInstance(UFlowAsset* Instance)
{
	ActiveInstances.Add(Instance);
}

int32 UFlowAsset::RemoveInstance(UFlowAsset* Instance)
{
#if WITH_EDITOR
	const UFlowAsset* InspectedInstancePtr = InspectedInstance.Get();
	if (InspectedInstancePtr && InspectedInstancePtr == Instance)
	{
		SetInspectedInstance(nullptr);
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
		SetInspectedInstance(nullptr);
	}
#endif

	for (int32 i = ActiveInstances.Num() - 1; i >= 0; i--)
	{
		if (ActiveInstances.IsValidIndex(i) && ActiveInstances[i])
		{
			ActiveInstances[i]->FinishFlowInstance(EFlowFinishPolicy::Keep);
			ActiveInstances[i]->DeinitializeInstance();
		}
	}

	ActiveInstances.Empty();
}

#if WITH_EDITOR
void UFlowAsset::AddReplicaInstance(UFlowAsset* Instance)
{
	// Replicas destroyed without PreDestroyFromReplication, i.e. on ending PIE, leave empty entries
	ReplicaInstances.RemoveAll([](const TWeakObjectPtr<UFlowAsset>& ReplicaInstance)
	{
		return !ReplicaInstance.IsValid();
	});

	ReplicaInstances.Add(Instance);
	BroadcastDebuggerRefresh();
}

void UFlowAsset::RemoveReplicaInstance(UFlowAsset* Instance)
{
	const UFlowAsset* InspectedInstancePtr = InspectedInstance.Get();
	if (InspectedInstancePtr && InspectedInstancePtr == Instance)
	{
		SetInspectedInstance(nullptr);
	}

	ReplicaInstances.Remove(Instance);
	BroadcastDebuggerRefresh();
}

void UFlowAsset::SetInspectedInstance(TWeakObjectPtr<const UFlowAsset> NewInspectedInstance)
{
	if (const UFlowAsset* NewInspectedInstancePtr = NewInspectedInstance.Get())
	{
		if (InspectedInstance == NewInspectedInstance)
		{
			// Nothing changed
			return;
		}

		bool bIsNewInstancePresent = Algo::AnyOf(ActiveInstances, [NewInspectedInstancePtr](const UFlowAsset* ActiveInstance)
		{
			return ActiveInstance && ActiveInstance == NewInspectedInstancePtr;
		}) || Algo::AnyOf(ReplicaInstances, [NewInspectedInstancePtr](const TWeakObjectPtr<UFlowAsset>& ReplicaInstance)
		{
			return ReplicaInstance.Get() == NewInspectedInstancePtr;
		});

		if (!ensureMsgf(bIsNewInstancePresent, TEXT("Trying to set %s as InspectedInstance, but it is not one of the ActiveInstances"), *NewInspectedInstancePtr->GetName()))
		{
			NewInspectedInstance = nullptr;
		}
	}

	InspectedInstance = NewInspectedInstance;
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

bool UFlowAsset::CanBeInstanced(const UFlowAsset& InTemplateAsset, const UObject* InOwner) const
{
	// Called for the template and again for its new instance.
	// Use the (this != &InTemplateAsset) comparison if you need to test the new instance.

	if (GetDefault<UFlowSettings>()->bRejectFlowInstancesWithInvalidNodes)
	{
		for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
		{
			if (!IsValid(Node.Value))
			{
				UE_LOG(LogFlow, Error,
					TEXT("Flow instance creation rejected an invalid node. Template: '%s', Asset: '%s', "
						"Node GUID: '%s', Owner: '%s'. Repair and recook the graph; it will not execute."),
					*InTemplateAsset.GetPathName(), *GetPathName(), *Node.Key.ToString(), *GetPathNameSafe(InOwner));
				return false;
			}
		}
	}

	return true;
}

void UFlowAsset::InitializeInstance(const TWeakObjectPtr<UObject> InOwner, UFlowAsset& InTemplateAsset)
{
	check(!IsInstanceInitialized());

	Owner = InOwner;
	TemplateAsset = &InTemplateAsset;
	// UFlowSubsystem::CreateFlowInstance assigns the root first; callers that initialize directly become their own root.
	if (!IsValid(OwningRootFlowInstance))
	{
		OwningRootFlowInstance = this;
	}

	// Initialize any customizable Policies before we instantiate nodes
	InitializePreloadPolicy();

	for (TPair<FGuid, TObjectPtr<UFlowNode>>& Node : Nodes)
	{
		// Node can be null if its class has been removed.
		if (IsValid(Node.Value))
		{
			UFlowNode* NewNodeInstance = NewObject<UFlowNode>(this, Node.Value->GetClass(), NAME_None, RF_Transient, Node.Value, false, nullptr);
			Node.Value = NewNodeInstance;

			if (UFlowNode_CustomInput* CustomInput = Cast<UFlowNode_CustomInput>(NewNodeInstance))
			{
				if (!CustomInput->GetEventName().IsNone())
				{
					CustomInputNodes.Emplace(CustomInput);
				}
			}

			NewNodeInstance->InitializeInstance();
		}
		else
		{
			UE_LOG(LogFlow, Error, TEXT("Flow asset '%s' contains an invalid node for GUID '%s', node won't be instanced."), *InTemplateAsset.GetPathName(), *Node.Key.ToString());
		}
	}
}

FName UFlowAsset::GetInstanceName() const
{
	return GetFName();
}

AActor* UFlowAsset::TryFindActorOwner() const
{
	UObject* OwnerObject = GetOwner();
	if (!IsValid(OwnerObject))
	{
		return nullptr;
	}

	// If the owner is already an Actor, return it directly
	if (AActor* OwnerAsActor = Cast<AActor>(OwnerObject))
	{
		return OwnerAsActor;
	}

	// If the owner is a Component, return its owning Actor
	if (const UActorComponent* OwnerAsComponent = Cast<UActorComponent>(OwnerObject))
	{
		return OwnerAsComponent->GetOwner();
	}

	return nullptr;
}

void UFlowAsset::PreStartFlow()
{
	ResetNodes();

#if WITH_EDITOR
	check(IsInstanceInitialized());

	if (TemplateAsset->ActiveInstances.Num() == 1)
	{
		// this instance is the only active one, set it directly as Inspected Instance
		TemplateAsset->SetInspectedInstance(this);
	}
	else
	{
		// request to refresh list to show newly created instance
		TemplateAsset->BroadcastDebuggerRefresh();
	}
#endif
}

void UFlowAsset::StartFlow(IFlowDataPinValueSupplierInterface* DataPinValueSupplier, IFlowGraphOutputDataReceiverInterface* InOutputDataReceiver)
{
	InitializeOutputDataReceiverAndValues(InOutputDataReceiver);

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

bool UFlowAsset::HasStartedFlow() const
{
	return RecordedNodes.Num() > 0;
}

void UFlowAsset::InitializeOutputDataReceiverAndValues(IFlowGraphOutputDataReceiverInterface* InOutputDataReceiver)
{
	OutputDataReceiver = Cast<UObject>(InOutputDataReceiver);

	// Initialize the live output store from the template asset's declarations
	OutputDataPinValues.Values.Reset();

	if (const UFlowAsset* Template = TemplateAsset.Get())
	{
		for (const FFlowNamedDataPinProperty& Declaration : Template->OutputDataPinDeclarations)
		{
			if (Declaration.IsValid())
			{
				OutputDataPinValues.Values.Add(Declaration.Name, Declaration.DataPinValue);
			}
			else
			{
				UE_LOG(LogFlow, Warning, TEXT("Invalid OutputDataPin %s"), *Declaration.Name.ToString());
			}
		}
	}
}

void UFlowAsset::WriteOutputDataPinValue(const FName& PinName, const TInstancedStruct<FFlowDataPinValue>& Value)
{
	if (OutputDataPinValues.Values.Contains(PinName))
	{
		OutputDataPinValues.Values[PinName] = Value;
	}
	else
	{
		UE_LOG(LogFlow, Warning, TEXT("Could not find pin named %s in WriteOutputDataPinValue"), *PinName.ToString());
	}
}

void UFlowAsset::FlushOutputDataPinValuesToReceiver() const
{
	if (IFlowGraphOutputDataReceiverInterface* Receiver = Cast<IFlowGraphOutputDataReceiverInterface>(OutputDataReceiver.Get()))
	{
		// Do an immediate push to the receiver
		Receiver->ReceiveOutputDataSnapshot(OutputDataPinValues);
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
			// if there's receiver able to read Output Data Pins
			if (IFlowGraphOutputDataReceiverInterface* Receiver = Cast<IFlowGraphOutputDataReceiverInterface>(OutputDataReceiver.Get()))
			{
				Receiver->ReceiveOutputDataSnapshot(OutputDataPinValues);
			}

			if (UFlowNode_SubGraph* OwningSubGraphNode = NodeOwningThisAssetInstance.Get())
			{
				OwningSubGraphNode->TriggerFirstOutput(true);
				return;
			}

			// if this instance is a Root Flow, we need to deregister it from the subsystem first. This will
			// finalize and deinitialize the root flow.
			UObject* OwnerObject = Owner.Get();
			UFlowSubsystem* FlowSubsystem = GetFlowSubsystem();
			if (OwnerObject && FlowSubsystem)
			{
				const TSet<UFlowAsset*>& RootFlowInstances = FlowSubsystem->GetRootInstancesByOwner(OwnerObject);
				if (RootFlowInstances.Contains(this))
				{
					FlowSubsystem->FinishAndDeinitializeRootFlow(OwnerObject, TemplateAsset, EFlowFinishPolicy::Keep);

					return;
				}
			}

			FinishFlowInstance(EFlowFinishPolicy::Keep);
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

void UFlowAsset::FinishFlow(const EFlowFinishPolicy InFinishPolicy, const bool bRemoveInstance)
{
	FinishFlowInstance(InFinishPolicy);
	
	if (bRemoveInstance)
	{
		DeinitializeInstance();
	}
}

void UFlowAsset::FinishFlowInstance(const EFlowFinishPolicy InFinishPolicy)
{
	FinishPolicy = InFinishPolicy;

	CancelAndWarnForUnflushedDeferredTriggers();

	// end execution of this asset and all of its nodes
	for (UFlowNode* Node : ActiveNodes)
	{
		Node->Deactivate();
	}
	ActiveNodes.Empty();
}

void UFlowAsset::DeinitializeInstance()
{
	// These should have been flushed in FinishFlowInstance()
	check(DeferredTransitionScopes.IsEmpty());

	if (IsInstanceInitialized())
	{
		StopReplication();

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

		// Unregister this child flow as a user of all components tracked by the root's manager.
		UFlowInjectComponentsManager* RootManager = FindComponentInjectionManagerInRootFlow();
		if (IsValid(RootManager) && RootManager != ComponentInjector)
		{
			RootManager->UnregisterAllComponentUsers(*this);
		}

		// Shut down only our own manager, which may be distinct from the root's manager.
		if (IsValid(ComponentInjector))
		{
			ComponentInjector->ShutdownRuntime();
			ComponentInjector = nullptr;
		}
		OwningRootFlowInstance = nullptr;
		TemplateAsset = nullptr;
	}
}

ESubGraphFinishPolicy UFlowAsset::GetSubGraphFinishPolicy() const
{
	return GetDefault<UFlowSettings>()->SubGraphFinishPolicy;
}

UFlowSubsystem* UFlowAsset::GetFlowSubsystem() const
{
	return Cast<UFlowSubsystem>(GetOuter());
}

UFlowNode_SubGraph* UFlowAsset::GetNodeOwningThisAssetInstance() const
{
	return NodeOwningThisAssetInstance.Get();
}

UFlowAsset* UFlowAsset::GetParentInstance() const
{
	const UFlowNode_SubGraph* OwningSubGraphNode = NodeOwningThisAssetInstance.Get();
	return OwningSubGraphNode ? OwningSubGraphNode->GetFlowAsset() : nullptr;
}

TWeakObjectPtr<UFlowAsset> UFlowAsset::GetFlowInstance(UFlowNode_SubGraph* SubGraphNode) const
{
	return ActiveSubGraphs.FindRef(SubGraphNode);
}

UFlowAsset* UFlowAsset::GetOwningRootFlowInstance() const
{
	return OwningRootFlowInstance.Get();
}

UFlowInjectComponentsManager* UFlowAsset::GetOrCreateComponentInjectionManager()
{
	if (!IsValid(ComponentInjector))
	{
		ComponentInjector = NewObject<UFlowInjectComponentsManager>(this);
		ComponentInjector->InitializeRuntime();
	}
	return ComponentInjector;
}

UFlowInjectComponentsManager* UFlowAsset::GetOrCreateComponentInjectionManagerInRootFlow() const
{
	UFlowAsset* Root = GetOwningRootFlowInstance();
	checkf(IsValid(Root), TEXT("Root component manager access requires a runtime Flow instance."));
	return Root->GetOrCreateComponentInjectionManager();
}

UFlowInjectComponentsManager* UFlowAsset::FindComponentInjectionManager() const
{
	return IsValid(ComponentInjector) ? ComponentInjector.Get() : nullptr;
}

UFlowInjectComponentsManager* UFlowAsset::FindComponentInjectionManagerInRootFlow() const
{
	const UFlowAsset* Root = GetOwningRootFlowInstance();
	return IsValid(Root) ? Root->FindComponentInjectionManager() : nullptr;
}

void UFlowAsset::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

#if WITH_PUSH_MODEL
	FDoRepLifetimeParams InitialOnlyParams;
	InitialOnlyParams.bIsPushBased = true;
	InitialOnlyParams.Condition = COND_InitialOnly;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TemplateAsset, InitialOnlyParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Owner, InitialOnlyParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedParentInstance, InitialOnlyParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, OwningSubGraphNodeGuid, InitialOnlyParams);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedNodeStates, Params);
#else
	DOREPLIFETIME_CONDITION(ThisClass, TemplateAsset, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ThisClass, Owner, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedParentInstance, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ThisClass, OwningSubGraphNodeGuid, COND_InitialOnly);

	DOREPLIFETIME(ThisClass, ReplicatedNodeStates);
#endif
}

void UFlowAsset::PostInitProperties()
{
	Super::PostInitProperties();

	// Instances copy this pointer from their template
	ReplicatedNodeStates.OwningAsset = this;
}

void UFlowAsset::PreDestroyFromReplication()
{
	// The last state sent by the server in the same frame as removing this instance might not arrive
	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (IsValid(Node.Value) && Node.Value->GetActivationState() == EFlowNodeState::Active)
		{
			Node.Value->ActivationState = EFlowNodeState::Completed;
			Node.Value->OnRep_ActivationState(EFlowNodeState::Active);
		}
	}

	if (UFlowAsset* ParentInstance = ReplicatedParentInstance.Get())
	{
		// Replica of the next run of the same SubGraph node might be linked already
		if (ParentInstance->ActiveSubGraphs.FindRef(NodeOwningThisAssetInstance).Get() == this)
		{
			ParentInstance->ActiveSubGraphs.Remove(NodeOwningThisAssetInstance);
		}
	}

#if WITH_EDITOR
	if (bReplicaInitialized && IsValid(TemplateAsset))
	{
		TemplateAsset->RemoveReplicaInstance(this);
	}
#endif

	Super::PreDestroyFromReplication();
}

bool UFlowAsset::ShouldReplicate() const
{
	return GetDefault<UFlowSettings>()->bReplicateFlowInstances && bReplicateInstances;
}

void UFlowAsset::StartReplication()
{
	const UFlowNode_SubGraph* OwningSubGraphNode = NodeOwningThisAssetInstance.Get();
	UFlowAsset* ParentInstance = OwningSubGraphNode ? OwningSubGraphNode->GetFlowAsset() : nullptr;

	// Clients link the replica of a SubGraph instance to the replica of its parent
	const bool bParentReplicates = ParentInstance == nullptr || ParentInstance->bReplicatedToClients;

	if (ShouldReplicate() && !bReplicatedToClients && bParentReplicates)
	{
		UObject* OwnerObject = Owner.Get();
		if (AActor* OwnerActor = Cast<AActor>(OwnerObject))
		{
			if (OwnerActor->GetIsReplicated() && (OwnerActor->IsNetMode(NM_DedicatedServer) || OwnerActor->IsNetMode(NM_ListenServer)))
			{
				if (OwnerActor->IsUsingRegisteredSubObjectList())
				{
					OwnerActor->AddReplicatedSubObject(this);
					bReplicatedToClients = true;
				}
				else
				{
					UE_LOG(LogFlow, Warning, TEXT("%s won't replicate, since its Owner %s doesn't use the registered subobject list."), *GetName(), *OwnerActor->GetName());
				}
			}
		}
		else if (UActorComponent* OwnerComponent = Cast<UActorComponent>(OwnerObject))
		{
			if (OwnerComponent->GetIsReplicated() && (OwnerComponent->IsNetMode(NM_DedicatedServer) || OwnerComponent->IsNetMode(NM_ListenServer)))
			{
				if (OwnerComponent->IsUsingRegisteredSubObjectList())
				{
					OwnerComponent->AddReplicatedSubObject(this);
					bReplicatedToClients = true;
				}
				else
				{
					UE_LOG(LogFlow, Warning, TEXT("%s won't replicate, since its Owner %s doesn't use the registered subobject list."), *GetName(), *OwnerComponent->GetName());
				}
			}
		}

		if (bReplicatedToClients)
		{
			if (ParentInstance)
			{
				ReplicatedParentInstance = ParentInstance;
				OwningSubGraphNodeGuid = OwningSubGraphNode->GetGuid();
			}

			for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
			{
				if (IsValid(Node.Value))
				{
					Node.Value->bHasReplicatedProperties = Node.Value->ShouldReplicateProperties() && FlowNodeReplication::HasReplicatedProperties(*Node.Value);
				}
			}

#if WITH_PUSH_MODEL
			MARK_PROPERTY_DIRTY_FROM_NAME(UFlowAsset, TemplateAsset, this);
			MARK_PROPERTY_DIRTY_FROM_NAME(UFlowAsset, Owner, this);
			MARK_PROPERTY_DIRTY_FROM_NAME(UFlowAsset, ReplicatedParentInstance, this);
			MARK_PROPERTY_DIRTY_FROM_NAME(UFlowAsset, OwningSubGraphNodeGuid, this);
#endif
		}
	}
}

void UFlowAsset::StopReplication()
{
	if (bReplicatedToClients)
	{
		UObject* OwnerObject = Owner.Get();
		if (AActor* OwnerActor = Cast<AActor>(OwnerObject))
		{
			OwnerActor->DestroyReplicatedSubObjectOnRemotePeers(this);
		}
		else if (UActorComponent* OwnerComponent = Cast<UActorComponent>(OwnerObject))
		{
			OwnerComponent->DestroyReplicatedSubObjectOnRemotePeers(this);
		}

		bReplicatedToClients = false;
	}
}

void UFlowAsset::OnRep_TemplateAsset()
{
	if (!bReplicaInitialized && IsValid(TemplateAsset))
	{
		bReplicaInitialized = true;

#if WITH_EDITOR
		TemplateAsset->AddReplicaInstance(this);
#endif

		LinkReplicaToParent();

		for (const TWeakObjectPtr<UFlowAsset>& ChildInstance : ReplicaChildrenAwaitingTemplate)
		{
			if (UFlowAsset* Child = ChildInstance.Get())
			{
				Child->LinkReplicaToParent();
			}
		}
		ReplicaChildrenAwaitingTemplate.Empty();

		for (const FFlowNodeReplicatedState& Item : ReplicatedNodeStates.Items)
		{
			ApplyReplicatedNodeState(Item);
		}
	}
}

void UFlowAsset::OnRep_ReplicatedParentInstance()
{
	LinkReplicaToParent();
}

void UFlowAsset::LinkReplicaToParent()
{
	if (!NodeOwningThisAssetInstance.IsValid())
	{
		if (UFlowAsset* ParentInstance = ReplicatedParentInstance.Get())
		{
			// The SubGraph node of the parent can be created only from its TemplateAsset
			if (ParentInstance->bReplicaInitialized)
			{
				if (UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(ParentInstance->GetOrCreateReplicaNode(OwningSubGraphNodeGuid)))
				{
					NodeOwningThisAssetInstance = SubGraphNode;
					ParentInstance->ActiveSubGraphs.Add(SubGraphNode, this);
				}
			}
			else
			{
				ParentInstance->ReplicaChildrenAwaitingTemplate.AddUnique(this);
			}
		}
	}
}

UFlowNode* UFlowAsset::GetOrCreateReplicaNode(const FGuid& NodeGuid)
{
	UFlowNode* Node = Nodes.FindRef(NodeGuid);

	if (Node == nullptr && IsValid(TemplateAsset))
	{
		// Node can be null if its class has been removed.
		UFlowNode* TemplateNode = TemplateAsset->Nodes.FindRef(NodeGuid);
		if (IsValid(TemplateNode))
		{
			Node = NewObject<UFlowNode>(this, TemplateNode->GetClass(), NAME_None, RF_Transient, TemplateNode, false, nullptr);
			Nodes.Add(NodeGuid, Node);

			Node->InitializeReplica();
		}
	}

	return Node;
}

void UFlowAsset::WriteReplicatedNodeState(UFlowNode& Node)
{
	if (bReplicatedToClients && Node.bHasReplicatedProperties)
	{
		const FGuid& NodeGuid = Node.GetGuid();
		FFlowNodeReplicatedState* Item = ReplicatedNodeStates.Items.FindByPredicate([&NodeGuid](const FFlowNodeReplicatedState& Existing)
		{
			return Existing.NodeGuid == NodeGuid;
		});

		if (Item == nullptr)
		{
			Item = &ReplicatedNodeStates.Items.AddDefaulted_GetRef();
			Item->NodeGuid = NodeGuid;
		}

		FlowNodeReplication::WriteReplicatedProperties(Node, Item->CustomData);

		// Legacy replication and Iris both limit the number of replicated array elements to uint16
		if (Item->CustomData.Num() > MAX_uint16)
		{
			UE_LOG(LogFlow, Error, TEXT("Replicated properties of node %s in %s take %d bytes, above the limit of %d bytes. Clients won't receive them."), *Node.GetName(), *GetName(), Item->CustomData.Num(), MAX_uint16);
		}

		ReplicatedNodeStates.MarkItemDirty(*Item);
#if WITH_PUSH_MODEL
		MARK_PROPERTY_DIRTY_FROM_NAME(UFlowAsset, ReplicatedNodeStates, this);
#endif
	}
}

void UFlowAsset::ApplyReplicatedNodeState(const FFlowNodeReplicatedState& Item)
{
	// States received before TemplateAsset are applied by OnRep_TemplateAsset
	if (bReplicaInitialized)
	{
		if (UFlowNode* Node = GetOrCreateReplicaNode(Item.NodeGuid))
		{
			FlowNodeReplication::ReadReplicatedProperties(*Node, Item.CustomData);
		}
	}
}

void UFlowAsset::InitializePreloadPolicy()
{
	if (PreloadPolicy.IsValid())
	{
		// use per-class policy
		PreloadPolicy.InitializeAsScriptStruct(PreloadPolicy.GetScriptStruct(), PreloadPolicy.GetMemory());
	}
	else
	{
		// fallback to project's default policy
		const FInstancedStruct& DefaultPolicy = GetDefault<UFlowSettings>()->PreloadPolicy;
		if (ensure(DefaultPolicy.IsValid()))
		{
			PreloadPolicy.InitializeAsScriptStruct(DefaultPolicy.GetScriptStruct(), DefaultPolicy.GetMemory());
		}
	}

	ensureAlwaysMsgf(PreloadPolicy.IsValid(), TEXT("There's no valid Preload Policy set in the project!"));
}

void UFlowAsset::EnsurePreloadPolicyInitialized()
{
	if (!PreloadPolicy.IsValid())
	{
		InitializePreloadPolicy();
	}
}

const FFlowPreloadPolicy& UFlowAsset::GetPreloadPolicy() const
{
	checkf(PreloadPolicy.IsValid(), TEXT("PreloadPolicy must be initialized prior to calling GetPreloadPolicy()"));
	return PreloadPolicy.Get();
}

void UFlowAsset::TriggerCustomInput(const FName& EventName, IFlowDataPinValueSupplierInterface* DataPinValueSupplier)
{
	for (UFlowNode_CustomInput* CustomInputNode : CustomInputNodes)
	{
		if (CustomInputNode->GetEventName() == EventName)
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

void UFlowAsset::TriggerCustomInput_FromSubGraph(UFlowNode_SubGraph* SubGraphNode, const FName& EventName) const
{
	// NOTE (gtaylor) Custom Input nodes cannot currently add data pins (like Start or DefineProperties nodes can)
	// but we may want to allow them to source parameters, so I am providing the subgraph node as the 
	// IFlowDataPinValueSupplierInterface when triggering the node (even though it's not used at this time).

	if (UFlowAsset* FlowInstance = ActiveSubGraphs.FindRef(SubGraphNode).Get())
	{
		FlowInstance->TriggerCustomInput(EventName, SubGraphNode);
	}
}

void UFlowAsset::TriggerCustomOutput(const FName& EventName)
{
	if (UFlowNode_SubGraph* OwningSubGraphNode = NodeOwningThisAssetInstance.Get())
	{
		// it's a SubGraph
		OwningSubGraphNode->TriggerOutput(EventName);
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
	if (ShouldDeferTriggers())
	{
		// Defer only if we have an open the top scope
		if (!DeferredTransitionScopes.IsEmpty() && DeferredTransitionScopes.Top()->IsOpen())
		{
			EnqueueDeferredTrigger(NodeGuid, PinName, FromPin);
		}
		else
		{
			const TSharedPtr<FFlowDeferredTransitionScope> CurrentScope = PushDeferredTransitionScope();
			TriggerInputDirect(NodeGuid, PinName, FromPin);
			PopDeferredTransitionScope(CurrentScope);
		}
	}
	else
	{
		TriggerInputDirect(NodeGuid, PinName, FromPin);
	}
}

void UFlowAsset::TriggerInputDirect(const FGuid& NodeGuid, const FName& PinName, const FConnectedPin& FromPin)
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

bool UFlowAsset::ShouldDeferTriggers() const
{
	return GetDefault<UFlowSettings>()->bDeferTriggeredOutputsWhileTriggering;
}

void UFlowAsset::EnqueueDeferredTrigger(const FGuid& NodeGuid, const FName& PinName, const FConnectedPin& FromPin)
{
	// TriggerInput() is the only caller, and only enqueues while the top scope is open
	check(!DeferredTransitionScopes.IsEmpty() && DeferredTransitionScopes.Top()->IsOpen());

	// Always enqueue to the current innermost (top) scope
	DeferredTransitionScopes.Top()->EnqueueDeferredTrigger(FFlowDeferredTriggerInput{NodeGuid, PinName, FromPin});
}

TSharedPtr<FFlowDeferredTransitionScope> UFlowAsset::PushDeferredTransitionScope()
{
	// Close the former top scope (if any)
	if (!DeferredTransitionScopes.IsEmpty())
	{
		const TSharedPtr<FFlowDeferredTransitionScope>& FormerTop = DeferredTransitionScopes.Top();
		FormerTop->CloseScope();
	}

	// Push a fresh open scope
	return DeferredTransitionScopes.Add_GetRef(MakeShared<FFlowDeferredTransitionScope>());
}

void UFlowAsset::PopDeferredTransitionScope(const TSharedPtr<FFlowDeferredTransitionScope>& Scope)
{
	TryFlushAndRemoveDeferredTransitionScope(Scope);
}

bool UFlowAsset::TryFlushAndRemoveDeferredTransitionScope(const TSharedPtr<FFlowDeferredTransitionScope>& ScopeToFlush)
{
	if (!ScopeToFlush->TryFlushDeferredTriggers(*this))
	{
		return false;
	}

	// Remove the exact instance we were holding (handles nested push/pop cases)
	DeferredTransitionScopes.RemoveSingle(ScopeToFlush);
	return true;
}

bool UFlowAsset::TryFlushAllDeferredTriggerScopes()
{
	while (const TSharedPtr<FFlowDeferredTransitionScope> TopScope = GetTopDeferredTransitionScope())
	{
		if (!TryFlushAndRemoveDeferredTransitionScope(TopScope))
		{
			break;
		}

		// Keep flushing until the stack is empty
	}

	check(DeferredTransitionScopes.IsEmpty());

	return DeferredTransitionScopes.IsEmpty();
}

void UFlowAsset::ClearAllDeferredTriggerScopes()
{
	DeferredTransitionScopes.Reset();
}

void UFlowAsset::CancelAndWarnForUnflushedDeferredTriggers()
{
	// Aggressively drop any pending deferred triggers — graph is done
	// In normal execution these should have been flushed via PopDeferredTransitionScope() in TriggerInputDirect
	// Remaining scopes here usually mean:
	//   - early/abnormal termination (e.g. FinishFlowInstance called from unexpected place)
	//   - exception/early return before Pop
	//   - forced deinitialization during active execution (e.g. PIE stop, subsystem cleanup)
	if (!DeferredTransitionScopes.IsEmpty())
	{
		int32 TotalDroppedTriggers = 0;

		for (const TSharedPtr<FFlowDeferredTransitionScope>& ScopePtr : DeferredTransitionScopes)
		{
			if (!ScopePtr.IsValid())
			{
				continue;
			}

			const TArray<FFlowDeferredTriggerInput>& Triggers = ScopePtr->GetDeferredTriggers();

			if (TotalDroppedTriggers == 0 && !Triggers.IsEmpty())
			{
				UE_LOG(LogFlow, Warning, TEXT("FlowAsset '%s' is finishing with %d lingering deferred transition scope(s) — dropping them. "
					"This is usually unexpected and may indicate a bug or abnormal termination."),
				    *GetName(), DeferredTransitionScopes.Num());
			}

			TotalDroppedTriggers += Triggers.Num();

			for (const FFlowDeferredTriggerInput& Trigger : Triggers)
			{
				const UFlowNode* ToNode = GetNode(Trigger.NodeGuid);
				const UFlowNode* FromNode = Trigger.FromPin.NodeGuid.IsValid() ? GetNode(Trigger.FromPin.NodeGuid) : nullptr;

				const FString ToNodeName = ToNode ? ToNode->GetName() : TEXT("<null/destroyed>");
				const FString FromNodeName = FromNode ? FromNode->GetName() : TEXT("<null/destroyed>");

				UE_LOG(LogFlow, Error,
				       TEXT("  → Dropped deferred trigger:\n")
				       TEXT("      To Node: %s (%s)\n")
				       TEXT("      To Pin:  %s\n")
				       TEXT("      From Node: %s (%s)\n")
				       TEXT("      From Pin:  %s"),
				       *ToNodeName,
				       *Trigger.NodeGuid.ToString(),
				       *Trigger.PinName.ToString(),
				       *FromNodeName,
				       *Trigger.FromPin.NodeGuid.ToString(),
				       *Trigger.FromPin.PinName.ToString()
				);
			}
		}

		ClearAllDeferredTriggerScopes();
	}
}

TSharedPtr<FFlowDeferredTransitionScope> UFlowAsset::GetTopDeferredTransitionScope() const
{
	return !DeferredTransitionScopes.IsEmpty() ? DeferredTransitionScopes.Top() : nullptr;
}

FFlowAssetSaveData UFlowAsset::SaveInstance(TArray<FFlowAssetSaveData>& SavedFlowInstances)
{
	FFlowAssetSaveData AssetRecord;
	AssetRecord.WorldName = IsBoundToWorld() ? GetWorld()->GetName() : FString();
	AssetRecord.InstanceName = GetName();

	// opportunity to collect data before serializing asset
	OnSave();

	// iterate nodes
	TArray<UFlowNode*> NodesInExecutionOrder = GatherNodesConnectedToAllInputs();
	for (UFlowNode* Node : NodesInExecutionOrder)
	{
		if (Node && Node->ShouldSave())
		{
			// iterate SubGraphs
			if (UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(Node))
			{
				if (UFlowAsset* SubFlowInstance = GetFlowInstance(SubGraphNode).Get())
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

bool UFlowAsset::IsBoundToWorld_Implementation() const
{
	return bWorldBound;
}

#if WITH_EDITOR

void UFlowAsset::LogError(const FString& MessageToLog, const UFlowNodeBase* Node) const
{
	LogRuntimeMessage(EMessageSeverity::Error, MessageToLog, Node);
}

void UFlowAsset::LogWarning(const FString& MessageToLog, const UFlowNodeBase* Node) const
{
	LogRuntimeMessage(EMessageSeverity::Warning, MessageToLog, Node);
}

void UFlowAsset::LogNote(const FString& MessageToLog, const UFlowNodeBase* Node) const
{
	LogRuntimeMessage(EMessageSeverity::Info, MessageToLog, Node);
}

void UFlowAsset::LogRuntimeMessage(EMessageSeverity::Type Severity, const FString& MessageToLog, const UFlowNodeBase* Node) const
{
	// this is runtime log which should only be called on runtime instances of asset
	if (TemplateAsset)
	{
		UE_LOG(LogFlow, Log, TEXT("Attempted to use Runtime Log on asset instance %s"), *MessageToLog);
	}

	if (RuntimeLog.Get())
	{
		TSharedPtr<FTokenizedMessage> TokenizedMessage = nullptr;
		switch (Severity)
		{
		case EMessageSeverity::Error:
			TokenizedMessage = RuntimeLog.Get()->Error(*MessageToLog, Node);
			break;

		case EMessageSeverity::Warning:
			TokenizedMessage = RuntimeLog.Get()->Warning(*MessageToLog, Node);
			break;

		default:
			TokenizedMessage = RuntimeLog.Get()->Note(*MessageToLog, Node);
			break;
		}

		BroadcastRuntimeMessageAdded(TokenizedMessage.ToSharedRef());
	}
}
#endif
