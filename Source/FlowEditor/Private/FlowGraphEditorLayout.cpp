// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphEditorLayout.h"

#include "Graph/FlowGraphEditorLayoutAlgorithm.h"

void UFlowGraphEditorLayout::ComputeAutoFormatPositions(
	const UFlowAsset* FlowAsset,
	const TSet<FGuid>& TargetGuids,
	TMap<FGuid, FIntPoint>& OutPositions)
{
	UE::FlowGraph::Private::FFlowGraphEditorLayoutAlgorithm::Compute(
		FlowAsset, TargetGuids, TMap<FGuid, FIntPoint>(), OutPositions);
}

void UFlowGraphEditorLayout::ComputeAutoFormatPositionsWithSizes(
	const UFlowAsset* FlowAsset,
	const TSet<FGuid>& TargetGuids,
	const TMap<FGuid, FIntPoint>& MeasuredNodeSizes,
	TMap<FGuid, FIntPoint>& OutPositions)
{
	UE::FlowGraph::Private::FFlowGraphEditorLayoutAlgorithm::Compute(
		FlowAsset, TargetGuids, MeasuredNodeSizes, OutPositions);
}

void UFlowGraphEditorLayout::ApplyPositionsToExistingGraph(
	UFlowAsset* FlowAsset,
	const TMap<FGuid, FIntPoint>& Positions)
{
	UE::FlowGraph::Private::FFlowGraphEditorLayoutAlgorithm::Apply(
		FlowAsset, Positions, TMap<FGuid, FIntPoint>());
}

void UFlowGraphEditorLayout::ApplyPositionsToExistingGraphWithSizes(
	UFlowAsset* FlowAsset,
	const TMap<FGuid, FIntPoint>& Positions,
	const TMap<FGuid, FIntPoint>& MeasuredNodeSizes)
{
	UE::FlowGraph::Private::FFlowGraphEditorLayoutAlgorithm::Apply(
		FlowAsset, Positions, MeasuredNodeSizes);
}
