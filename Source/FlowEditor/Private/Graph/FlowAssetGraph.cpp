#include "FlowAssetGraph.h"
#include "Nodes/FlowGraphNode.h"
#include "Nodes/FlowGraphNode_In.h"
#include "Nodes/FlowGraphNode_Out.h"
#include "FlowGraphSchema.h"

#include "Graph/FlowAsset.h"
#include "Graph/Nodes/FlowNode.h"
#include "Graph/Nodes/FlowNodeIn.h"
#include "Graph/Nodes/FlowNodeOut.h"

#include "Kismet2/BlueprintEditorUtils.h"

class FFlowAssetEditorInterface : public IFlowAssetEditorInterface
{
public:
	FFlowAssetEditorInterface()
	{
	}

	~FFlowAssetEditorInterface()
	{
	}

	UEdGraph* CreateGraph(UFlowAsset* InFlowAsset) override
	{
		return CastChecked<UFlowAssetGraph>(FBlueprintEditorUtils::CreateNewGraph(InFlowAsset, NAME_None, UFlowAssetGraph::StaticClass(), UFlowGraphSchema::StaticClass()));
	}

	FGuid CreateGraphNode(UEdGraph* FlowGraph, UFlowNode* FlowNode, bool bSelectNewNode) override
	{
		// Node In
		if (UFlowNodeIn* NodeIn = Cast<UFlowNodeIn>(FlowNode))
		{
			FGraphNodeCreator<UFlowGraphNode_In> NodeCreator(*FlowGraph);
			UFlowGraphNode* GraphNode = NodeCreator.CreateNode(bSelectNewNode);
			GraphNode->SetFlowNode(FlowNode);
			NodeCreator.Finalize();
			return GraphNode->NodeGuid;
		}
		
		// Node Out
		if (UFlowNodeOut* NodeOut = Cast<UFlowNodeOut>(FlowNode))
		{
			FGraphNodeCreator<UFlowGraphNode_Out> NodeCreator(*FlowGraph);
			UFlowGraphNode* GraphNode = NodeCreator.CreateNode(bSelectNewNode);
			GraphNode->SetFlowNode(FlowNode);
			NodeCreator.Finalize();
			return GraphNode->NodeGuid;
		}
		
		// Every other node
		FGraphNodeCreator<UFlowGraphNode> NodeCreator(*FlowGraph);
		UFlowGraphNode* GraphNode = NodeCreator.CreateNode(bSelectNewNode);
		GraphNode->SetFlowNode(FlowNode);
		NodeCreator.Finalize();
		return GraphNode->NodeGuid;
	}

	void CompileNodeConnections(UFlowAsset* FlowAsset) override
	{
		for (TArray<UEdGraphNode*>::TIterator It(FlowAsset->GetGraph()->Nodes); It; ++It)
		{
			if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(*It))
			{
				if (UFlowNode* Node = GraphNode->GetFlowNode())
				{
					TMap<FName, FConnectedPin> Connections;

					for (uint8 i = 0; i < GraphNode->OutputPins.Num(); i++)
					{
						const UEdGraphPin* Pin = GraphNode->OutputPins[i];
						if (Pin->LinkedTo.Num() > 0)
						{
							const FName OutputPinName = Node->GetOutputName(i);

							UFlowGraphNode* LinkedGraphNode = CastChecked<UFlowGraphNode>(Pin->LinkedTo[0]->GetOwningNode());
							const FGuid NodeId = LinkedGraphNode->GetFlowNode()->GetGuid();
							const uint8 PinIndex = LinkedGraphNode->GetPinIndex(Pin->LinkedTo[0]);
							const FName PinName = LinkedGraphNode->GetFlowNode()->GetInputName(PinIndex);

							Connections.Add(OutputPinName, FConnectedPin(NodeId, PinIndex, PinName));
						}
					}

					Node->SetFlags(RF_Transactional);
					Node->Modify();
					Node->SetConnections(Connections);
					Node->PostEditChange();
				}
			}
		}
	}
};

UFlowAssetGraph::UFlowAssetGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!UFlowAsset::GetFlowAssetEditor().IsValid())
	{
		UFlowAsset::SetFlowAssetEditor(TSharedPtr<IFlowAssetEditorInterface>(new FFlowAssetEditorInterface()));
	}
}

UFlowAsset* UFlowAssetGraph::GetFlowAsset() const
{
	return CastChecked<UFlowAsset>(GetOuter());
}