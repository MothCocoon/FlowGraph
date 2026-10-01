// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphLayout.h"
#include "FlowGraphImporter.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/FlowPin.h"

// Returns true when two positions are close enough to count as a collision. The tolerance is
// half the corresponding spacing in each axis, so a candidate has to land meaningfully close to a
// known position - not just share an axis value by coincidence.
static bool IsNearlySamePosition(const FIntPoint& PositionA, const FIntPoint& PositionB)
{
	return FMath::Abs(PositionA.X - PositionB.X) < FFlowGraphLayout::ColumnSpacing / 2
		&& FMath::Abs(PositionA.Y - PositionB.Y) < FFlowGraphLayout::RowSpacing / 2;
}

void FFlowGraphLayout::ComputeAutoPlacedPositions(
	const UFlowAsset* ExistingAsset,
	const TArray<FGuid>& NewNodeGuids,
	const TArray<FFlowGraphParsedConnection>& AllConnections,
	TMap<FGuid, FIntPoint>& InOutPositions)
{
	// Known/occupied positions this call must never land a new node on top of: every pre-existing
	// node's live editor position, plus every position already in InOutPositions (explicit Pos:
	// entries from the mutation document). Grows as each new node is placed, so later new nodes in
	// NewNodeGuids see earlier ones too.
	TArray<FIntPoint> KnownPositions;
	TMap<FGuid, FIntPoint> ResolvedPositions = InOutPositions;

	if (ExistingAsset)
	{
		for (const TPair<FGuid, UFlowNode*>& Pair : ExistingAsset->GetNodes())
		{
			if (const UFlowNode* Node = Pair.Value)
			{
				if (const UEdGraphNode* GraphNode = Node->GetGraphNode())
				{
					const FIntPoint Pos(GraphNode->NodePosX, GraphNode->NodePosY);
					KnownPositions.Add(Pos);
					ResolvedPositions.Add(Pair.Key, Pos);
				}
			}
		}
	}

	for (const TPair<FGuid, FIntPoint>& Pair : InOutPositions)
	{
		KnownPositions.Add(Pair.Value);
	}

	// Default stacking column/row for a new node with no upstream connection to chain off - starts
	// below the origin so it doesn't collide with a typical entry node placed at (0,0).
	int32 NextIsolatedY = RowSpacing;

	for (const FGuid& NewNodeGuid : NewNodeGuids)
	{
		if (InOutPositions.Contains(NewNodeGuid))
		{
			// Explicit Pos: already given for this node - never override it.
			continue;
		}

		// Primary upstream connection: first non-delete-marker entry (document order) whose target
		// is this new node.
		const FFlowGraphParsedConnection* Upstream = AllConnections.FindByPredicate(
			[&NewNodeGuid](const FFlowGraphParsedConnection& Connection)
			{
				return !Connection.bIsDeleteMarker && Connection.TargetNodeGuid == NewNodeGuid;
			});

		FIntPoint Candidate;
		if (Upstream)
		{
			if (const FIntPoint* SourcePos = ResolvedPositions.Find(Upstream->SourceNodeGuid))
			{
				Candidate = FIntPoint(SourcePos->X + ColumnSpacing, SourcePos->Y);
			}
			else
			{
				// Source has no known position either (e.g. also new-without-Pos but not yet
				// processed, or referenced by an alias this function doesn't resolve) - fall back
				// to the isolated stacking column rather than guessing.
				Candidate = FIntPoint(0, NextIsolatedY);
				NextIsolatedY += RowSpacing;
			}
		}
		else
		{
			Candidate = FIntPoint(0, NextIsolatedY);
			NextIsolatedY += RowSpacing;
		}

		// Nudge down until clear of every known position - existing nodes are never moved; only
		// the new node's own row is adjusted (see class comment for why this is sufficient here).
		while (KnownPositions.ContainsByPredicate([&Candidate](const FIntPoint& Occupied) { return IsNearlySamePosition(Candidate, Occupied); }))
		{
			Candidate.Y += RowSpacing;
		}

		InOutPositions.Add(NewNodeGuid, Candidate);
		ResolvedPositions.Add(NewNodeGuid, Candidate);
		KnownPositions.Add(Candidate);
	}
}
