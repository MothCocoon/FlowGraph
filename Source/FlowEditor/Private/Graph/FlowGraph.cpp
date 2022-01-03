#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/Nodes/FlowGraphNode.h"

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
	UFlowGraph* NewGraph = CastChecked<UFlowGraph>(FBlueprintEditorUtils::CreateNewGraph(InFlowAsset, NAME_None, StaticClass(), UFlowGraphSchema::StaticClass()));
	NewGraph->bAllowDeletion = false;

	return OnCreateGraph(InFlowAsset, NewGraph);
}

UEdGraph* UFlowGraph::OnCreateGraph(UFlowAsset* InFlowAsset, UFlowGraph* InFlowGraph)
{
	InFlowAsset->FlowGraph = InFlowGraph;
	InFlowGraph->GetSchema()->CreateDefaultNodesForGraph(*InFlowGraph);
	return InFlowGraph;
}

void UFlowGraph::NotifyGraphChanged()
{
	GetFlowAsset()->HarvestNodeConnections();
	GetFlowAsset()->MarkPackageDirty();

	Super::NotifyGraphChanged();
}

UFlowAsset* UFlowGraph::GetFlowAsset() const
{
	return CastChecked<UFlowAsset>(GetOuter());
}
