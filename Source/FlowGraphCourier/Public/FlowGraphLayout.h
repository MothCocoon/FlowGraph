// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"

class UFlowAsset;
struct FFlowGraphParsedConnection;

/**
 * Deterministic, non-full-layout node placement for the reconciler's patch-apply path.
 * Not a graph layout algorithm - only answers "where should a brand-new node with no explicit
 * Pos: line land so it doesn't overlap anything and reads near its upstream source".
 *
 *  - New nodes without an explicit Pos are placed one column to the right of their primary
 *    upstream connection's source (first connection in AllConnections whose target is this node),
 *    at the same row; a new node with no upstream connection stacks in a default starting column.
 *  - Overlap avoidance nudges the new node's row downward until it clears every known position
 *    (pre-existing asset nodes, explicitly-declared Pos: entries in this same document, and other
 *    new nodes already placed earlier in this same call). Existing nodes are never moved.
 *
 * Full graph auto-format lives in FlowEditor's UFlowGraphEditorLayout.
 *
 * Plain static-method class, not a UObject: it has no UFUNCTIONs and is never instantiated.
 */
class FLOWGRAPHCOURIER_API FFlowGraphLayout
{
public:
	// Column/row spacing in editor graph units, exposed for tests that want to assert relative
	// placement without hardcoding these constants twice.
	static constexpr int32 ColumnSpacing = 300;
	static constexpr int32 RowSpacing = 150;

	/**
	 * For every GUID in NewNodeGuids not already present as a key in InOutPositions (i.e. it had
	 * no explicit "Pos:" line in the mutation document), computes and inserts a deterministic,
	 * non-overlapping position into InOutPositions. Positions already present in InOutPositions,
	 * and every pre-existing node's live editor position on ExistingAsset (if non-null), are read
	 * as fixed occupied anchors and never modified. AllConnections supplies upstream/downstream
	 * relationships (delete markers are ignored) to find each new node's primary upstream source.
	 * Iterates NewNodeGuids in the order given, so document order drives placement order and
	 * later new nodes can chain off earlier ones already placed this same call.
	 */
	static void ComputeAutoPlacedPositions(
		const UFlowAsset* ExistingAsset,
		const TArray<FGuid>& NewNodeGuids,
		const TArray<FFlowGraphParsedConnection>& AllConnections,
		TMap<FGuid, FIntPoint>& InOutPositions);

	// Full graph auto-format (UFlowGraphEditorLayout::ComputeAutoFormatPositions and
	// ApplyPositionsToExistingGraph) lives in FlowEditor rather than here because FlowGraphCourier
	// is a public dependency of FlowEditor, not the reverse - an editor-UI affordance cannot call
	// back into FlowGraphCourier without a cycle. ComputeAutoPlacedPositions is the one function
	// that genuinely needs FFlowGraphParsedConnection (a FlowGraphCourier type), which
	// UFlowGraphEditorLayout deliberately does not depend on.
};
