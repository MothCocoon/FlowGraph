#include "FlowAsset.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"

#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeIn.h"
#include "Nodes/FlowNodeOut.h"
#include "Nodes/FlowNodeSubGraph.h"

#if WITH_EDITOR
#include "FlowAssetGraph.h"
#include "FlowGraphSchema.h"

TSharedPtr<IFlowGraphInterface> UFlowAsset::FlowGraphInterface = nullptr;
#endif

UFlowAsset::UFlowAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void UFlowAsset::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad | RF_Transient))
	{
		CreateGraph();
	}
}

void UFlowAsset::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UFlowAsset* This = CastChecked<UFlowAsset>(InThis);
	Collector.AddReferencedObject(This->FlowGraph, This);

	Super::AddReferencedObjects(InThis, Collector);
}

void UFlowAsset::CreateGraph()
{
	if (FlowGraph == nullptr)
	{
		FlowGraph = UFlowAsset::GetFlowGraphInterface()->CreateGraph(this);

		FlowGraph->bAllowDeletion = false;
		FlowGraph->GetSchema()->CreateDefaultNodesForGraph(*FlowGraph);
	}
}

FGuid UFlowAsset::CreateGraphNode(UFlowNode* InFlowNode, bool bSelectNewNode /*= true*/) const
{
	check(InFlowNode->GraphNode == nullptr);
	return UFlowAsset::GetFlowGraphInterface()->CreateGraphNode(FlowGraph, InFlowNode, bSelectNewNode);
}

void UFlowAsset::SetFlowGraphInterface(TSharedPtr<IFlowGraphInterface> InFlowAssetEditor)
{
	check(!FlowGraphInterface.IsValid());
	FlowGraphInterface = InFlowAssetEditor;
}

void UFlowAsset::RegisterNode(const FGuid& NewGuid, UFlowNode* NewNode)
{
	NewNode->SetGuid(NewGuid);
	Nodes.Emplace(NewGuid, NewNode);
}

void UFlowAsset::UnregisterNode(FGuid NodeGuid)
{
	Nodes.Remove(NodeGuid);
	Nodes.Shrink();

	CompileNodeConnections();
	MarkPackageDirty();
}
#endif

void UFlowAsset::CompileNodeConnections()
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
			if (ThisPin->Direction == EEdGraphPinDirection::EGPD_Output && ThisPin->LinkedTo.Num() > 0)
			{
				if (UEdGraphPin* LinkedPin = ThisPin->LinkedTo[0])
				{
					const UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
					Connections.Add(ThisPin->PinName, FConnectedPin(LinkedNode->NodeGuid, LinkedNode->Pins.Find(LinkedPin), LinkedPin->PinName));
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

void UFlowAsset::RecursiveFindNodesByClass(UFlowNode* Node, const TSubclassOf<UFlowNode> Class, uint8 Depth, TArray<UFlowNode*>& OutNodes) const
{
	if (Node)
	{
		// Record the node if it is the desired type
		if (Node->GetClass() == Class)
		{
			OutNodes.AddUnique(Node);
		}

		if (OutNodes.Num() == Depth)
		{
			return;
		}

		// Recurse
		for (UFlowNode* ConnectedNode : Node->GetConnectedNodes())
		{
			RecursiveFindNodesByClass(ConnectedNode, Class, Depth, OutNodes);
		}
	}
}

void UFlowAsset::AddInstance(UFlowAsset* NewInstance)
{
	ActiveInstances.Add(NewInstance);

#if WITH_EDITOR
	if (ActiveInstances.Num() == 1)
	{
		InspectedInstance = NewInstance;
	}
#endif
}

int32 UFlowAsset::RemoveInstance(UFlowAsset* Instance)
{
#if WITH_EDITOR
	if (InspectedInstance.IsValid() && InspectedInstance.Get() == Instance)
	{
		InspectedInstance = nullptr;
	}
#endif

	ActiveInstances.Remove(Instance);
	return ActiveInstances.Num();
}

void UFlowAsset::ClearInstances()
{
	for (UFlowAsset* Instance : ActiveInstances)
	{
		for (UFlowNode* Node : Instance->ActiveNodes)
		{
			Node->Cleanup();
		}

		Instance->ActiveNodes.Empty();
	}

	ActiveInstances.Empty();
}

void UFlowAsset::InitInstance(UFlowAsset* InTemplateAsset)
{
	TemplateAsset = InTemplateAsset;

	for (TPair<FGuid, UFlowNode*>& Node : Nodes)
	{
		UFlowNode* NewInstance = NewObject<UFlowNode>(this, Node.Value->GetClass(), NAME_None, RF_Transient, Node.Value, false, nullptr);
		Node.Value = NewInstance;

		if (UFlowNodeIn* InNode = Cast<UFlowNodeIn>(NewInstance))
		{
			InNodes.Add(InNode);
		}
	}
}

void UFlowAsset::PreloadNodes()
{
	// NOTE: this is just the example algorithm of gathering nodes for preload
	for (UFlowNodeIn* InNode : InNodes)
	{
		for (const TPair<TSubclassOf<UFlowNode>, int32>& Node : UFlowSettings::Get()->DefaultPreloadDepth)
		{
			if (Node.Value > 0)
			{
				TArray<UFlowNode*> FoundNodes;
				RecursiveFindNodesByClass(InNode, Node.Key, Node.Value, FoundNodes);

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

void UFlowAsset::FlushPreload()
{
	for (UFlowNode* PreloadedNode : PreloadedNodes)
	{
		PreloadedNode->TriggerFlush();
	}
	PreloadedNodes.Empty();
}

void UFlowAsset::StartFlow()
{
	ResetNodes();

	// when starting root flow, always call default In node
	for (UFlowNodeIn* Node : InNodes)
	{
		RecordedNodes.Add(Node);
		Node->TriggerFirstOutput(true);
		return;
	}
}

void UFlowAsset::StartSubFlow(UFlowNodeSubGraph* FlowNode)
{
	OwningFlowNode = FlowNode;
	FlowNode->GetFlowAsset()->AddChildFlow(FlowNode, this);

	ResetNodes();

	// todo: support selecting In node matching input on SubFlow node in parent graph
	for (UFlowNodeIn* Node : InNodes)
	{
		RecordedNodes.Add(Node);
		Node->TriggerFirstOutput(true);
		return;
	}
}

void UFlowAsset::AddChildFlow(UFlowNodeSubGraph* Node, UFlowAsset* Asset)
{
	ChildFlows.Add(Node, Asset);
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

		if (Node->GetClass()->IsChildOf(UFlowNodeOut::StaticClass()) && OwningFlowNode.IsValid())
		{
			OwningFlowNode.Get()->TriggerFirstOutput(true);
			OwningFlowNode = nullptr;
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