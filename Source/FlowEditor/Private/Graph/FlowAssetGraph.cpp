#include "Graph/FlowAssetGraph.h"

#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Graph/Nodes/FlowGraphNode_In.h"
#include "Graph/Nodes/FlowGraphNode_Out.h"
#include "Graph/Nodes/FlowGraphNode_Reroute.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Route/FlowNode_In.h"
#include "Nodes/Route/FlowNode_Out.h"
#include "Nodes/Route/FlowNode_Reroute.h"

#include "Kismet2/BlueprintEditorUtils.h"

class FFlowGraphInterface : public IFlowGraphInterface
{
public:
	FFlowGraphInterface()
	{
	}

	~FFlowGraphInterface()
	{
	}

	UEdGraph* CreateGraph(UFlowAsset* InFlowAsset) override
	{
		return CastChecked<UFlowAssetGraph>(FBlueprintEditorUtils::CreateNewGraph(InFlowAsset, NAME_None, UFlowAssetGraph::StaticClass(), UFlowGraphSchema::StaticClass()));
	}

	FGuid CreateGraphNode(UEdGraph* Graph, UFlowNode* FlowNode, const bool bSelectNewNode) override
	{
		// Node In
		if (FlowNode->GetClass()->IsChildOf(UFlowNode_In::StaticClass()))
		{
			FGraphNodeCreator<UFlowGraphNode_In> NodeCreator(*Graph);
			UFlowGraphNode* GraphNode = NodeCreator.CreateNode(bSelectNewNode);
			GraphNode->SetFlowNode(FlowNode);
			NodeCreator.Finalize();
			return GraphNode->NodeGuid;
		}
		
		// Node Out
		if (FlowNode->GetClass()->IsChildOf(UFlowNode_Out::StaticClass()))
		{
			FGraphNodeCreator<UFlowGraphNode_Out> NodeCreator(*Graph);
			UFlowGraphNode* GraphNode = NodeCreator.CreateNode(bSelectNewNode);
			GraphNode->SetFlowNode(FlowNode);
			NodeCreator.Finalize();
			return GraphNode->NodeGuid;
		}

		// Node Reroute
		if (FlowNode->GetClass()->IsChildOf(UFlowNode_Reroute::StaticClass()))
		{
			FGraphNodeCreator<UFlowGraphNode_Reroute> NodeCreator(*Graph);
			UFlowGraphNode* GraphNode = NodeCreator.CreateNode(bSelectNewNode);
			GraphNode->SetFlowNode(FlowNode);
			NodeCreator.Finalize();
			return GraphNode->NodeGuid;
		}
		
		// Every other node
		FGraphNodeCreator<UFlowGraphNode> NodeCreator(*Graph);
		UFlowGraphNode* GraphNode = NodeCreator.CreateNode(bSelectNewNode);
		GraphNode->SetFlowNode(FlowNode);
		NodeCreator.Finalize();
		return GraphNode->NodeGuid;
	}

	virtual void IFlowGraphInterface::OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index) override
	{
		if (GraphNode)
		{
			Cast<UFlowGraphNode>(GraphNode)->OnInputTriggered(Index);
		}
	}

	virtual void IFlowGraphInterface::OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index) override
	{
		if (GraphNode)
		{
			Cast<UFlowGraphNode>(GraphNode)->OnOutputTriggered(Index);
		}
	}
};

UFlowAssetGraph::UFlowAssetGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!UFlowAsset::GetFlowGraphInterface().IsValid())
	{
		UFlowAsset::SetFlowGraphInterface(MakeShared<FFlowGraphInterface>());
	}
}

void UFlowAssetGraph::NotifyGraphChanged()
{
	GetFlowAsset()->CompileNodeConnections();
	GetFlowAsset()->MarkPackageDirty();
	
	Super::NotifyGraphChanged();
}

UFlowAsset* UFlowAssetGraph::GetFlowAsset() const
{
	return CastChecked<UFlowAsset>(GetOuter());
}
