// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphExporter.h"
#include "FlowGraphImporter.h"
#include "FlowGraphRegrapher.h"
#include "FlowCourierConverter.h"
#include "FlowCourierDocument.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "AddOns/FlowNodeAddOn.h"
#include "AddOns/FlowNodeAddOn_SwitchCase.h"
#include "Graph/FlowGraph.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "EdGraph/EdGraphNode.h"
#include "Misc/AutomationTest.h"

// Tests for the Flow Courier format: the FlowCourier header, AssetClass line, node Pos
// round-trip, stable AddOn GUID identity, multi-level addon-of-addon recursion, and deterministic
// ordering. Property-grammar edge cases (embedded quotes/newlines) are also covered.

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FFlowCourierFormatSpec, "FlowGraphCourier.EditorGame.Format", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext)
	UFlowAsset* TestFlowAsset;
	FString TestImportText;

	static UFlowAsset* ImportFixtureDocument(const FString& TargetAssetPath, const FString& Json, FString& OutErrorMessage)
	{
		FFlowCourierDocument Document;
		TArray<FFlowCourierIssue> Issues;
		if (!FFlowCourierConverter::ParseDocument(Json, Document, Issues, OutErrorMessage))
		{
			return nullptr;
		}
		TArray<FFlowGraphParsedNode> ParsedNodes;
		TArray<FFlowGraphParsedConnection> ParsedConnections;
		TArray<FGuid> ScopedNodeGuidsUnused;
		TMap<FString, FGuid> AliasMapUnused;
		FFlowCourierConverter::ConvertToParsedGraph(Document, ParsedNodes, ParsedConnections, ScopedNodeGuidsUnused, AliasMapUnused, Issues);
		return UFlowGraphImporter::ImportFlowGraphFromDocument(TargetAssetPath, Document.AssetClass, Document.bWorldBound, ParsedNodes, ParsedConnections, OutErrorMessage);
	}

	static UFlowAsset* ImportAndRegraphFixtureDocument(const FString& Json, const FString& TargetAssetPath)
	{
		FFlowCourierDocument Document;
		TArray<FFlowCourierIssue> Issues;
		FString ErrorMessage;
		if (!FFlowCourierConverter::ParseDocument(Json, Document, Issues, ErrorMessage))
		{
			return nullptr;
		}
		TArray<FFlowGraphParsedNode> ParsedNodes;
		TArray<FFlowGraphParsedConnection> ParsedConnections;
		TArray<FGuid> ScopedNodeGuidsUnused;
		TMap<FString, FGuid> AliasMapUnused;
		FFlowCourierConverter::ConvertToParsedGraph(Document, ParsedNodes, ParsedConnections, ScopedNodeGuidsUnused, AliasMapUnused, Issues);
		return UFlowGraphRegrapher::ImportAndRegraphFromDocument(TargetAssetPath, Document.AssetClass, Document.bWorldBound, ParsedNodes, ParsedConnections);
	}
END_DEFINE_SPEC(FFlowCourierFormatSpec)

void FFlowCourierFormatSpec::Define()
{
	Describe("FlowCourierFormat", [this]()
	{
		BeforeEach([this]()
		{
			TestFlowAsset = nullptr;
			TestImportText.Empty();
		});

		AfterEach([this]()
		{
			if (TestFlowAsset)
			{
				TestFlowAsset->ClearFlags(RF_Standalone);
				TestFlowAsset->MarkAsGarbage();
				TestFlowAsset = nullptr;
			}
		});

		Describe("Header", [this]()
		{
			It("should emit FlowCourier and an AssetClass line on export", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":true,\"ops\":[]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestV2HeaderExport"), TestImportText, ErrorMessage);
				TestNotNull("Import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

				TestTrue("Export should declare Courier v2's formatVersion", ExportedText.Contains(TEXT("\"formatVersion\": 2")));
				TestTrue("Export should contain an assetClass field matching the asset's concrete class",
					ExportedText.Contains(FString::Printf(TEXT("\"assetClass\": \"%s\""), *UFlowAsset::StaticClass()->GetPathName())));
			});

			It("should resolve AssetClass on import and instantiate the matching class", [this]()
			{
				TestImportText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"%s\",\"bWorldBound\":true,\"ops\":[]}"
				), *UFlowAsset::StaticClass()->GetPathName());

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestV2AssetClassResolve"), TestImportText, ErrorMessage);
				TestNotNull("Import should succeed", TestFlowAsset);
				if (TestFlowAsset)
				{
					TestEqual("Imported asset class should match the AssetClass header", TestFlowAsset->GetClass(), UFlowAsset::StaticClass());
				}
			});

			It("should default to UFlowAsset when importing a document with no AssetClass header", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":true,\"ops\":[]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestV1NoAssetClass"), TestImportText, ErrorMessage);
				TestNotNull("Import should succeed", TestFlowAsset);
				if (TestFlowAsset)
				{
					TestEqual("Should default to the base UFlowAsset class", TestFlowAsset->GetClass(), UFlowAsset::StaticClass());
				}
			});
		});

		Describe("Node Position", [this]()
		{
			It("should restore node Pos through ImportAndRegraphFromDocument", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"bHasPosition\":true,\"position\":{\"x\":320,\"y\":-64},"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}]}"
				);

				TestFlowAsset = ImportAndRegraphFixtureDocument(TestImportText, TEXT("/Game/Test/TestPosRestoreOnRegraph"));
				TestNotNull("Import+regraph should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Node should exist", Node);
				if (!Node) { return; }

				UEdGraphNode* GraphNode = Node->GetGraphNode();
				TestNotNull("Node should have a graph node after regraphing", GraphNode);
				if (GraphNode)
				{
					TestEqual("NodePosX should match the Pos line", GraphNode->NodePosX, 320);
					TestEqual("NodePosY should match the Pos line", GraphNode->NodePosY, -64);
				}
			});

			It("should re-emit the same Pos it was given, after a regraph", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"bHasPosition\":true,\"position\":{\"x\":150,\"y\":75},"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}]}"
				);

				TestFlowAsset = ImportAndRegraphFixtureDocument(TestImportText, TEXT("/Game/Test/TestPosReExport"));
				TestNotNull("Import+regraph should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);
				TestTrue("Re-export should contain the same position", ExportedText.Contains(TEXT("\"x\": 150")) && ExportedText.Contains(TEXT("\"y\": 75")));
			});

			It("should not move an existing editor node's position on a second regraph", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"bHasPosition\":true,\"position\":{\"x\":100,\"y\":50},"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}]}"
				);

				TestFlowAsset = ImportAndRegraphFixtureDocument(TestImportText, TEXT("/Game/Test/TestPosPreserveOnReregraph"));
				TestNotNull("First import+regraph should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				if (!Node) { AddError(TEXT("Node missing after first regraph")); return; }

				UEdGraphNode* GraphNode = Node->GetGraphNode();
				if (!GraphNode) { AddError(TEXT("GraphNode missing after first regraph")); return; }

				// Simulate a designer manually moving the node in the editor before a second patch.
				GraphNode->NodePosX = 999;
				GraphNode->NodePosY = 999;

				const bool bSecondRegraphSuccess = UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset);
				TestTrue("Second regraph should succeed", bSecondRegraphSuccess);

				UFlowNode* NodeAfter = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Node should still exist", NodeAfter);
				if (!NodeAfter) { return; }

				UEdGraphNode* GraphNodeAfter = NodeAfter->GetGraphNode();
				TestNotNull("GraphNode should still exist", GraphNodeAfter);
				if (GraphNodeAfter)
				{
					TestEqual("Existing editor node identity should be unchanged", GraphNodeAfter, GraphNode);
					TestEqual("Existing node X position should be untouched by regraph", GraphNodeAfter->NodePosX, 999);
					TestEqual("Existing node Y position should be untouched by regraph", GraphNodeAfter->NodePosY, 999);
				}
			});
		});

		Describe("AddOn Identity", [this]()
		{
			It("should parse an explicit AddOn GUID from Flow Courier text", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"11111111-1111-1111-1111-111111111111\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"Alpha\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestAddOnExplicitGuid"), TestImportText, ErrorMessage);
				TestNotNull("Import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Node should exist", Node);
				if (!Node) { return; }

				const TArray<UFlowNodeAddOn*>& AddOns = Node->GetFlowNodeAddOnChildren();
				TestEqual("Should have exactly one AddOn", AddOns.Num(), 1);
				if (AddOns.Num() == 1 && AddOns[0])
				{
					FGuid ExpectedGuid;
					FGuid::Parse(TEXT("11111111-1111-1111-1111-111111111111"), ExpectedGuid);
					TestEqual("AddOn GUID should match the explicit GUID token", AddOns[0]->GetGuid(), ExpectedGuid);
				}
			});

			It("should mint a fresh valid GUID for an AddOn line with no GUID token", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"newAlias\":\"case\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"Beta\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestAddOnMintedGuid"), TestImportText, ErrorMessage);
				TestNotNull("Import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				if (!Node) { return; }

				const TArray<UFlowNodeAddOn*>& AddOns = Node->GetFlowNodeAddOnChildren();
				TestEqual("Should have exactly one AddOn", AddOns.Num(), 1);
				if (AddOns.Num() == 1 && AddOns[0])
				{
					TestTrue("A fresh, valid GUID should have been minted", AddOns[0]->GetGuid().IsValid());
				}
			});

			It("should re-export the AddOn's GUID", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"22222222-2222-2222-2222-222222222222\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"Gamma\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestAddOnGuidReExport"), TestImportText, ErrorMessage);
				TestNotNull("Import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

				FGuid ExpectedAddOnGuid;
				FGuid::Parse(TEXT("22222222-2222-2222-2222-222222222222"), ExpectedAddOnGuid);
				TestTrue("Re-export should contain the same AddOn GUID", ExportedText.Contains(FString::Printf(TEXT("\"guid\": \"%s\""), *ExpectedAddOnGuid.ToString())));
				TestTrue("Re-export should contain the AddOn's class", ExportedText.Contains(TEXT("\"type\": \"/Script/Flow.FlowNodeAddOn_SwitchCase\"")));
			});

			It("should round-trip a two-level-deep addon tree with stable GUIDs", [this]()
			{
				// AddOn 11...11 is a direct child of the node; AddOn 22...22 is nested under it
				// (addon-of-addon) via 4-space indentation on its "AddOn:" line.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"11111111-1111-1111-1111-111111111111\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"Outer\"}},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"22222222-2222-2222-2222-222222222222\",\"parentGuid\":\"11111111-1111-1111-1111-111111111111\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"Inner\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestAddOnTreeRoundTrip"), TestImportText, ErrorMessage);
				TestNotNull("Import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuid, OuterAddOnGuid, InnerAddOnGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				FGuid::Parse(TEXT("11111111-1111-1111-1111-111111111111"), OuterAddOnGuid);
				FGuid::Parse(TEXT("22222222-2222-2222-2222-222222222222"), InnerAddOnGuid);

				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Node should exist", Node);
				if (!Node) { return; }

				const TArray<UFlowNodeAddOn*>& OuterAddOns = Node->GetFlowNodeAddOnChildren();
				TestEqual("Node should have exactly 1 direct addon child", OuterAddOns.Num(), 1);
				if (OuterAddOns.Num() != 1 || !OuterAddOns[0]) { return; }
				TestEqual("Outer addon should have the expected GUID", OuterAddOns[0]->GetGuid(), OuterAddOnGuid);

				const TArray<UFlowNodeAddOn*>& InnerAddOns = OuterAddOns[0]->GetFlowNodeAddOnChildren();
				TestEqual("Outer addon should have exactly 1 nested child", InnerAddOns.Num(), 1);
				if (InnerAddOns.Num() != 1 || !InnerAddOns[0]) { return; }
				TestEqual("Inner addon should have the expected GUID", InnerAddOns[0]->GetGuid(), InnerAddOnGuid);

				const FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);
				TestTrue("Re-export should contain the outer addon's guid", ExportedText.Contains(FString::Printf(TEXT("\"guid\": \"%s\""), *OuterAddOnGuid.ToString())));
				TestTrue("Re-export should contain the inner addon's guid", ExportedText.Contains(FString::Printf(TEXT("\"guid\": \"%s\""), *InnerAddOnGuid.ToString())));
				TestTrue("Re-export should parent the inner addon to the outer addon",
					ExportedText.Contains(FString::Printf(TEXT("\"parentGuid\": \"%s\""), *OuterAddOnGuid.ToString())));

				// export(import(export(x))) == export(x) - the round-trip contract, now proven
				// for a nested tree rather than just flat siblings.
				FFlowCourierDocument ReImportDocument;
				TArray<FFlowCourierIssue> ReImportIssues;
				FString ReImportError;
				const bool bReImportParsed = FFlowCourierConverter::ParseDocument(ExportedText, ReImportDocument, ReImportIssues, ReImportError);
				TestTrue("Exported document should parse", bReImportParsed);
				if (!bReImportParsed) { AddError(FString::Printf(TEXT("Re-import parse failed: %s"), *ReImportError)); return; }

				TArray<FFlowGraphParsedNode> ReImportedNodes;
				TArray<FFlowGraphParsedConnection> ReImportedConnections;
				TArray<FGuid> ScopedNodeGuidsUnused;
				TMap<FString, FGuid> AliasMapUnused;
				FFlowCourierConverter::ConvertToParsedGraph(ReImportDocument, ReImportedNodes, ReImportedConnections, ScopedNodeGuidsUnused, AliasMapUnused, ReImportIssues);

				UFlowAsset* ReImportedAsset = UFlowGraphImporter::ImportFlowGraphFromDocument(
					TEXT("/Game/Test/TestAddOnTreeRoundTrip2"), ReImportDocument.AssetClass, ReImportDocument.bWorldBound,
					ReImportedNodes, ReImportedConnections, ReImportError);
				TestNotNull("Re-import of the exported document should succeed", ReImportedAsset);
				if (!ReImportedAsset) { AddError(FString::Printf(TEXT("Re-import failed: %s"), *ReImportError)); return; }

				const FString ReExportedText = UFlowGraphExporter::ExportFlowGraphToString(ReImportedAsset);
				TestEqual("Re-export of the re-import should be byte-identical", ReExportedText, ExportedText);
			});

			It("should report a reachable addon parent cycle instead of recursing indefinitely", [this]()
			{
				const FString CyclicText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":true,\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\"},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\"},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"00000000-0000-0000-0000-000000000003\",\"parentGuid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\"},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"parentGuid\":\"00000000-0000-0000-0000-000000000003\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\"}]}"
				);
				FFlowCourierDocument Document;
				TArray<FFlowCourierIssue> Issues;
				FString ErrorMessage;
				if (!TestTrue("Cyclic document parses", FFlowCourierConverter::ParseDocument(CyclicText, Document, Issues, ErrorMessage)))
				{
					return;
				}

				TArray<FFlowGraphParsedNode> Nodes;
				TArray<FFlowGraphParsedConnection> Connections;
				TArray<FGuid> ScopedNodeGuids;
				TMap<FString, FGuid> Aliases;
				FFlowCourierConverter::ConvertToParsedGraph(Document, Nodes, Connections, ScopedNodeGuids, Aliases, Issues);
				TestTrue("Reachable addon cycle is a validation error", Issues.ContainsByPredicate([](const FFlowCourierIssue& Issue)
				{
					return Issue.Code == TEXT("AddonCycle") && Issue.Severity == EFlowValidationSeverity::Error;
				}));
			});

			It("should report a detached addon parent cycle rather than silently dropping its ops", [this]()
			{
				const FString CyclicText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":true,\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\"},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"parentGuid\":\"00000000-0000-0000-0000-000000000003\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\"},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"00000000-0000-0000-0000-000000000003\",\"parentGuid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\"}]}"
				);
				FFlowCourierDocument Document;
				TArray<FFlowCourierIssue> Issues;
				FString ErrorMessage;
				if (!TestTrue("Detached cycle document parses", FFlowCourierConverter::ParseDocument(CyclicText, Document, Issues, ErrorMessage)))
				{
					return;
				}

				TArray<FFlowGraphParsedNode> Nodes;
				TArray<FFlowGraphParsedConnection> Connections;
				TArray<FGuid> ScopedNodeGuids;
				TMap<FString, FGuid> Aliases;
				FFlowCourierConverter::ConvertToParsedGraph(Document, Nodes, Connections, ScopedNodeGuids, Aliases, Issues);
				TestTrue("Detached addon cycle is a validation error", Issues.ContainsByPredicate([](const FFlowCourierIssue& Issue)
				{
					return Issue.Code == TEXT("AddonCycle") && Issue.Severity == EFlowValidationSeverity::Error;
				}));
			});
		});

		Describe("Property Value Grammar", [this]()
		{
			It("round-trips a property value containing a colon and an escaped newline", [this]()
			{
				// FlowNode_Log's Message (FString) is a plain, already-proven property to hang
				// this test on. The value below deliberately contains a colon, an embedded
				// newline, and a double quote - the three hazards the exporter escapes.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Log\","
					"\"properties\":{\"Message\":\"Label: line one\\nline \\\"two\\\"\"}}]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestPropertyEscaping"), TestImportText, ErrorMessage);
				TestNotNull("Import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Node should exist", Node);
				if (!Node) { return; }

				FProperty* MessageProperty = Node->GetClass()->FindPropertyByName(TEXT("Message"));
				TestNotNull("Message property should exist", MessageProperty);
				if (!MessageProperty) { return; }

				FStrProperty* StrProperty = CastField<FStrProperty>(MessageProperty);
				TestNotNull("Message should be a string property", StrProperty);
				if (!StrProperty) { return; }

				const FString ActualValue = StrProperty->GetPropertyValue_InContainer(Node);
				const FString ExpectedValue = TEXT("Label: line one\nline \"two\"");
				TestEqual("Value should round-trip exactly, including the colon, newline, and quote", ActualValue, ExpectedValue);

				// export(import(x)) then re-import should reproduce the exact same live value -
				// proves the value survives Courier v2's JSON string escaping, not just UE's own
				// property export-text escaping.
				const FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

				FFlowCourierDocument ReImportDocument;
				TArray<FFlowCourierIssue> ReImportIssues;
				FString ReImportError;
				const bool bReImportParsed = FFlowCourierConverter::ParseDocument(ExportedText, ReImportDocument, ReImportIssues, ReImportError);
				TestTrue("Exported document should parse", bReImportParsed);
				if (!bReImportParsed) { AddError(FString::Printf(TEXT("Re-import parse failed: %s"), *ReImportError)); return; }

				TArray<FFlowGraphParsedNode> ReImportedNodes;
				TArray<FFlowGraphParsedConnection> ReImportedConnections;
				TArray<FGuid> ScopedNodeGuidsUnused;
				TMap<FString, FGuid> AliasMapUnused;
				FFlowCourierConverter::ConvertToParsedGraph(ReImportDocument, ReImportedNodes, ReImportedConnections, ScopedNodeGuidsUnused, AliasMapUnused, ReImportIssues);

				UFlowAsset* ReImportedAsset = UFlowGraphImporter::ImportFlowGraphFromDocument(
					TEXT("/Game/Test/TestPropertyEscaping2"), ReImportDocument.AssetClass, ReImportDocument.bWorldBound,
					ReImportedNodes, ReImportedConnections, ReImportError);
				TestNotNull("Re-import of the exported document should succeed", ReImportedAsset);
				if (!ReImportedAsset) { AddError(FString::Printf(TEXT("Re-import failed: %s"), *ReImportError)); return; }

				UFlowNode* ReImportedNode = ReImportedAsset->GetNode(NodeGuid);
				TestNotNull("Re-imported node should exist", ReImportedNode);
				if (ReImportedNode)
				{
					FStrProperty* ReImportedStrProperty = CastField<FStrProperty>(ReImportedNode->GetClass()->FindPropertyByName(TEXT("Message")));
					if (TestNotNull("Re-imported Message property should exist", ReImportedStrProperty))
					{
						TestEqual("Value should survive a full export/re-import round-trip",
							ReImportedStrProperty->GetPropertyValue_InContainer(ReImportedNode), ExpectedValue);
					}
				}
			});

			It("produces a structured error naming the GUID and property for a malformed value, aborting the parse", [this]()
			{
				// FlowNode_DefineProperties' NamedProperties (TArray<FFlowNamedDataPinProperty>) is
				// already proven to import correctly with well-formed text elsewhere in this test
				// suite - deliberately garble it here (missing closing parens) to force
				// FArrayProperty::ImportText_Direct to fail.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_DefineProperties\","
					"\"properties\":{\"NamedProperties\":\"((Name=\\\"TestProp\\\",DataPinValue=/Script/Flow.FlowDataPinValue_Text(Values=(\\\"unterminated\"},"
					"\"outputPins\":[{\"name\":\"TestProp\",\"type\":\"Text\"}]}]}"
				);

				// Importing deliberately triggers a LogFlow Error as it aborts the parse - tell the
				// automation framework to expect it, otherwise the unexpected-error auto-fail would
				// mask the actual TestNull/TestTrue assertions below.
				AddExpectedErrorPlain(TEXT("Failed to set property 'NamedProperties'"), EAutomationExpectedErrorFlags::Contains, 1);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestMalformedValue"), TestImportText, ErrorMessage);
				TestNull("Import should fail rather than silently continue with a partially-set node", TestFlowAsset);

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), NodeGuid);
				TestTrue("Error should name the GUID that failed", ErrorMessage.Contains(NodeGuid.ToString()));
				TestTrue("Error should name the property that failed", ErrorMessage.Contains(TEXT("NamedProperties")));
			});
		});

		Describe("Deterministic Ordering", [this]()
		{
			It("should export nodes sorted by GUID string regardless of declaration order", [this]()
			{
				// Declared out of GUID order (3, 1, 2) to prove the exporter sorts rather than
				// preserving TMap/declaration order.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000003\",\"type\":\"/Script/Flow.FlowNode_Reroute\"},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Reroute\"},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_Reroute\"}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestNodeOrdering"), TestImportText, ErrorMessage);
				TestNotNull("Import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

				FGuid Guid1, Guid2, Guid3;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), Guid1);
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), Guid2);
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000003"), Guid3);

				const int32 Pos1 = ExportedText.Find(FString::Printf(TEXT("\"guid\": \"%s\""), *Guid1.ToString()));
				const int32 Pos2 = ExportedText.Find(FString::Printf(TEXT("\"guid\": \"%s\""), *Guid2.ToString()));
				const int32 Pos3 = ExportedText.Find(FString::Printf(TEXT("\"guid\": \"%s\""), *Guid3.ToString()));

				TestTrue("Node 1 should be present", Pos1 != INDEX_NONE);
				TestTrue("Node 2 should be present", Pos2 != INDEX_NONE);
				TestTrue("Node 3 should be present", Pos3 != INDEX_NONE);
				TestTrue("Nodes should appear in ascending GUID-string order, not declaration order",
					Pos1 != INDEX_NONE && Pos2 != INDEX_NONE && Pos3 != INDEX_NONE && Pos1 < Pos2 && Pos2 < Pos3);
			});

			It("should produce byte-identical exports across repeated calls with no changes", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_Finish\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\"Out\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"In\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(TEXT("/Game/Test/TestExportDeterminism"), TestImportText, ErrorMessage);
				TestNotNull("Import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const FString Export1 = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);
				const FString Export2 = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

				TestEqual("Repeated exports with no changes should be byte-identical", Export1, Export2);
			});
		});
	});
}

#endif
