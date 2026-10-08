// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

// Courier v2 JSON mutation documents used by FlowGraphValidation.spec.cpp. Each targets the
// two-node (A=Start, B=Finish) fixture built via UFlowGraphImporter::ImportFlowGraphFromText in
// the spec file (MakeFixtureText) - only the mutation documents fed to the reconciler need to be
// Courier v2, since the fixture-building importer path remains valid v1 text.

// Adds Finish node C, then fans A.Out out to both B.In and C.In - illegal (one target per pin).
inline const TCHAR* GValidationJson_FanOut = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000C0", "type": "/Script/Flow.FlowNode_Finish", "inputPins": [ { "name": "In", "type": "Exec" } ] },
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000A0", "pin": "Out" }, "target": { "nodeGuid": "00000000-0000-0000-0000-0000000000B0", "pin": "In" } },
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000A0", "pin": "Out" }, "target": { "nodeGuid": "00000000-0000-0000-0000-0000000000C0", "pin": "In" } }
	]
}
)JSON");

// Two UpsertNode ops targeting the same guid D0 - a duplicate node identity in one document.
inline const TCHAR* GValidationJson_DuplicateNodeGuid = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000D0", "type": "/Script/Flow.FlowNode_Finish", "inputPins": [ { "name": "In", "type": "Exec" } ] },
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000D0", "type": "/Script/Flow.FlowNode_Finish", "inputPins": [ { "name": "In", "type": "Exec" } ] }
	]
}
)JSON");

// References a node class path that does not exist.
inline const TCHAR* GValidationJson_UnresolvableClass = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000E0", "type": "/Script/Flow.FlowNode_ThisClassDoesNotExist" }
	]
}
)JSON");

// Connects A.Out to a node guid (AA) never declared as a node anywhere in this document or on the asset.
inline const TCHAR* GValidationJson_DanglingNode = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000A0", "pin": "Out" }, "target": { "nodeGuid": "00000000-0000-0000-0000-0000000000AA", "pin": "In" } }
	]
}
)JSON");

// Node B (Finish) is real, but has no "NoSuchPin" input pin.
inline const TCHAR* GValidationJson_DanglingPin = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000A0", "pin": "Out" }, "target": { "nodeGuid": "00000000-0000-0000-0000-0000000000B0", "pin": "NoSuchPin" } }
	]
}
)JSON");

// Adds a Switch node (F0) with a SwitchCase addon (F1) - a legal attachment.
inline const TCHAR* GValidationJson_LegalAddonAttachment = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000F0", "type": "/Script/Flow.FlowNode_Switch", "inputPins": [ { "name": "In", "type": "Exec" } ] },
		{ "kind": "UpsertAddon", "guid": "00000000-0000-0000-0000-0000000000F1", "parentGuid": "00000000-0000-0000-0000-0000000000F0", "type": "/Script/Flow.FlowNodeAddOn_SwitchCase", "properties": { "CaseName": "Alpha" } }
	]
}
)JSON");

// A SwitchCase addon (Inner) nested under another SwitchCase (Outer) on a new Start node - the
// outer SwitchCase only accepts IFlowPredicateInterface children, so this is an illegal attachment.
inline const TCHAR* GValidationJson_IllegalAddonAttachment = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000A5", "type": "/Script/Flow.FlowNode_Start", "outputPins": [ { "name": "Out", "type": "Exec" } ] },
		{ "kind": "UpsertAddon", "guid": "00000000-0000-0000-0000-0000000000A6", "parentGuid": "00000000-0000-0000-0000-0000000000A5", "type": "/Script/Flow.FlowNodeAddOn_SwitchCase", "properties": { "CaseName": "Outer" } },
		{ "kind": "UpsertAddon", "guid": "00000000-0000-0000-0000-0000000000A7", "parentGuid": "00000000-0000-0000-0000-0000000000A6", "type": "/Script/Flow.FlowNodeAddOn_SwitchCase", "properties": { "CaseName": "Inner" } }
	]
}
)JSON");

// A benign no-op-shaped update to node B (present but empty properties) - for the per-asset-class
// validation hook test, which injects its own fake Error finding regardless of document content.
inline const TCHAR* GValidationJson_TouchNodeB = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000B0" }
	]
}
)JSON");

// New FormatText node C1; wires its Text output into B's exec input pin (data -> exec, illegal).
inline const TCHAR* GValidationJson_ExecDataCross = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertNode",
			"guid": "00000000-0000-0000-0000-0000000000C1",
			"type": "/Script/Flow.FlowNode_FormatText",
			"inputPins": [ { "name": "FormatText", "type": "Text" } ],
			"outputPins": [ { "name": "Formatted Text", "type": "Text" } ]
		},
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000C1", "pin": "Formatted Text" }, "target": { "nodeGuid": "00000000-0000-0000-0000-0000000000B0", "pin": "In" } }
	]
}
)JSON");

// Two FormatText nodes (C2, D2); Text output -> Text input is a compatible data connection.
inline const TCHAR* GValidationJson_CompatibleDataConnection = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertNode",
			"guid": "00000000-0000-0000-0000-0000000000C2",
			"type": "/Script/Flow.FlowNode_FormatText",
			"inputPins": [ { "name": "FormatText", "type": "Text" } ],
			"outputPins": [ { "name": "Formatted Text", "type": "Text" } ]
		},
		{
			"kind": "UpsertNode",
			"guid": "00000000-0000-0000-0000-0000000000D2",
			"type": "/Script/Flow.FlowNode_FormatText",
			"inputPins": [ { "name": "FormatText", "type": "Text" } ],
			"outputPins": [ { "name": "Formatted Text", "type": "Text" } ]
		},
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000C2", "pin": "Formatted Text" }, "target": { "nodeGuid": "00000000-0000-0000-0000-0000000000D2", "pin": "FormatText" } }
	]
}
)JSON");

// One Text output (C4) drives two Text inputs (D4, E4). Data outputs may fan out;
// only exec outputs are limited to one target.
inline const TCHAR* GValidationJson_DataFanOutIsLegal = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{
			"kind": "UpsertNode",
			"guid": "00000000-0000-0000-0000-0000000000C4",
			"type": "/Script/Flow.FlowNode_FormatText",
			"inputPins": [ { "name": "FormatText", "type": "Text" } ],
			"outputPins": [ { "name": "Formatted Text", "type": "Text" } ]
		},
		{
			"kind": "UpsertNode",
			"guid": "00000000-0000-0000-0000-0000000000D4",
			"type": "/Script/Flow.FlowNode_FormatText",
			"inputPins": [ { "name": "FormatText", "type": "Text" } ]
		},
		{
			"kind": "UpsertNode",
			"guid": "00000000-0000-0000-0000-0000000000E4",
			"type": "/Script/Flow.FlowNode_FormatText",
			"inputPins": [ { "name": "FormatText", "type": "Text" } ]
		},
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000C4", "pin": "Formatted Text" }, "target": { "nodeGuid": "00000000-0000-0000-0000-0000000000D4", "pin": "FormatText" } },
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000C4", "pin": "Formatted Text" }, "target": { "nodeGuid": "00000000-0000-0000-0000-0000000000E4", "pin": "FormatText" } }
	]
}
)JSON");

// Native Vector output (C3) -> Bool input (D3): both pins exist, but no standard
// policy allows Vector -> Bool, so validation rejects the connection.
inline const TCHAR* GValidationJson_IncompatibleDataConnection = TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000C3", "type": "/Script/FlowGraphCourier.FlowGraphValidationTestVectorNode", "outputPins": [ { "name": "MyVec", "type": "Vector" } ] },
		{ "kind": "UpsertNode", "guid": "00000000-0000-0000-0000-0000000000D3", "type": "/Script/FlowGraphCourier.FlowGraphValidationTestBoolNode", "inputPins": [ { "name": "MyBool", "type": "Bool" } ] },
		{ "kind": "AddConnection", "source": { "nodeGuid": "00000000-0000-0000-0000-0000000000C3", "pin": "MyVec" }, "target": { "nodeGuid": "00000000-0000-0000-0000-0000000000D3", "pin": "MyBool" } }
	]
}
)JSON");
