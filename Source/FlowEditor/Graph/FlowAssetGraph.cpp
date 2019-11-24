#include "FlowAssetGraph.h"
#include "Nodes/FlowGraphNode.h"
#include "Nodes/FlowGraphNode_In.h"
#include "Nodes/FlowGraphNode_Out.h"
#include "FlowGraphSchema.h"

#include "Flow/Graph/FlowAsset.h"
#include "Flow/Graph/Nodes/FlowNode.h"
#include "Flow/Graph/Nodes/FlowNodeIn.h"
#include "Flow/Graph/Nodes/FlowNodeOut.h"

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
			UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(*It);
			if (UFlowNode* Node = GraphNode->GetFlowNode())
			{
				TMap<uint8, FFlowPin> Connections;

				TArray<UEdGraphPin*> OutputPins;
				GraphNode->GetOutputPins(OutputPins);
				for (uint8 i = 0; i < OutputPins.Num(); i++)
				{
					const UEdGraphPin* Pin = OutputPins[i];
					if (Pin->LinkedTo.Num() > 0)
					{
						UFlowGraphNode* GraphChildNode = CastChecked<UFlowGraphNode>(Pin->LinkedTo[0]->GetOwningNode());
						const FGuid NodeId = GraphChildNode->GetFlowNode()->GetGuid();
						const uint8 PinIndex = GraphChildNode->GetPinIndex(Pin->LinkedTo[0]);

						Connections.Add(i, FFlowPin(NodeId, PinIndex));
					}
				}

				Node->SetFlags(RF_Transactional);
				Node->Modify();
				Node->SetConnections(Connections);
				Node->PostEditChange();
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
