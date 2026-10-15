// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"

class UFlowAsset;

namespace UE::FlowGraph::Private
{
class FFlowGraphEditorLayoutAlgorithm
{
public:
	static void Compute(
		const UFlowAsset* FlowAsset,
		const TSet<FGuid>& TargetGuids,
		const TMap<FGuid, FIntPoint>& MeasuredNodeSizes,
		TMap<FGuid, FIntPoint>& OutPositions);

	static void Apply(
		UFlowAsset* FlowAsset,
		const TMap<FGuid, FIntPoint>& Positions,
		const TMap<FGuid, FIntPoint>& MeasuredNodeSizes);
};
} // namespace UE::FlowGraph::Private
