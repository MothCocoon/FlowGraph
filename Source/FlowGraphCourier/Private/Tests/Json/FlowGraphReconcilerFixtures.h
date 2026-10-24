// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

// Courier v2 JSON mutation documents used by FlowGraphReconciler.spec.cpp. Kept as raw string
// constants in their own file so the JSON itself stays plain and readable, uncluttered by C++.
// GUIDs here match the fixed fixture GUIDs the spec builds via UFlowGraphImporter::ImportFlowGraphFromText
// (still valid - only the mutation documents fed to the reconciler need to be Courier v2).

// Updates node B's CompletionTime in place. Nothing else in the three-node fixture is mentioned.
inline const TCHAR* GReconcilerJson_UpdateNodeBProperty = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-00000000000B", "properties": { "CompletionTime": "9.0" } }
	]
}
)JSON");

// Re-declares node A's output pins, adding a new "Score" Float data pin alongside the existing
// exec pin - proves ApplyDeclaredPins runs on the reconciler's update path, not just node creation.
inline const TCHAR* GReconcilerJson_AddScorePinOnNodeA = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertNode",
			"guid": "00000000-0000-0000-0000-00000000000A",
			"outputPins": [
				{ "name": "Out", "type": "Exec" },
				{ "name": "Score", "type": "Float" }
			]
		}
	]
}
)JSON");

// Adds a new Timer node via newAlias "extraTimer" - no connections reference it.
inline const TCHAR* GReconcilerJson_AddNodeViaAlias = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertNode",
			"newAlias": "extraTimer",
			"type": "/Script/Flow.FlowNode_Timer",
			"properties": { "CompletionTime": "3.0", "StepTime": "1.0" },
			"inputPins": [ { "name": "In", "type": "Exec" } ],
			"outputPins": [ { "name": "Completed", "type": "Exec" }, { "name": "Step", "type": "Exec" } ]
		}
	]
}
)JSON");

// Adds a new Finish node via newAlias "extraFinish" and retargets B's existing Completed
// connection from C onto the new aliased node. Aliases resolve before connection ops apply.
inline const TCHAR* GReconcilerJson_AddNodeViaAliasAndConnect = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "newAlias": "extraFinish", "type": "/Script/Flow.FlowNode_Finish", "inputPins": [ { "name": "In", "type": "Exec" } ] },
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-00000000000B", "pin": "Completed" }, "target": { "nodeAlias": "extraFinish", "pin": "In" } }
	]
}
)JSON");

// References an alias that no op in the document defines - must fail with UnresolvedAlias.
inline const TCHAR* GReconcilerJson_ConnectionToUndefinedAlias = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-00000000000B", "pin": "Completed" }, "target": { "nodeAlias": "neverDefined", "pin": "In" } }
	]
}
)JSON");

// Deletes node B by GUID. Connections referencing B (A->B, B->C) must be dropped with it.
inline const TCHAR* GReconcilerJson_DeleteNodeB = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "DeleteNode", "guid": "00000000-0000-0000-0000-00000000000B" }
	]
}
)JSON");

// Adds a Finish node D and connects B.Completed -> D.In. Applying this same document a second
// time must be a no-op for both the node and the connection (idempotent upsert/add).
inline const TCHAR* GReconcilerJson_AddNodeAndConnectionIdempotent = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-00000000000D", "type": "/Script/Flow.FlowNode_Finish", "inputPins": [ { "name": "In", "type": "Exec" } ] },
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-00000000000B", "pin": "Completed" }, "target": { "nodeGuid": "00000000-0000-0000-0000-00000000000D", "pin": "In" } }
	]
}
)JSON");

// Mentions only node B - A and C are omitted entirely (not DeleteNode). Neither should be deleted;
// omission without scope is never destructive.
inline const TCHAR* GReconcilerJson_PartialDocumentDoesNotDelete = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-00000000000B", "properties": { "CompletionTime": "7.0" } }
	]
}
)JSON");

// scopeNodeGuids names only C. Document mentions neither B nor C via any op: B is unscoped so
// must survive; C is in scope, so its absence alone means "delete it".
inline const TCHAR* GReconcilerJson_ScopedDeleteOfNodeC = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"scopeNodeGuids": [ "00000000-0000-0000-0000-00000000000C" ],
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-00000000000B", "properties": { "CompletionTime": "7.0" } }
	]
}
)JSON");

// Retargets A's exec output pin from B onto a new node D, without any RemoveConnection for the
// old A->B edge - an exec output pin is one-target-only at runtime, so the add alone must replace it.
inline const TCHAR* GReconcilerJson_RetargetExecOutputPin = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-00000000000D", "type": "/Script/Flow.FlowNode_Finish", "inputPins": [ { "name": "In", "type": "Exec" } ] },
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-00000000000A", "pin": "Out" }, "target": { "nodeGuid": "00000000-0000-0000-0000-00000000000D", "pin": "In" } }
	]
}
)JSON");

// Deletes addon D from node A1 by GUID, leaving sibling addon E1 untouched (not bReplaceAddons).
inline const TCHAR* GReconcilerJson_DeleteOneAddonLeavesSibling = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "DeleteAddon", "guid": "00000000-0000-0000-0000-0000000000D1", "parentGuid": "00000000-0000-0000-0000-0000000000A1" }
	]
}
)JSON");

// Adds a brand-new addon to node A1 (no existing addons in this fixture).
inline const TCHAR* GReconcilerJson_AddAddonToNodeA1 = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertAddon",
			"newAlias": "newCase",
			"parentGuid": "00000000-0000-0000-0000-0000000000A1",
			"type": "/Script/Flow.FlowNodeAddOn_SwitchCase",
			"properties": { "CaseName": "CaseF" }
		}
	]
}
)JSON");

// Updates an existing addon's CaseName property in place (matched by GUID).
inline const TCHAR* GReconcilerJson_UpdateAddonCaseName = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertAddon",
			"guid": "00000000-0000-0000-0000-0000000000F1",
			"parentGuid": "00000000-0000-0000-0000-0000000000A1",
			"properties": { "CaseName": "CaseFRenamed" }
		}
	]
}
)JSON");

// bReplaceAddons true on node A1: D1 is explicitly re-affirmed so it survives; E1 is omitted
// entirely, and under bReplaceAddons that omission alone means "delete it".
inline const TCHAR* GReconcilerJson_ReplaceAddonsTrueDeletesOmitted = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000A1", "bReplaceAddons": true },
		{ "kind": "UpsertAddon", "guid": "00000000-0000-0000-0000-0000000000D1", "parentGuid": "00000000-0000-0000-0000-0000000000A1", "properties": { "CaseName": "CaseD" } }
	]
}
)JSON");

// Re-affirms D1 without bReplaceAddons (the default, false): merge semantics mean E1 - omitted
// entirely from this document - must survive untouched alongside D1.
inline const TCHAR* GReconcilerJson_ReplaceAddonsFalsePreservesOmitted = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertAddon", "guid": "00000000-0000-0000-0000-0000000000D1", "parentGuid": "00000000-0000-0000-0000-0000000000A1", "properties": { "CaseName": "CaseD" } }
	]
}
)JSON");

// Deletes addon F1 from node A1 - the node's only addon.
inline const TCHAR* GReconcilerJson_DeleteAddonF1 = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "DeleteAddon", "guid": "00000000-0000-0000-0000-0000000000F1", "parentGuid": "00000000-0000-0000-0000-0000000000A1" }
	]
}
)JSON");

// Full-mode document creating a brand-new single-node asset - for the reconciler's create path.
inline const TCHAR* GReconcilerJson_CreateAssetOneNode = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Full",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000AA", "type": "/Script/Flow.FlowNode_Start", "outputPins": [ { "name": "Out", "type": "Exec" } ] }
	]
}
)JSON");

// A Full-mode document creating a brand-new two-node asset with one connection between them,
// both nodes identified only by newAlias.
inline const TCHAR* GReconcilerJson_NewAssetTwoNodes = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Full",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "newAlias": "start", "type": "/Script/Flow.FlowNode_Start", "outputPins": [ { "name": "Out", "type": "Exec" } ] },
		{ "kind": "UpsertNode", "newAlias": "finish", "type": "/Script/Flow.FlowNode_Finish", "inputPins": [ { "name": "In", "type": "Exec" } ] },
		{ "kind": "AddConnection", "source": { "nodeAlias": "start", "pin": "Out" }, "target": { "nodeAlias": "finish", "pin": "In" } }
	]
}
)JSON");

// Not valid JSON at all - ComputeReconcilePlan must fail hard (return nullptr), not throw or crash.
inline const TCHAR* GReconcilerJson_MalformedJson = TEXT("{ this is not valid json");

// Adds two Timer nodes with identical properties but deliberately out-of-lexical-order GUIDs -
// for FindOrCreateNode's deterministic-tiebreak test (lowest FGuid::ToString() must win).
inline const TCHAR* GReconcilerJson_AddTwoMatchingTimerNodes = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "FFFFFFFF-0000-0000-0000-000000000001", "type": "/Script/Flow.FlowNode_Timer", "properties": { "CompletionTime": "77.0", "StepTime": "2.0" } },
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-000000000002", "type": "/Script/Flow.FlowNode_Timer", "properties": { "CompletionTime": "77.0", "StepTime": "2.0" } }
	]
}
)JSON");

// Re-declares the already-wired data connection E1."Formatted Text" -> E2.FormatText - must be
// recognized as already present (idempotent), not treated as new.
inline const TCHAR* GReconcilerJson_ReaffirmDataConnection = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000E1", "pin": "Formatted Text" }, "target": { "nodeGuid": "00000000-0000-0000-0000-0000000000E2", "pin": "FormatText" } }
	]
}
)JSON");

// Dry-run fixture: updates node B's CompletionTime to 42.0. Never actually applied when the
// caller runs ComputeReconcilePlan alone (dry run stops before ExecuteReconcilePlan).
inline const TCHAR* GReconcilerJson_DryRunUpdateNodeB = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-00000000000B", "properties": { "CompletionTime": "42.0" } }
	]
}
)JSON");

// Adds a new Timer node via newAlias "extra" - for the nodes_touched/nodes_preserved add-metrics test.
inline const TCHAR* GReconcilerJson_MetricsAddNode = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertNode",
			"newAlias": "extra",
			"type": "/Script/Flow.FlowNode_Timer",
			"properties": { "CompletionTime": "1.0", "StepTime": "1.0" },
			"inputPins": [ { "name": "In", "type": "Exec" } ],
			"outputPins": [ { "name": "Completed", "type": "Exec" }, { "name": "Step", "type": "Exec" } ]
		}
	]
}
)JSON");

// Deletes node C - for the nodes_touched/nodes_preserved delete-metrics test.
inline const TCHAR* GReconcilerJson_MetricsDeleteNodeC = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "DeleteNode", "guid": "00000000-0000-0000-0000-00000000000C" }
	]
}
)JSON");

// Re-declares node B (type + pins + a new CompletionTime) for the nodes_touched/nodes_preserved
// metrics test.
inline const TCHAR* GReconcilerJson_MetricsUpdateNodeB = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertNode",
			"guid": "00000000-0000-0000-0000-00000000000B",
			"type": "/Script/Flow.FlowNode_Timer",
			"properties": { "CompletionTime": "99.0" },
			"inputPins": [ { "name": "In", "type": "Exec" } ],
			"outputPins": [ { "name": "Completed", "type": "Exec" }, { "name": "Step", "type": "Exec" } ]
		}
	]
}
)JSON");

// Full mode, re-describing only A and B and the A->B connection. Node C and the B->C connection
// are not mentioned at all - Full mode's authoritative-over-the-whole-asset contract means both
// must be implicitly deleted, unlike Patch mode where unmentioned content is left untouched.
inline const TCHAR* GReconcilerJson_FullModeOmitsNodeC = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Full",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertNode",
			"guid": "00000000-0000-0000-0000-00000000000A",
			"type": "/Script/Flow.FlowNode_Start",
			"outputPins": [ { "name": "Out", "type": "Exec" } ]
		},
		{
			"kind": "UpsertNode",
			"guid": "00000000-0000-0000-0000-00000000000B",
			"type": "/Script/Flow.FlowNode_Timer",
			"properties": { "CompletionTime": "5.0", "StepTime": "1.0" },
			"inputPins": [ { "name": "In", "type": "Exec" } ],
			"outputPins": [ { "name": "Completed", "type": "Exec" }, { "name": "Step", "type": "Exec" } ]
		},
		{
			"kind": "AddConnection",
			"source": { "nodeGuid": "00000000-0000-0000-0000-00000000000A", "pin": "Out" },
			"target": { "nodeGuid": "00000000-0000-0000-0000-00000000000B", "pin": "In" }
		}
	]
}
)JSON");
