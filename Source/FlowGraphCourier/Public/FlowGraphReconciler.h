// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Object.h"
#include "FlowGraphValidation.h"
#include "FlowGraphReconciler.generated.h"

class UFlowAsset;
class UFlowNode;
class UFlowNodeBase;
struct FFlowGraphParsedNode;
struct FFlowGraphParsedNodeAddOn;
struct FFlowGraphParsedConnection;

/**
 * One addon touched by a reconcile, reported in FFlowReconcilePlan. Carries enough to let a caller
 * understand the change without re-exporting the asset: which node owns the addon, the addon's
 * GUID and class, and the properties the mutation document set on it.
 *
 * Reported flat (not nested): a change to a nested addon-of-addon lands here keyed by its own
 * OwnerNodeGuid, which for a nested addon is the GUID of the parent addon it hangs off of (every
 * UFlowNodeBase - node or addon - has a GUID).
 */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowReconcileAddonEntry
{
	GENERATED_BODY()

	// GUID of the owner this addon attaches to: the node GUID for a top-level addon, or the parent
	// addon's GUID for a nested addon-of-addon.
	UPROPERTY()
	FGuid OwnerNodeGuid;

	UPROPERTY()
	FGuid AddOnGuid;

	// Resolved class path. Populated for added/updated addons from the mutation document; for a
	// deleted addon it is read from the live object being removed (empty only if unavailable).
	UPROPERTY()
	FString AddOnType;

	// Properties the mutation document set on this addon: for an add this is the initial state, for
	// an update the delta that was written. Empty for a delete.
	UPROPERTY()
	TMap<FString, FString> Properties;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowReconcilePlan
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGuid> NodesAdded;

	UPROPERTY()
	TArray<FGuid> NodesUpdated;

	UPROPERTY()
	TArray<FGuid> NodesDeleted;

	// Rendered as "SourceGuid.SourcePin -> TargetGuid.TargetPin", matching the export grammar.
	// Only genuinely-new connections are listed here - a connection already present on the asset
	// is not reported as added (idempotency), even though re-applying it is harmless.
	UPROPERTY()
	TArray<FString> ConnectionsAdded;

	// Same rendering as ConnectionsAdded. Only populated for a delete marker that matched a
	// connection actually present on the asset (idempotent: deleting an absent one is a no-op).
	UPROPERTY()
	TArray<FString> ConnectionsRemoved;

	UPROPERTY()
	TArray<FFlowReconcileAddonEntry> AddOnsAdded;

	UPROPERTY()
	TArray<FFlowReconcileAddonEntry> AddOnsUpdated;

	UPROPERTY()
	TArray<FFlowReconcileAddonEntry> AddOnsDeleted;

	bool IsEmpty() const
	{
		return NodesAdded.IsEmpty() && NodesUpdated.IsEmpty() && NodesDeleted.IsEmpty()
			&& ConnectionsAdded.IsEmpty() && ConnectionsRemoved.IsEmpty()
			&& AddOnsAdded.IsEmpty() && AddOnsUpdated.IsEmpty() && AddOnsDeleted.IsEmpty();
	}
};

/**
 * Output of ComputeReconcilePlan. Contains the human-readable summary (Plan, AliasMap,
 * ValidationFindings) for inspection and confirmation, plus the resolved execution data
 * (parsed nodes/connections classified into add/update/delete) that ExecuteReconcilePlan
 * consumes without re-parsing the mutation text.
 *
 * For a new-asset mutation (no existing asset at TargetAssetPath), bIsNewAsset is true and
 * only NodesToAdd and AllParsedNodes are populated; ExecuteReconcilePlan runs a full import.
 */
struct FLOWGRAPHCOURIER_API FFlowReconcileExecutionPlan
{
	// Human-readable summary of what will change.
	FFlowReconcilePlan Plan;

	// Document-local "new:<alias>" tokens resolved to the GUIDs minted for them.
	TMap<FString, FGuid> AliasMap;

	// Structured pre-commit validation results. An Error-severity finding blocks execution.
	TArray<FFlowValidationFinding> ValidationFindings;

	// Metrics computed at plan time.
	int32 NodesTouched = 0;
	int32 NodesPreserved = 0;
	int32 InputSizeBytes = 0;

	// True when no existing asset was found at TargetAssetPath; ExecuteReconcilePlan will
	// run a full import rather than a diff apply.
	bool bIsNewAsset = false;

	// Document header fields, needed by the new-asset import path (only valid when bIsNewAsset
	// is true) to create the right asset class.
	FString AssetClassPath;
	bool bWorldBound = false;

	// Execution data: resolved and classified nodes/connections ready to apply.
	TArray<FFlowGraphParsedNode> NodesToAdd;
	TArray<FFlowGraphParsedNode> NodesToUpdate;
	TArray<FGuid> NodesToDelete;
	TArray<FFlowGraphParsedConnection> ConnectionsToAdd;
	TArray<FFlowGraphParsedConnection> ConnectionsToRemove;

	// Full parsed lists needed by downstream helpers (SetupConnections, RegraphFlowAsset).
	TArray<FFlowGraphParsedNode> AllParsedNodes;
	TArray<FFlowGraphParsedConnection> AllParsedConnections;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowReconcileResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bSuccess = false;

	UPROPERTY()
	FFlowReconcilePlan Plan;

	// Document-local "new:<alias>" tokens resolved to the GUID minted for them. The bare "NEW"
	// sentinel (no alias) mints a GUID too but is never keyed here (nothing to key it by) - find
	// it via Plan.NodesAdded instead.
	UPROPERTY()
	TMap<FString, FGuid> AliasMap;

	// Plain diagnostic messages for parse/apply failures (low-level, unstructured). The structured
	// validation model lives in ValidationFindings below.
	UPROPERTY()
	TArray<FString> Findings;

	// Structured pre-commit validation results. An Error-severity finding blocks execution;
	// warnings/info are surfaced but do not block.
	UPROPERTY()
	TArray<FFlowValidationFinding> ValidationFindings;

	UPROPERTY()
	int32 NodesTouched = 0;

	UPROPERTY()
	int32 NodesPreserved = 0;

	UPROPERTY()
	int32 InputSizeBytes = 0;
};

/**
 * GUID-keyed minimal-diff patch pipeline for UFlowAsset.
 *
 * Usage: call ComputeReconcilePlan to parse and classify the mutation document, then inspect
 * the returned FFlowReconcileExecutionPlan (Plan summary, ValidationFindings, AliasMap) before
 * calling ExecuteReconcilePlan to commit. Both steps are required; there is no combined path.
 *
 *  - Node property update is a MERGE: only properties listed in an UpsertNode op's Properties
 *    change; everything else on the node (UObject identity, editor position) is left alone.
 *  - Node add via an UpsertNode op with newAlias set (mints a fresh GUID, returned via AliasMap).
 *  - Node delete via DeleteNode (also drops connections referencing that node). Idempotent.
 *  - Connection add/remove, both idempotent.
 *  - AddOns on existing nodes: matched GUID updates in place; unmatched GUID creates; a DeleteAddon
 *    op removes. Unmentioned addons are left alone unless the owning UpsertNode op sets bReplaceAddons.
 *
 * Plain static-method class, not a UObject: it has no UFUNCTIONs and is never instantiated.
 */
class FLOWGRAPHCOURIER_API FFlowGraphReconciler
{
public:
	/**
	 * Parses CourierDocumentJson (Courier v2 JSON; see agent-docs/CourierTextFormat.md) against the
	 * existing asset at TargetAssetPath (if any), classifies all changes, resolves aliases, and
	 * runs pre-commit validation. Returns nullptr only on a hard parse failure that makes the
	 * document meaningless (malformed JSON, wrong formatVersion) - OutErrorMessage set. Every
	 * other problem (structural issues from FFlowCourierConverter, semantic issues from
	 * FFlowGraphValidation) is returned as data in the plan's ValidationFindings rather than by
	 * failing this call; an Error-severity finding means ExecuteReconcilePlan will refuse to apply.
	 */
	static TSharedPtr<FFlowReconcileExecutionPlan> ComputeReconcilePlan(
		const FString& TargetAssetPath,
		const FString& CourierDocumentJson,
		FString& OutErrorMessage);

	/**
	 * Applies a plan previously returned by ComputeReconcilePlan. Returns a result with
	 * bSuccess=false (and OutErrorMessage set) if any Error-severity ValidationFinding is present
	 * or if the apply itself fails. Does not re-parse or re-validate; the plan is consumed as-is.
	 */
	static FFlowReconcileResult ExecuteReconcilePlan(
		const FFlowReconcileExecutionPlan& ExecutionPlan,
		const FString& TargetAssetPath,
		FString& OutErrorMessage);

	/**
	 * Idempotent "ensure a node of this type/properties exists" helper: finds an existing node of
	 * NodeType whose properties exactly match MatchProperties, or creates exactly one new node if
	 * none matches.
	 *
	 * Match is exact, not superset/fuzzy: every key in MatchProperties must equal the node's
	 * current property value (via UFlowGraphExporter::GetPropertyValueAsString). A key naming a
	 * property NodeType does not have is a hard error, not a silent non-match. If multiple nodes
	 * match, the one with the lexicographically lowest FGuid::ToString() is returned.
	 *
	 * On no match, the new node is created (transaction-wrapped) via the same
	 * ComputeReconcilePlan/ExecuteReconcilePlan pipeline apply_flow_patch uses, so it gets the same
	 * validation, auto-placement, and regraph.
	 *
	 * TargetAssetPath must already exist; this does not also create the asset.
	 */
	static bool FindOrCreateNode(
		const FString& TargetAssetPath,
		const FString& NodeType,
		const TMap<FString, FString>& MatchProperties,
		bool bDryRun,
		FGuid& OutNodeGuid,
		bool& OutCreated,
		FString& OutErrorMessage);

	// Finds the UFlowAsset at TargetAssetPath: checks in-memory (FindObject) then disk (LoadObject).
	// Returns nullptr if the asset does not exist yet. A caller resolving an asset that may have
	// just been created in the same call, and so may not be saved to disk yet, must use this
	// instead of a disk-only load or it will incorrectly treat the asset as missing.
	static UFlowAsset* FindOrLoadFlowAsset(const FString& TargetAssetPath);

	// Checks each connection in ExpectedConnections against FlowAsset's live Connections maps,
	// using the same source/key-pin logic ClassifyParsedConnections uses to detect "present".
	// Rendered as "SourceGuid.SourcePin -> TargetGuid.TargetPin", split into those verified as
	// actually wired (OutVerified) and those that did not land (OutNotLanded). The write paths
	// this guards (SetupConnections and a full import) do not themselves guarantee every
	// requested connection took, so a caller reporting success must check here
	// rather than trust the write call's return value alone.
	static void VerifyConnectionsLanded(
		UFlowAsset* FlowAsset,
		const TArray<FFlowGraphParsedConnection>& ExpectedConnections,
		TArray<FString>& OutVerified,
		TArray<FString>& OutNotLanded);

private:
	// Removes NodeGuidToRemove from the asset's Nodes map (reflection) and drops any connection
	// entry, on any other node, whose FConnectedPin references it. Returns false only if FlowAsset
	// or the reflection properties are invalid; removing an absent GUID is a no-op success.
	static bool RemoveNodeAndReferencingConnections(UFlowAsset* FlowAsset, const FGuid& NodeGuidToRemove);

	// Removes the single connection entry keyed by SourcePinName from SourceNode's Connections map
	// (reflection). Returns false only if SourceNode or the reflection property are invalid.
	static bool RemoveConnectionFromSourceNode(UFlowNode* SourceNode, FName SourcePinName);

	// Reconciles OwnerNode's AddOns array against ParsedAddOns by AddOnGuid: a matched GUID updates
	// the addon in place; an unmatched GUID creates it; a delete-marker entry removes the addon if
	// present. By default (bFullBlock = false) an addon with no entry in ParsedAddOns is left alone.
	// When bFullBlock is true the owning Node: block was marked "!full": any existing addon not
	// matched by GUID is deleted. Reports each touched addon as an FFlowReconcileAddonEntry into
	// OutAdded/OutUpdated/OutDeleted, tagged with OwnerNodeGuid (the node GUID at the top level, or
	// the parent addon GUID when recursing into nested addon-of-addon children).
	static bool ReconcileNodeAddOns(UFlowNodeBase* OwnerNode, const FGuid& OwnerNodeGuid, const TArray<FFlowGraphParsedNodeAddOn>& ParsedAddOns,
		TArray<FFlowReconcileAddonEntry>& OutAdded, TArray<FFlowReconcileAddonEntry>& OutUpdated, TArray<FFlowReconcileAddonEntry>& OutDeleted, FString& OutErrorMessage, bool bFullBlock = false);

	// Parses RawValue into a scratch instance of Property's type via
	// UFlowGraphImporter::SetPropertyFromString, then reads it back out via
	// UFlowGraphExporter::GetPropertyValueAsString. Used by FindOrCreateNode so its match
	// comparison sees the same canonical string form a value actually stored on a node would have,
	// rather than only matching when a caller happens to pass the exact serialized form. Returns
	// false (OutErrorMessage set) if RawValue doesn't parse for Property.
	static bool NormalizePropertyValueString(FProperty* Property, const FString& RawValue, FString& OutNormalizedValue, FString& OutErrorMessage);

	// Classifies ParsedNodes (relative to CurrentNodes + ScopedNodeGuids) into NodesToAdd,
	// NodesToUpdate, NodesToDelete. Also populates AliasMap for new:<alias> nodes.
	// Scope-driven implicit deletes (nodes in scope but absent from document) are added to
	// NodesToDelete after the main loop.
	static void ClassifyParsedNodes(
		const TArray<FFlowGraphParsedNode>& ParsedNodes,
		const TArray<FGuid>& ScopedNodeGuids,
		const TMap<FGuid, UFlowNode*>& CurrentNodes,
		TArray<FFlowGraphParsedNode>& OutNodesToAdd,
		TArray<FFlowGraphParsedNode>& OutNodesToUpdate,
		TArray<FGuid>& OutNodesToDelete,
		TMap<FString, FGuid>& OutAliasMap);

	// Classifies ParsedConnections (relative to CurrentNodes) into ConnectionsToAdd and
	// ConnectionsToRemove, populating Plan.ConnectionsAdded/ConnectionsRemoved with rendered labels.
	// Idempotent: already-present adds and absent removes are no-ops.
	static void ClassifyParsedConnections(
		const TArray<FFlowGraphParsedConnection>& ParsedConnections,
		const TMap<FGuid, UFlowNode*>& CurrentNodes,
		TArray<FFlowGraphParsedConnection>& OutConnectionsToAdd,
		TArray<FFlowGraphParsedConnection>& OutConnectionsToRemove,
		FFlowReconcilePlan& OutPlan);

	// Applies the classified plan (deletes, adds, updates, connection rewiring, regraph) to
	// ExistingAsset inside a single ScopedTransaction. Returns true on success.
	static bool ApplyPlanToExistingAsset(
		UFlowAsset* ExistingAsset,
		const FFlowReconcileExecutionPlan& ExecutionPlan,
		FFlowReconcileResult& Result,
		FString& OutErrorMessage);

	// Populates OutPlan from an already-converted document for a new-asset create (no existing
	// asset). ParsedNodes/ParsedConnections come from FFlowCourierConverter::ConvertToParsedGraph.
	static bool ComputePlanForNewAsset(
		const FString& AssetClassPath,
		bool bWorldBound,
		const TArray<FFlowGraphParsedNode>& ParsedNodes,
		const TArray<FFlowGraphParsedConnection>& ParsedConnections,
		FFlowReconcileExecutionPlan& OutPlan,
		FString& OutErrorMessage);

	// Full import + regraph for a new-asset create.
	// Calls UFlowGraphRegrapher::RegraphFlowAsset(asset, positions) (private, accessible via friend).
	static bool ExecutePlanForNewAsset(
		const FFlowReconcileExecutionPlan& ExecutionPlan,
		const FString& TargetAssetPath,
		FFlowReconcileResult& Result,
		FString& OutErrorMessage);
};
