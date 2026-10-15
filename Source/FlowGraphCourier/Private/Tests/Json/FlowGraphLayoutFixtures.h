// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

// Courier v2 JSON mutation documents used by FlowGraphLayout.spec.cpp. Fixture-building
// ("CreateText" in the spec file) still uses UFlowGraphRegrapher::ImportAndRegraphFromText (v1
// text, still valid) - only the mutation documents fed through the reconciler need Courier v2.

// Adds a new Finish node via newAlias "nodeB", connected from A.Out - for the auto-placement test.
inline const TCHAR* GLayoutJson_AddNodeBFromA = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "newAlias": "nodeB", "type": "/Script/Flow.FlowNode_Finish", "inputPins": [ { "name": "In", "type": "Exec" } ] },
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000A1", "pin": "Out" }, "target": { "nodeAlias": "nodeB", "pin": "In" } }
	]
}
)JSON");

// Inserts a new Timer node via newAlias "nodeB" chained off A - for the preserve-existing-position test.
inline const TCHAR* GLayoutJson_InsertTimerNodeBFromA = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertNode",
			"newAlias": "nodeB",
			"type": "/Script/Flow.FlowNode_Timer",
			"properties": { "CompletionTime": "1.0", "StepTime": "1.0" },
			"inputPins": [ { "name": "In", "type": "Exec" } ],
			"outputPins": [ { "name": "Completed", "type": "Exec" }, { "name": "Step", "type": "Exec" } ]
		},
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000A2", "pin": "Out" }, "target": { "nodeAlias": "nodeB", "pin": "In" } }
	]
}
)JSON");

// Adds an isolated new Finish node via newAlias "isolated" - no connections at all - for the
// overlap-nudge test.
inline const TCHAR* GLayoutJson_AddIsolatedNode = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "newAlias": "isolated", "type": "/Script/Flow.FlowNode_Finish", "inputPins": [ { "name": "In", "type": "Exec" } ] }
	]
}
)JSON");

// Adds a Timer node through alias "timer" with an explicit position. The position must
// resolve to the GUID minted for that alias.
inline const TCHAR* GLayoutJson_AddTimerWithExplicitPos = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertNode",
			"newAlias": "timer",
			"type": "/Script/Flow.FlowNode_Timer",
			"properties": { "CompletionTime": "5.0", "StepTime": "1.0" },
			"inputPins": [ { "name": "In", "type": "Exec" } ],
			"outputPins": [ { "name": "Completed", "type": "Exec" }, { "name": "Step", "type": "Exec" } ],
			"bHasPosition": true,
			"position": { "x": 700, "y": 200 }
		}
	]
}
)JSON");
