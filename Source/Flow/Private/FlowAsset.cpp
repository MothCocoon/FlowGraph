#include "FlowAsset.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"

#include "Nodes/FlowNode.h"
#include "Nodes/Route/FlowNode_CustomEvent.h"
#include "Nodes/Route/FlowNode_Start.h"
#include "Nodes/Route/FlowNode_Finish.h"
#include "Nodes/Route/FlowNode_SubGraph.h"

UFlowAsset::UFlowAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
#if WITH_EDITOR
	, FlowGraph(nullptr)
#endif
	, TemplateAsset(nullptr)
	, StartNode(nullptr)
{
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

	if (PropertyChangedEvent.Property && (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomEvents)
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
		Nodes.Empty();
	}
}

EDataValidationResult UFlowAsset::IsDataValid(TArray<FText>& ValidationErrors)
{
	for (const TPair<FGuid, UFlowNode*>& NodePair : Nodes)
	{
		EDataValidationResult Result = NodePair.Value->IsDataValid(ValidationErrors);
		if (Result == EDataValidationResult::Invalid)
		{
			return EDataValidationResult::Invalid;
		}
	}

	return EDataValidationResult::Valid;
}

TSharedPtr<IFlowGraphInterface> UFlowAsset::FlowGraphInterface = nullptr;

void UFlowAsset::SetFlowGraphInterface(TSharedPtr<IFlowGraphInterface> InFlowAssetEditor)
{
	check(!FlowGraphInterface.IsValid());
	FlowGraphInterface = InFlowAssetEditor;
}

UFlowNode* UFlowAsset::CreateNode(UClass* NodeClass, UEdGraphNode* GraphNode)
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
}

void UFlowAsset::UnregisterNode(const FGuid& NodeGuid)
{
	Nodes.Remove(NodeGuid);
	Nodes.Compact();

	HarvestNodeConnections();
	MarkPackageDirty();
}
#endif

void UFlowAsset::HarvestNodeConnections()
{
	TMap<FName, FConnectedPin> Connections;

	// last moment to remove invalid nodes
	for (auto NodeIt = Nodes.CreateIterator(); NodeIt; ++NodeIt)
	{
		const TPair<FGuid, UFlowNode*>& Pair = *NodeIt;
		if (Pair.Value == nullptr)
		{
			NodeIt.RemoveCurrent();
		}
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : Nodes)
	{
		UFlowNode* Node = Pair.Value;
		Connections.Empty();

		for (const UEdGraphPin* ThisPin : Node->GetGraphNode()->Pins)
		{
			if (ThisPin->Direction == EGPD_Output && ThisPin->LinkedTo.Num() > 0)
			{
				if (UEdGraphPin* LinkedPin = ThisPin->LinkedTo[0])
				{
					const UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
					Connections.Add(ThisPin->PinName, FConnectedPin(LinkedNode->NodeGuid, LinkedPin->PinName));
				}
			}
		}

#if WITH_EDITOR
		Node->SetFlags(RF_Transactional);
		Node->Modify();
#endif
		Node->SetConnections(Connections);

#if WITH_EDITOR
		Node->PostEditChange();
#endif
	}
}

UFlowNode* UFlowAsset::GetNode(const FGuid& Guid) const
{
	if (UFlowNode* Node = Nodes.FindRef(Guid))
	{
		return Node;
	}

	return nullptr;
}

void UFlowAsset::AddInstance(UFlowAsset* NewInstance)
{
	ActiveInstances.Add(NewInstance);
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
		if (ActiveInstances[i])
		{
			ActiveInstances[i]->FinishFlow(false);
		}
	}

	ActiveInstances.Empty();
}

#if WITH_EDITOR
void UFlowAsset::GetInstanceDisplayNames(TArray<TSharedPtr<FName>>& OutDisplayNames) const
{
	for (UFlowAsset* Instance : ActiveInstances)
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

	BroadcastRegenerateToolbars();
}
#endif

void UFlowAsset::InitInstance(UObject* InOwner, UFlowAsset* InTemplateAsset)
{
	Owner = InOwner;
	TemplateAsset = InTemplateAsset;

	for (TPair<FGuid, UFlowNode*>& Node : Nodes)
	{
		UFlowNode* NewInstance = NewObject<UFlowNode>(this, Node.Value->GetClass(), NAME_None, RF_Transient, Node.Value, false, nullptr);
		Node.Value = NewInstance;

		// there can be only one, automatically added while creating graph
		if (UFlowNode_Start* InNode = Cast<UFlowNode_Start>(NewInstance))
		{
			StartNode = InNode;
		}

		if (UFlowNode_CustomEvent* CustomEvent = Cast<UFlowNode_CustomEvent>(NewInstance))
		{
			const FName& EventName = CustomEvent->EventName;
			if (!EventName.IsNone() && !CustomEventNodes.Contains(CustomEvent->EventName))
			{
				CustomEventNodes.Emplace(CustomEvent->EventName, CustomEvent);
			}
		}
	}
}

void UFlowAsset::PreloadNodes()
{
	TArray<UFlowNode*> GraphEntryNodes = {StartNode};
	for (const TPair<FName, UFlowNode_CustomEvent*>& CustomEvent : CustomEventNodes)
	{
		GraphEntryNodes.Emplace(CustomEvent.Value);
	}

	// NOTE: this is just the example algorithm of gathering nodes for pre-load
	for (UFlowNode* EntryNode : GraphEntryNodes)
	{
		for (const TPair<TSubclassOf<UFlowNode>, int32>& Node : UFlowSettings::Get()->DefaultPreloadDepth)
		{
			if (Node.Value > 0)
			{
				TArray<UFlowNode*> FoundNodes;
				UFlowNode::RecursiveFindNodesByClass(EntryNode, Node.Key, Node.Value, FoundNodes);

				for (UFlowNode* FoundNode : FoundNodes)
				{
					if (!PreloadedNodes.Contains(FoundNode))
					{
						FoundNode->TriggerPreload();
						PreloadedNodes.Emplace(FoundNode);
					}
				}
			}
		}
	}
}

void UFlowAsset::StartFlow()
{
	ResetNodes();

#if WITH_EDITOR
	if (TemplateAsset->ActiveInstances.Num() == 1)
	{
		TemplateAsset->SetInspectedInstance(GetDisplayName());
	}
	else
	{
		TemplateAsset->BroadcastRegenerateToolbars();
	}
#endif

	ensureAlways(StartNode);
	RecordedNodes.Add(StartNode);
	StartNode->TriggerFirstOutput(true);
}

void UFlowAsset::StartAsSubFlow(UFlowNode_SubGraph* SubGraphNode)
{
	NodeOwningThisAssetInstance = SubGraphNode;
	NodeOwningThisAssetInstance->GetFlowAsset()->ActiveSubGraphs.Add(SubGraphNode, this);

	StartFlow();
}

void UFlowAsset::FinishFlow(const bool bFlowCompleted)
{
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

	// clear instance entries
	const int32 ActiveInstancesLeft = TemplateAsset->RemoveInstance(this);
	if (ActiveInstancesLeft == 0 && GetFlowSubsystem())
	{
		GetFlowSubsystem()->RemoveInstancedTemplate(TemplateAsset);
	}

	// if this instance was created by SubGraph node
	if (NodeOwningThisAssetInstance.IsValid())
	{
		NodeOwningThisAssetInstance->GetFlowAsset()->ActiveSubGraphs.Remove(NodeOwningThisAssetInstance.Get());
		if (bFlowCompleted)
		{
			NodeOwningThisAssetInstance.Get()->TriggerFirstOutput(true);
		}

		NodeOwningThisAssetInstance = nullptr;
	}
}

UObject* UFlowAsset::GetOwner() const
{
	return Owner;
}

TWeakObjectPtr<UFlowAsset> UFlowAsset::GetFlowInstance(UFlowNode_SubGraph* SubGraphNode) const
{
	return ActiveSubGraphs.FindRef(SubGraphNode);
}

void UFlowAsset::TriggerCustomEvent(UFlowNode_SubGraph* Node, const FName& EventName)
{
	const TWeakObjectPtr<UFlowAsset> FlowInstance = ActiveSubGraphs.FindRef(Node);
	if (FlowInstance.IsValid())
	{
		if (UFlowNode_CustomEvent* CustomEvent = FlowInstance->CustomEventNodes.FindRef(EventName))
		{
			RecordedNodes.Add(CustomEvent);
			CustomEvent->TriggerFirstOutput(true);
		}
	}
}

void UFlowAsset::TriggerCustomOutput(const FName& EventName) const
{
	NodeOwningThisAssetInstance->TriggerOutput(EventName);
}

void UFlowAsset::TriggerInput(const FGuid& NodeGuid, const FName& PinName)
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

		if (Node->GetClass()->IsChildOf(UFlowNode_Finish::StaticClass()))
		{
			FinishFlow(true);
		}
	}
}

void UFlowAsset::ResetNodes()
{
#if !UE_BUILD_SHIPPING
	for (UFlowNode* Node : RecordedNodes)
	{
		Node->ResetRecords();
	}
#endif

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

UFlowAsset* UFlowAsset::GetMasterInstance() const
{
	return NodeOwningThisAssetInstance.IsValid() ? NodeOwningThisAssetInstance.Get()->GetFlowAsset() : nullptr;
}

UFlowNode* UFlowAsset::GetNodeInstance(const FGuid Guid) const
{
	return Nodes.FindRef(Guid);
}
