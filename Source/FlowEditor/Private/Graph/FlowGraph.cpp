// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

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

	InFlowAsset->FlowGraph = NewGraph;
	NewGraph->GetSchema()->CreateDefaultNodesForGraph(*NewGraph);

	return NewGraph;
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
