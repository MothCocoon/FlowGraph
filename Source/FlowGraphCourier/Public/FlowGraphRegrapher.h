// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Object.h"
#include "FlowGraphImporter.h"
#include "FlowGraphRegrapher.generated.h"

class UFlowAsset;

/**
 * Helper class for rebuilding UEdGraph from UFlowAsset runtime nodes.
 * 
 * This class provides functionality to reconstruct the editor graph (UFlowGraph and UFlowGraphNodes)
 * from a UFlowAsset that contains runtime UFlowNode instances with connections.
 * 
 * Use case: After programmatically creating a UFlowAsset with UFlowNodes and setting up their
 * Connections maps, call RegraphFlowAsset() to generate the corresponding editor graph nodes
 * and wire them up properly.
 */
UCLASS()
class FLOWGRAPHCOURIER_API UFlowGraphRegrapher : public UObject
{
	GENERATED_BODY()

	friend class FFlowGraphReconciler;

public:
	/**
	 * Imports a FlowGraph from already-parsed nodes and connections (e.g. from
	 * FFlowCourierConverter::ConvertToParsedGraph) and creates the editor graph, restoring
	 * Pos/Comment on newly-created editor nodes from the same parsed data.
	 * Not BlueprintCallable - FFlowGraphParsedNode/FFlowGraphParsedConnection are C++-only types.
	 */
	static UFlowAsset* ImportAndRegraphFromDocument(
		const FString& AssetPath,
		const FString& AssetClassPath,
		bool bWorldBound,
		const TArray<FFlowGraphParsedNode>& ParsedNodes,
		const TArray<FFlowGraphParsedConnection>& ParsedConnections);

	/**
	 * Rebuilds the editor graph (UFlowGraph) from a UFlowAsset's runtime nodes.
	 * 
	 * This function:
	 * 1. Ensures the FlowAsset has a UFlowGraph
	 * 2. Creates UFlowGraphNodes for all UFlowNodes
	 * 3. Wires up the editor pins based on the runtime Connections maps
	 * 4. Calls HarvestNodeConnections() to ensure consistency
	 * 
	 * @param FlowAsset The FlowAsset to rebuild the graph for
	 * @return True if successful, false if FlowAsset is null or graph creation failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Flow Courier")
	static bool RegraphFlowAsset(UFlowAsset* FlowAsset);

	/**
	 * Reports every way the editor graph disagrees with the runtime data: an editor node whose
	 * runtime node is gone (a node still drawn but no longer executable), a runtime node with no
	 * editor node, a runtime connection whose endpoint is not in the node map, an editor link with
	 * no runtime connection (a wire drawn but dead), and a runtime connection with no editor link
	 * (a wire that executes but is invisible).
	 *
	 * A caller that verifies a mutation by re-reading the runtime map alone cannot see any of these -
	 * the map is what the mutation wrote, so it agrees with itself by construction. Report this
	 * alongside such a read rather than treating an unchanged map as proof the asset is intact.
	 *
	 * @param FlowAsset The FlowAsset to inspect
	 * @param OutIssues One human-readable line per disagreement; empty means editor and runtime agree
	 */
	static void CollectGraphParityIssues(const UFlowAsset* FlowAsset, TArray<FString>& OutIssues);

	/**
	 * Destroys any top-level UFlowGraphNode whose runtime UFlowNode is no longer registered on the
	 * asset, breaking its links first. RegraphFlowAsset runs this before pin wiring so a stale link
	 * cannot be harvested back into the runtime map; it is exposed separately so a caller can report
	 * how much pre-existing damage a rebuild repaired. Comment nodes and addon sub-nodes are never
	 * touched - they have no entry in the runtime node map by design.
	 *
	 * @param FlowAsset The FlowAsset to prune
	 * @return Number of orphaned editor nodes destroyed
	 */
	static int32 PruneOrphanedEditorNodes(UFlowAsset* FlowAsset);

private:
	/**
	 * Deletes the editor node representing a runtime node, breaking its pin links first.
	 *
	 * Removing a node from UFlowAsset::Nodes without this leaves its UFlowGraphNode in the graph: it
	 * is still drawn, still holds the removed UFlowNode alive in the package, and its surviving pin
	 * links are harvested back into the runtime Connections map by the next regraph. Mirrors the
	 * editor's own delete (SFlowGraphEditor::DeleteSelectedNodes) so both halves go together.
	 *
	 * @param FlowAsset The FlowAsset owning the graph
	 * @param NodeGuid Runtime GUID of the node whose editor representation should be destroyed
	 * @return True if an editor node was found and destroyed
	 */
	static bool DestroyEditorNodeForRuntimeNode(UFlowAsset* FlowAsset, const FGuid& NodeGuid);

	// Same as RegraphFlowAsset, but applies NodePositions (keyed by runtime node GUID) to any
	// newly-created UFlowGraphNode. Nodes with no entry in the map are left at the default position.
	// Pre-existing editor nodes are never repositioned.
	static bool RegraphFlowAsset(UFlowAsset* FlowAsset, const TMap<FGuid, FIntPoint>& NodePositions);

	// Full internal overload: applies both positions and comment text. NodeComments entries are
	// written to UEdGraphNode::NodeComment on newly-created nodes (WITH_EDITOR only).
	static bool RegraphFlowAsset(UFlowAsset* FlowAsset, const TMap<FGuid, FIntPoint>& NodePositions, const TMap<FGuid, FString>& NodeComments);

};
