#include "FlowAsset.h"
#include "../FlowSubsystem.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeIn.h"
#include "Nodes/FlowNodeOut.h"
#include "Nodes/FlowNodeSubFlow.h"

#if WITH_EDITOR
#include "FlowEditor/Graph/FlowAssetGraph.h"
#include "FlowEditor/Graph/FlowGraphSchema.h"

TSharedPtr<IFlowAssetEditorInterface> UFlowAsset::FlowGraphEditor = nullptr;
#endif

UFlowAsset::UFlowAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void UFlowAsset::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
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
		FlowGraph = UFlowAsset::GetFlowAssetEditor()->CreateGraph(this);

		FlowGraph->bAllowDeletion = false;
		FlowGraph->GetSchema()->CreateDefaultNodesForGraph(*FlowGraph);
	}
}

FGuid UFlowAsset::CreateGraphNode(UFlowNode* InFlowNode, bool bSelectNewNode /*= true*/)
{
	check(InFlowNode->GraphNode == nullptr);
	return UFlowAsset::GetFlowAssetEditor()->CreateGraphNode(FlowGraph, InFlowNode, bSelectNewNode);
}

void UFlowAsset::CompileNodeConnections()
{
	UFlowAsset::GetFlowAssetEditor()->CompileNodeConnections(this);
}

void UFlowAsset::SetFlowAssetEditor(TSharedPtr<IFlowAssetEditorInterface> InFlowAssetEditor)
{
	check(!FlowGraphEditor.IsValid());
	FlowGraphEditor = InFlowAssetEditor;
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

void UFlowAsset::ClearInstances()
{
	ActiveInstances.Empty();
}

void UFlowAsset::CreateNodeInstances()
{
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

void UFlowAsset::StartFlow(UFlowSubsystem* Subsystem)
{
	FlowSubsystem = Subsystem;

	ResetNodes();

	// when starting root flow, always call default In node
	for (UFlowNodeIn* Node : InNodes)
	{
		ActivateInput(Node, 0);
		return;
	}
}

void UFlowAsset::StartSubFlow(UFlowSubsystem* Subsystem, UFlowNodeSubFlow* FlowNode, UFlowAsset* ParentAsset)
{
	FlowSubsystem = Subsystem;

	OwningFlowNode = FlowNode;
	ParentAsset->AddChildFlow(FlowNode, this);

	ResetNodes();

	// todo: support choosing In node matching input on SubFlow node in parent graph
	for (UFlowNodeIn* Node : InNodes)
	{
		ActivateInput(Node, 0);
		return;
	}
}

void UFlowAsset::AddChildFlow(UFlowNodeSubFlow* Node, const UFlowAsset* Asset)
{
	ChildFlows.Add(Node, Asset);
}

void UFlowAsset::ActivateInput(UFlowNode* Node, const uint8 Pin)
{
	ensure(Node);

	if (!ActiveNodes.Contains(Node))
	{
		ActiveNodes.Add(Node);
		RecordedNodes.Add(Node);
	}

	Node->RecordInput(Pin);
	Node->ExecuteInput(Pin);
}

void UFlowAsset::ActivateInput(const FGuid& NodeGuid, const uint8 Pin)
{
	if (UFlowNode* Node = Nodes.FindRef(NodeGuid))
	{
		ActivateInput(Node, Pin);
	}
}

void UFlowAsset::FinishNode(UFlowNode* Node)
{
	if (ActiveNodes.Contains(Node))
	{
		ActiveNodes.Remove(Node);

		if (UFlowNodeOut* NodeOut = Cast<UFlowNodeOut>(Node))
		{
			if (OwningFlowNode.IsValid())
			{
				OwningFlowNode.Get()->ExecuteOutput(0);
				OwningFlowNode = nullptr;
			}
			return;
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
