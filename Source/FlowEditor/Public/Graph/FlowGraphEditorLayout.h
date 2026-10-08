// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Object.h"
#include "FlowGraphEditorLayout.generated.h"

class UFlowAsset;

/** On-demand, user-triggered whole-graph or selection auto-format. */
UCLASS()
class FLOWEDITOR_API UFlowGraphEditorLayout : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Computes a deterministic, comment-aware, left-to-right layout. An empty target set formats
	 * the whole graph. This overload estimates node extents and is suitable for headless callers.
	 */
	static void ComputeAutoFormatPositions(
		const UFlowAsset* FlowAsset,
		const TSet<FGuid>& TargetGuids,
		TMap<FGuid, FIntPoint>& OutPositions);

	/**
	 * Geometry-aware form used by an open graph editor. MeasuredNodeSizes contains the Slate bounds
	 * available in the active panel; missing entries use a conservative pin/add-on based estimate.
	 */
	static void ComputeAutoFormatPositionsWithSizes(
		const UFlowAsset* FlowAsset,
		const TSet<FGuid>& TargetGuids,
		const TMap<FGuid, FIntPoint>& MeasuredNodeSizes,
		TMap<FGuid, FIntPoint>& OutPositions);

	/** Applies positions and refits fully targeted comment boxes around their original members. */
	static void ApplyPositionsToExistingGraph(
		UFlowAsset* FlowAsset,
		const TMap<FGuid, FIntPoint>& Positions);

	/** Applies positions and refits comments using node sizes measured from the active graph panel. */
	static void ApplyPositionsToExistingGraphWithSizes(
		UFlowAsset* FlowAsset,
		const TMap<FGuid, FIntPoint>& Positions,
		const TMap<FGuid, FIntPoint>& MeasuredNodeSizes);
};
