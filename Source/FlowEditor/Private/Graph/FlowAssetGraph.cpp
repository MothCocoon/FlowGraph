#include "FlowAssetGraph.h"
#include "Nodes/FlowGraphNode.h"
#include "Nodes/FlowGraphNode_In.h"
#include "Nodes/FlowGraphNode_Out.h"
#include "Nodes/FlowGraphNode_Reroute.h"
#include "FlowGraphSchema.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeIn.h"
#include "Nodes/FlowNodeOut.h"
#include "Nodes/FlowNodeReroute.h"

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

	FGuid CreateGraphNode(UEdGraph* Graph, UFlowNode* FlowNode, bool bSelectNewNode) override
	{
		// Node In
		if (UFlowNodeIn* NodeIn = Cast<UFlowNodeIn>(FlowNode))
		{
			FGraphNodeCreator<UFlowGraphNode_In> NodeCreator(*Graph);
			UFlowGraphNode* GraphNode = NodeCreator.CreateNode(bSelectNewNode);
			GraphNode->SetFlowNode(FlowNode);
			NodeCreator.Finalize();
			return GraphNode->NodeGuid;
		}
		
		// Node Out
		if (UFlowNodeOut* NodeOut = Cast<UFlowNodeOut>(FlowNode))
		{
			FGraphNodeCreator<UFlowGraphNode_Out> NodeCreator(*Graph);
			UFlowGraphNode* GraphNode = NodeCreator.CreateNode(bSelectNewNode);
			GraphNode->SetFlowNode(FlowNode);
			NodeCreator.Finalize();
			return GraphNode->NodeGuid;
		}

		// Node Reroute
		if (UFlowNodeReroute* NodeOut = Cast<UFlowNodeReroute>(FlowNode))
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

	void IFlowGraphInterface::OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index)
	{
		if (GraphNode)
		{
			Cast<UFlowGraphNode>(GraphNode)->OnInputTriggered(Index);
		}
	}

	void IFlowGraphInterface::OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index)
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
		UFlowAsset::SetFlowGraphInterface(TSharedPtr<IFlowGraphInterface>(new FFlowGraphInterface()));
	}
}

UFlowAsset* UFlowAssetGraph::GetFlowAsset() const
{
	return CastChecked<UFlowAsset>(GetOuter());
}