// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FlowGraphSubgraphQuery.h"
#include "FlowGraphImporter.h"
#include "FlowCourierConverter.h"
#include "FlowCourierDocument.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

BEGIN_DEFINE_SPEC(FFlowGraphSubgraphQuerySpec, "FlowGraphCourier.EditorGame.SubgraphQuery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UFlowAsset* TestFlowAsset = nullptr;

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
END_DEFINE_SPEC(FFlowGraphSubgraphQuerySpec)

void FFlowGraphSubgraphQuerySpec::Define()
{
	Describe("FlowGraphSubgraphQuery", [this]()
	{
		BeforeEach([this]()
		{
			TestFlowAsset = nullptr;
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

		// Two disconnected exec chains (Start -> Timer -> Finish each) so reachability tests can
		// prove ExportSubgraph stops at the island boundary rather than exporting the whole asset.
		auto MakeTwoIslandsText = [](
			const FString& GuidA, const FString& GuidB, const FString& GuidC,
			const FString& GuidD, const FString& GuidE, const FString& GuidF) -> FString
		{
			return FString::Printf(TEXT(
				"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
				"\"ops\":["
				"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\","
				"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Timer\","
				"\"properties\":{\"CompletionTime\":\"1.0\"},"
				"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
				"\"outputPins\":[{\"name\":\"Completed\",\"type\":\"Exec\"},{\"name\":\"Step\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\","
				"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\","
				"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Timer\","
				"\"properties\":{\"CompletionTime\":\"2.0\"},"
				"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
				"\"outputPins\":[{\"name\":\"Completed\",\"type\":\"Exec\"},{\"name\":\"Step\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\","
				"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Out\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
				"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Completed\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
				"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Out\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
				"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Completed\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}"
				"]}"
			),
			*GuidA, *GuidB, *GuidC, *GuidD, *GuidE, *GuidF,
			*GuidA, *GuidB, *GuidB, *GuidC, *GuidD, *GuidE, *GuidE, *GuidF);
		};

		const FString GuidA = TEXT("00000000-0000-0000-0000-00000000AAAA");
		const FString GuidB = TEXT("00000000-0000-0000-0000-00000000BBBB");
		const FString GuidC = TEXT("00000000-0000-0000-0000-00000000CCCC");
		const FString GuidD = TEXT("00000000-0000-0000-0000-00000000DDDD");
		const FString GuidE = TEXT("00000000-0000-0000-0000-00000000EEEE");
		const FString GuidF = TEXT("00000000-0000-0000-0000-00000000FFFF");

		Describe("FindNodes", [this, MakeTwoIslandsText, GuidA, GuidB, GuidC, GuidD, GuidE, GuidF]()
		{
			It("returns every node when no filters are set", [this, MakeTwoIslandsText, GuidA, GuidB, GuidC, GuidD, GuidE, GuidF]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestSubgraphQuery_FindAll");
				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeTwoIslandsText(GuidA, GuidB, GuidC, GuidD, GuidE, GuidF), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				TArray<FFlowSubgraphNodeSummary> Results;
				FString ErrorMessage;
				const bool bResult = FFlowGraphSubgraphQuery::FindNodes(TestFlowAsset, TEXT(""), TEXT(""), false, Results, ErrorMessage);

				TestTrue("FindNodes should succeed", bResult);
				TestEqual("Should return all six nodes", Results.Num(), 6);
			});

			It("filters by class", [this, MakeTwoIslandsText, GuidA, GuidB, GuidC, GuidD, GuidE, GuidF]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestSubgraphQuery_FindByClass");
				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeTwoIslandsText(GuidA, GuidB, GuidC, GuidD, GuidE, GuidF), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				TArray<FFlowSubgraphNodeSummary> Results;
				FString ErrorMessage;
				const bool bResult = FFlowGraphSubgraphQuery::FindNodes(TestFlowAsset, TEXT(""), TEXT("FlowNode_Timer"), false, Results, ErrorMessage);

				TestTrue("FindNodes should succeed", bResult);
				TestEqual("Should return only the two Timer nodes", Results.Num(), 2);
				for (const FFlowSubgraphNodeSummary& Summary : Results)
				{
					TestTrue("Every result should be a FlowNode_Timer", Summary.ClassPath.Contains(TEXT("FlowNode_Timer")));
				}
			});

			It("returns a hard error for an unresolvable class filter", [this, MakeTwoIslandsText, GuidA, GuidB, GuidC, GuidD, GuidE, GuidF]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestSubgraphQuery_FindByBadClass");
				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeTwoIslandsText(GuidA, GuidB, GuidC, GuidD, GuidE, GuidF), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				TArray<FFlowSubgraphNodeSummary> Results;
				FString ErrorMessage;
				const bool bResult = FFlowGraphSubgraphQuery::FindNodes(TestFlowAsset, TEXT(""), TEXT("NotARealFlowNodeClass"), false, Results, ErrorMessage);

				TestFalse("FindNodes should fail for an unresolvable class filter", bResult);
				TestFalse("Error message should not be empty", ErrorMessage.IsEmpty());
			});

			It("bEntryPointsOnly returns only nodes with no incoming connection", [this, MakeTwoIslandsText, GuidA, GuidB, GuidC, GuidD, GuidE, GuidF]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestSubgraphQuery_EntryPoints");
				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeTwoIslandsText(GuidA, GuidB, GuidC, GuidD, GuidE, GuidF), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				TArray<FFlowSubgraphNodeSummary> Results;
				FString ErrorMessage;
				const bool bResult = FFlowGraphSubgraphQuery::FindNodes(TestFlowAsset, TEXT(""), TEXT(""), true, Results, ErrorMessage);

				TestTrue("FindNodes should succeed", bResult);
				TestEqual("Should return exactly the two Start nodes (one per island)", Results.Num(), 2);

				FGuid ExpectedA, ExpectedD;
				FGuid::Parse(GuidA, ExpectedA);
				FGuid::Parse(GuidD, ExpectedD);
				for (const FFlowSubgraphNodeSummary& Summary : Results)
				{
					TestTrue("Every result should carry bIsEntryPoint=true", Summary.bIsEntryPoint);
					TestTrue("Every result should be node A or node D",
						Summary.NodeGuid == ExpectedA || Summary.NodeGuid == ExpectedD);
				}
			});
		});

		Describe("ExportSubgraph", [this, MakeTwoIslandsText, GuidA, GuidB, GuidC, GuidD, GuidE, GuidF]()
		{
			It("exports only the connected island containing the start node", [this, MakeTwoIslandsText, GuidA, GuidB, GuidC, GuidD, GuidE, GuidF]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestSubgraphQuery_ExportIsland");
				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeTwoIslandsText(GuidA, GuidB, GuidC, GuidD, GuidE, GuidF), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid StartGuid, ExpectedA, ExpectedB, ExpectedC, ExpectedD, ExpectedE, ExpectedF;
				FGuid::Parse(GuidB, StartGuid);
				FGuid::Parse(GuidA, ExpectedA);
				FGuid::Parse(GuidB, ExpectedB);
				FGuid::Parse(GuidC, ExpectedC);
				FGuid::Parse(GuidD, ExpectedD);
				FGuid::Parse(GuidE, ExpectedE);
				FGuid::Parse(GuidF, ExpectedF);

				FString ExportedText;
				int32 NodeCount = 0;
				FString ErrorMessage;
				const bool bResult = FFlowGraphSubgraphQuery::ExportSubgraph(TestFlowAsset, StartGuid, ExportedText, NodeCount, ErrorMessage);

				TestTrue("ExportSubgraph should succeed", bResult);
				TestEqual("Should include exactly the 3-node island (A, B, C)", NodeCount, 3);
				// Node/connection lines render FGuid::ToString() in its default (Digits, no separators)
				// format, not the dashed literal used to author the fixture - compare against that form.
				TestTrue("Exported text should mention node A", ExportedText.Contains(ExpectedA.ToString()));
				TestTrue("Exported text should mention node B", ExportedText.Contains(ExpectedB.ToString()));
				TestTrue("Exported text should mention node C", ExportedText.Contains(ExpectedC.ToString()));
				TestFalse("Exported text should not mention node D from the other island", ExportedText.Contains(ExpectedD.ToString()));
				TestFalse("Exported text should not mention node E from the other island", ExportedText.Contains(ExpectedE.ToString()));
				TestFalse("Exported text should not mention node F from the other island", ExportedText.Contains(ExpectedF.ToString()));

				// The result must be independently re-importable - same Courier v2 JSON document
				// shape as a full export.
				FFlowCourierDocument ReimportDocument;
				TArray<FFlowCourierIssue> ReimportIssues;
				FString ReimportError;
				const bool bParsed = FFlowCourierConverter::ParseDocument(ExportedText, ReimportDocument, ReimportIssues, ReimportError);
				TestTrue("Exported subgraph JSON should parse as a valid Courier v2 document", bParsed);
				if (bParsed)
				{
					TArray<FFlowGraphParsedNode> ReimportedNodes;
					TArray<FFlowGraphParsedConnection> ReimportedConnections;
					TArray<FGuid> ScopedNodeGuidsUnused;
					TMap<FString, FGuid> AliasMapUnused;
					FFlowCourierConverter::ConvertToParsedGraph(ReimportDocument, ReimportedNodes, ReimportedConnections, ScopedNodeGuidsUnused, AliasMapUnused, ReimportIssues);

					UFlowAsset* ReimportedAsset = UFlowGraphImporter::ImportFlowGraphFromDocument(
						TEXT("/Game/Test/TestSubgraphQuery_Reimported"), ReimportDocument.AssetClass, ReimportDocument.bWorldBound,
						ReimportedNodes, ReimportedConnections, ReimportError);
					TestNotNull("Exported subgraph document should be independently re-importable", ReimportedAsset);
					if (ReimportedAsset)
					{
						TestEqual("Reimported asset should have exactly 3 nodes", ReimportedAsset->GetNodes().Num(), 3);
						ReimportedAsset->ClearFlags(RF_Standalone);
						ReimportedAsset->MarkAsGarbage();
					}
				}
			});

			It("returns a hard error when StartNodeGuid does not resolve to a node on the asset", [this, MakeTwoIslandsText, GuidA, GuidB, GuidC, GuidD, GuidE, GuidF]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestSubgraphQuery_ExportBadGuid");
				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeTwoIslandsText(GuidA, GuidB, GuidC, GuidD, GuidE, GuidF), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FString ExportedText;
				int32 NodeCount = 0;
				FString ErrorMessage;
				const bool bResult = FFlowGraphSubgraphQuery::ExportSubgraph(TestFlowAsset, FGuid::NewGuid(), ExportedText, NodeCount, ErrorMessage);

				TestFalse("ExportSubgraph should fail for a guid not present on the asset", bResult);
				TestFalse("Error message should not be empty", ErrorMessage.IsEmpty());
			});

			// Undirected traversal includes upstream data dependencies. These FormatText nodes
			// have no exec pins, so their only connection is the data edge.
			It("includes a node reached only via an upstream data-pin dependency, and excludes a genuinely disconnected node", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestSubgraphQuery_DataPinReachability");
				const FString GuidSource = TEXT("00000000-0000-0000-0000-0000000000D1");
				const FString GuidTarget = TEXT("00000000-0000-0000-0000-0000000000D2");
				const FString GuidDisconnected = TEXT("00000000-0000-0000-0000-0000000000D3");

				const FString ImportText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_FormatText\","
					"\"outputPins\":[{\"name\":\"Formatted Text\",\"type\":\"Text\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_FormatText\","
					"\"inputPins\":[{\"name\":\"FormatText\",\"type\":\"Text\"}],"
					"\"outputPins\":[{\"name\":\"Formatted Text\",\"type\":\"Text\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Formatted Text\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"FormatText\"}}"
					"]}"
				), *GuidSource, *GuidTarget, *GuidDisconnected, *GuidSource, *GuidTarget);

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, ImportText, ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid StartGuid, ExpectedSource, ExpectedTarget, ExpectedDisconnected;
				FGuid::Parse(GuidTarget, StartGuid);
				FGuid::Parse(GuidSource, ExpectedSource);
				FGuid::Parse(GuidTarget, ExpectedTarget);
				FGuid::Parse(GuidDisconnected, ExpectedDisconnected);

				FString ExportedText;
				int32 NodeCount = 0;
				FString ErrorMessage;
				// Start traversal from the *downstream* node (the data pin's receiver) - the
				// upstream source must still be picked up by the undirected BFS.
				const bool bResult = FFlowGraphSubgraphQuery::ExportSubgraph(TestFlowAsset, StartGuid, ExportedText, NodeCount, ErrorMessage);

				TestTrue("ExportSubgraph should succeed", bResult);
				TestEqual("Should include exactly the 2-node data-pin pair", NodeCount, 2);
				TestTrue("Exported text should mention the upstream data-pin source node", ExportedText.Contains(ExpectedSource.ToString()));
				TestTrue("Exported text should mention the downstream start node", ExportedText.Contains(ExpectedTarget.ToString()));
				TestFalse("Exported text should not mention the genuinely disconnected node", ExportedText.Contains(ExpectedDisconnected.ToString()));
			});
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
