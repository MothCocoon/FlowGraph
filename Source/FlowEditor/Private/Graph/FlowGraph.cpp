// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "Nodes/FlowNode.h"

#include "Kismet2/BlueprintEditorUtils.h"

void FFlowGraphInterface::OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index) const
{
	CastChecked<UFlowGraphNode>(GraphNode)->OnInputTriggered(Index);
}

void FFlowGraphInterface::OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index) const
{
	CastChecked<UFlowGraphNode>(GraphNode)->OnOutputTriggered(Index);
}

UFlowGraph::UFlowGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!UFlowAsset::GetFlowGraphInterface().IsValid())
	{
		UFlowAsset::SetFlowGraphInterface(MakeShared<FFlowGraphInterface>());
	}
}

UEdGraph* UFlowGraph::CreateGraph(UFlowAsset* InFlowAsset)
{
	UEdGraph* NewGraph = CastChecked<UFlowGraph>(FBlueprintEditorUtils::CreateNewGraph(InFlowAsset, NAME_None, StaticClass(), UFlowGraphSchema::StaticClass()));
	NewGraph->bAllowDeletion = false;

	InFlowAsset->FlowGraph = NewGraph;
	NewGraph->GetSchema()->CreateDefaultNodesForGraph(*NewGraph);

	return NewGraph;
}

void UFlowGraph::RefreshGraph()
{
	// don't run fixup in PIE
	if (GEditor && !GEditor->PlayWorld)
	{
		// check if all Graph Nodes have expected, up-to-date type
		CastChecked<UFlowGraphSchema>(GetSchema())->GatherNativeNodes();
		for (const TPair<FGuid, UFlowNode*>& Node : GetFlowAsset()->GetNodes())
		{
			if (UFlowNode* FlowNode = Node.Value)
			{
				const UClass* ExpectGraphNodeClass = UFlowGraphSchema::GetAssignedGraphNodeClass(FlowNode->GetClass());
				if (FlowNode->GetGraphNode() && FlowNode->GetGraphNode()->GetClass() != ExpectGraphNodeClass)
				{
					// Create a new Flow Graph Node of proper type
					FFlowGraphSchemaAction_NewNode::RecreateNode(this, FlowNode->GetGraphNode(), FlowNode);
				}
			}
		}

		// refresh nodes
		TArray<UFlowGraphNode*> FlowGraphNodes;
		GetNodesOfClass<UFlowGraphNode>(FlowGraphNodes);
		for (UFlowGraphNode* GraphNode : FlowGraphNodes)
		{
			GraphNode->OnGraphRefresh();
		}
	}
}

void UFlowGraph::NotifyGraphChanged()
{
	GetFlowAsset()->HarvestNodeConnections();
	GetFlowAsset()->MarkPackageDirty();

	Super::NotifyGraphChanged();
}

UFlowAsset* UFlowGraph::GetFlowAsset() const
{
	return GetTypedOuter<UFlowAsset>();
}
