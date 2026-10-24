// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "FlowMCPToolsetTypes.h"

#include "FlowMCPToolset.generated.h"

/**
 * Read, search, and mutate Flow graphs (FlowAsset node graphs) via Courier v2 JSON documents.
 *
 * Research loop: ListFlowAssetTypes -> FindFlowNodeTypes (narrow by AssetClassName/Kind/Query to
 * "shape", then "brief", then "full") -> FindFlowNodeUsage for real examples -> FindFlowNodes to
 * locate instances on one asset. Use SearchFlowAssets for free-text search across comments,
 * classes, and property values instead of structural filtering.
 *
 * Edit loop: ExportFlowAsset/ExportFlowSubgraph to read current state, FindOrCreateFlowNode to get
 * a GUID without duplicating an existing node, then ApplyFlowPatch (bDryRun first) with a Courier
 * v2 JSON document scoped to just the nodes/connections/addons that change - untouched content
 * keeps identity, position, and content. AutoFormatFlowGraph after structural edits. DiffFlowAsset
 * for verification.
 *
 * Mutating ops take a Mutation (FFlowMCPMutationOptions) field. Where supported, set bDryRun to
 * preview a change without applying it (requires bUseTransaction true and no ambient transaction).
 * ImportAndRegraphFlowAsset creates a package and does not support a dry run.
 */
UCLASS(BlueprintType)
class FLOWGRAPHCOURIER_API UFlowMCPToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Export a FlowAsset as a full Courier v2 JSON document (every node, connection, and addon).
	 * @param Request FlowAsset object path.
	 * @return The JSON document text and its length.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPExportFlowAssetResult ExportFlowAsset(
		const FFlowMCPExportFlowAssetRequest& Request);

	/**
	 * Import a Courier v2 JSON document into a new FlowAsset and build its editor graph in one call.
	 * An occupied asset path is rejected. Use ApplyFlowPatch to edit an existing asset.
	 * @param Request Courier v2 JSON document, target path, and mutation policy.
	 * @return Imported asset metadata and mutation report.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPImportAndRegraphFlowAssetResult ImportAndRegraphFlowAsset(
		const FFlowMCPImportAndRegraphFlowAssetRequest& Request);

	/**
	 * List every FlowAsset subclass in this project with its allowed-class counts - the starting
	 * point for Flow graph discovery. Counts only, never the member name arrays; call
	 * FindFlowNodeTypes with AssetClassName set to see the actual allowed classes.
	 * @param Request Empty request struct.
	 * @return One summary row per concrete FlowAsset subclass.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPListFlowAssetTypesResult ListFlowAssetTypes(
		const FFlowMCPListFlowAssetTypesRequest& Request);

	/**
	 * Check whether an AddOn class is eligible to attach to a given node or addon class.
	 * @param Request Parent (node or addon) class name and addon class name.
	 * @return Whether the addon is eligible to attach, the handshake result, and a reason on rejection.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPCheckAddonAttachmentEligibilityResult CheckAddonAttachmentEligibility(
		const FFlowMCPCheckAddonAttachmentEligibilityRequest& Request);

	/**
	 * Search the project catalog of Flow node and addon CLASSES by asset type, keyword, or name -
	 * one filter-then-project query for both legality lists and deep-dives by exact name. Trap: the
	 * default Sections preset ("shape") returns histograms and no rows at all - pass "brief" or
	 * "full" to get classes back. Use ListFlowAssetTypes first if you don't yet know which FlowAsset
	 * subclass to scope to.
	 * @param Request Optional AssetClassName/ClassNames/Kind/Query/Keywords/Categories/Articles/PinTypes filters, a Sections list or preset ("shape"|"brief"|"full"), a deprecated opt-in, and paging.
	 * @return Matching classes split into Nodes/Addons, counts, the Shape histograms, and any unresolved or ambiguous ClassNames.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPFindFlowNodeTypesResult FindFlowNodeTypes(const FFlowMCPFindFlowNodeTypesRequest& Request);

	/**
	 * Write the hand-authored documentation layer (guidance, tags, article slugs) onto one
	 * node or addon class. Blueprint classes are checked out and saved; native and script classes
	 * cannot be authored at runtime, so those report Persisted false and return the source to paste
	 * instead.
	 * @param Request Target class, doc fields, and mutation policy.
	 * @return Where the doc landed, and a source snippet when a manual edit is needed.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPSetFlowAgentDocResult SetFlowAgentDoc(const FFlowMCPSetFlowAgentDocRequest& Request);

	/**
	 * Report where a node or addon CLASS is actually used across the project's Flow Assets, with the
	 * smallest containing assets first and the classes most often attached alongside it. Also the only
	 * view of pins contributed by an attached addon, which no class default object can report.
	 * @param Request Class identifier, example limit, and whether to include per-instance snippets.
	 * @return Usage counts, example assets, co-attached classes, and optional snippets.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPFindFlowNodeUsageResult FindFlowNodeUsage(const FFlowMCPFindFlowNodeUsageRequest& Request);

	/**
	 * Find node INSTANCES on one existing Flow Asset by title substring, class, and/or entry-point
	 * status - not to be confused with FindFlowNodeTypes, which searches node/addon CLASSES across
	 * the whole project rather than instances within one asset.
	 * @param Request Asset path and optional title/class/entry-point filters.
	 * @return Lightweight node summaries matching the filters.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPFindFlowNodesResult FindFlowNodes(const FFlowMCPFindFlowNodesRequest& Request);

	/**
	 * Export the connected region around a start node from a Flow Asset as a Courier v2 JSON
	 * document - undirected reachability, so it also picks up nodes only reachable upstream via a
	 * data pin.
	 * @param Request Asset path and the GUID of the node to start the reachability search from.
	 * @return JSON for just the reachable subgraph, independently re-importable.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPExportFlowSubgraphResult ExportFlowSubgraph(const FFlowMCPExportFlowSubgraphRequest& Request);

	/**
	 * Validate a top-level node selection for Flow's editor "create sub-graph from selection" operation.
	 * This is read-only and reports boundary interface names and all planner errors without opening UI.
	 * @param Request Source asset, selected node GUIDs, and proposed child asset name.
	 * @return Whether the selection can be collapsed and the generated interface summary.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPPlanFlowSubgraphFromSelectionResult PlanFlowSubgraphFromSelection(
		const FFlowMCPPlanFlowSubgraphFromSelectionRequest& Request);

	/**
	 * Collapse selected top-level nodes into a new Flow sub-graph asset and replace them with a
	 * SubGraph node. The operation is UI-free, reruns planning immediately before applying, and
	 * treats the source and generated child packages as one mutation. Mutation.bUseTransaction must
	 * be true, and the operation cannot run inside an existing editor transaction. Use
	 * Mutation.bDryRun first.
	 * @param Request Source asset, selected node GUIDs, child asset name, and mutation policy.
	 * @return Plan details, generated asset/node paths, and the mutation report.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPCreateFlowSubgraphFromSelectionResult CreateFlowSubgraphFromSelection(
		const FFlowMCPCreateFlowSubgraphFromSelectionRequest& Request);

	/**
	 * Find or create a single node of a given type whose properties exactly match MatchProperties.
	 * @param Request Target asset, node type, exact-match properties, and mutation policy.
	 * @return Whether a new node was created, and the matched/created node's GUID.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPFindOrCreateFlowNodeResult FindOrCreateFlowNode(const FFlowMCPFindOrCreateFlowNodeRequest& Request);

	/**
	 * Create a new Flow node or add-on Blueprint asset. Use this rather than a generic Blueprint
	 * creation tool: the palette and the catalog both gather by asset class, so a plain Blueprint
	 * parented to a Flow node compiles and is then invisible to both. The asset class is derived
	 * from ParentClass, not supplied - a parent under neither UFlowNode nor UFlowNodeAddOn is
	 * rejected rather than guessed at. Check AssetClassPath on the result, not just success.
	 * @param Request Destination, parent class, optional palette text, and mutation policy.
	 * @return The created asset's paths, its derived asset class, compile status, and whether the
	 *         new class resolves through the catalog.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPCreateFlowNodeBlueprintResult CreateFlowNodeBlueprint(const FFlowMCPCreateFlowNodeBlueprintRequest& Request);

	/**
	 * Auto-format a Flow Asset's node layout.
	 * @param Request Target asset, optional node-GUID selection, and mutation policy.
	 * @return Computed node positions and any selection GUIDs that did not resolve.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPAutoFormatFlowGraphResult AutoFormatFlowGraph(const FFlowMCPAutoFormatFlowGraphRequest& Request);

	/**
	 * Diff two Flow graphs (each a full asset path or a raw Courier v2 JSON document) and return a
	 * structured result.
	 * @param Request Old and New inputs to diff.
	 * @return Added/removed/changed nodes and connections.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPDiffFlowAssetResult DiffFlowAsset(const FFlowMCPDiffFlowAssetRequest& Request);

	/**
	 * Apply a scoped, GUID-keyed patch to a Flow Asset via the reconciler (the "scalpel").
	 * Only the nodes/connections/addons named in the Courier v2 JSON document change; untouched
	 * nodes keep identity, position, and content. Trap: a validation error blocks the whole apply
	 * and leaves the asset untouched, but Result still carries Plan/ValidationFindings - check
	 * bApplySucceeded, don't assume a lack of exception means it applied. Call
	 * DescribeCourierGrammar first if you are not certain which fields a given op Kind allows.
	 * @param Request Target asset, Courier v2 JSON mutation document, and mutation policy.
	 * @return The reconcile plan, validation findings, and mutation report.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPApplyFlowPatchResult ApplyFlowPatch(const FFlowMCPApplyFlowPatchRequest& Request);

	/**
	 * Replace a node's class while preserving its graph identity and mapped connections. Preview
	 * with Mutation.bDryRun and inspect Findings for properties, pins, and addons that cannot map.
	 * @param Request Asset and node GUID, new class, optional mappings, and mutation policy.
	 * @return Feasibility, class paths, affected addon GUIDs, findings, and mutation report.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPReplaceFlowNodeClassResult ReplaceFlowNodeClass(const FFlowMCPReplaceFlowNodeClassRequest& Request);

	/**
	 * Describe the Courier v2 op grammar: every legal FFlowCourierOp.Kind value, and for each,
	 * which fields are required, required only when creating, optional, or illegal to set. Read
	 * this before hand-authoring a Courier v2 document rather than inferring the grammar from
	 * example documents, which may not exercise every field.
	 * @param Request No fields; the grammar does not depend on any asset or document.
	 * @return Every op Kind with its per-field legality, plus top-level document field legality.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPDescribeCourierGrammarResult DescribeCourierGrammar(const FFlowMCPDescribeCourierGrammarRequest& Request);

	/**
	 * Search for Flow nodes across one or all Flow Assets.
	 * @param Request Search query, flags/scope bitfields, max recursion depth, context asset, and optional pin direction/connection filters.
	 * @return Matching nodes with category and matched-pin metadata.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPSearchFlowAssetsResult SearchFlowAssets(const FFlowMCPSearchFlowAssetsRequest& Request);

	/**
	 * Rebuild a FlowAsset's editor graph from its runtime nodes/connections. Trap: a freshly
	 * duplicated FlowAsset (via AssetTools.duplicate) exports connections still pointing at the
	 * source asset's node GUIDs, because duplication regenerates every node GUID but does not
	 * repoint connection targets - this repairs that without requiring the asset be opened in the
	 * Flow editor first.
	 * @param Request Target asset and mutation policy.
	 * @return The asset path, its node count after rebuilding, and the mutation report.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPReconstructFlowGraphResult ReconstructFlowGraph(
		const FFlowMCPReconstructFlowGraphRequest& Request);

	/**
	 * Export every node/addon class known to the catalog to a JSON file on disk, for offline
	 * analysis (e.g. kb_lint.py's --catalog bidirectional-linkage and stem-collision checks).
	 * @param Request Output file path and an optional FlowAsset class to scope the export to.
	 * @return The output file path and whether the export succeeded.
	 */
	UFUNCTION(meta = (AICallable), Category = "Flow Courier")
	static FFlowMCPExportFlowCatalogResult ExportFlowCatalog(
		const FFlowMCPExportFlowCatalogRequest& Request);
};
