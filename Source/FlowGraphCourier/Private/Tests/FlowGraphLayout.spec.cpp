// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FlowGraphLayout.h"
#include "Graph/FlowGraphEditorLayout.h"
#include "FlowGraphReconciler.h"
#include "FlowGraphValidation.h"
#include "FlowGraphImporter.h"
#include "FlowGraphRegrapher.h"
#include "FlowCourierConverter.h"
#include "FlowCourierDocument.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphNode_Comment.h"
#include "Json/FlowGraphLayoutFixtures.h"

BEGIN_DEFINE_SPEC(FFlowGraphLayoutSpec, "FlowGraphCourier.EditorGame.Layout", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UFlowAsset* TestFlowAsset = nullptr;

	// Imports a Courier v2 JSON fixture document and regraphs it - the document-based equivalent
	// of the deleted UFlowGraphRegrapher::ImportAndRegraphFromText. Same (Json, AssetPath) argument
	// order as the text-based function it replaces.
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

	static FFlowReconcileResult ApplyMutationText(const FString& AssetPath, const FString& MutationText, bool bDryRun, FString& OutErrorMessage)
	{
		TSharedPtr<FFlowReconcileExecutionPlan> Plan = FFlowGraphReconciler::ComputeReconcilePlan(AssetPath, MutationText, OutErrorMessage);
		if (!Plan)
		{
			return FFlowReconcileResult{};
		}
		if (bDryRun)
		{
			FFlowReconcileResult DryResult;
			DryResult.bSuccess = true;
			DryResult.Plan = Plan->Plan;
			DryResult.AliasMap = Plan->AliasMap;
			DryResult.ValidationFindings = Plan->ValidationFindings;
			DryResult.NodesTouched = Plan->NodesTouched;
			DryResult.NodesPreserved = Plan->NodesPreserved;
			DryResult.InputSizeBytes = Plan->InputSizeBytes;
			return DryResult;
		}
		return FFlowGraphReconciler::ExecuteReconcilePlan(*Plan, AssetPath, OutErrorMessage);
	}
END_DEFINE_SPEC(FFlowGraphLayoutSpec)

void FFlowGraphLayoutSpec::Define()
{
	Describe("FlowGraphLayout", [this]()
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

		Describe("Patch Placement", [this]()
		{
			// Acceptance criterion: "A new node without Pos lands at a non-zero, non-overlapping
			// position near its upstream source."
			It("auto-places a new node at a non-overlapping position near its source", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutAutoPlace");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A1");

				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}]}"
				), *GuidA);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FString ApplyError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GLayoutJson_AddNodeBFromA, false, ApplyError);
				if (!Result.bSuccess)
				{
					AddError(FString::Printf(TEXT("Apply failed: %s | Findings: %s | ValidationFindings: %d"),
						*ApplyError, *FString::Join(Result.Findings, TEXT(" ; ")), Result.ValidationFindings.Num()));
					for (const FFlowValidationFinding& Finding : Result.ValidationFindings)
					{
						AddError(FString::Printf(TEXT("  ValidationFinding: %s"), *Finding.Message));
					}
				}
				TestTrue("Apply should succeed", Result.bSuccess);

				const FGuid* NodeBGuid = Result.AliasMap.Find(TEXT("nodeB"));
				if (!TestTrue("nodeB alias should resolve", NodeBGuid != nullptr) || !NodeBGuid)
				{
					return;
				}

				UFlowNode* NodeB = TestFlowAsset->GetNode(*NodeBGuid);
				if (!TestNotNull("New node should exist on the asset", NodeB))
				{
					return;
				}

				const UEdGraphNode* GraphNodeB = NodeB->GetGraphNode();
				if (!TestNotNull("New node should have a live editor graph node after regraph", GraphNodeB))
				{
					return;
				}

				TestTrue("New node should be placed at a non-zero X (moved away from the source's column)", GraphNodeB->NodePosX != 0);
				TestEqual("New node should land one column right of its upstream source (A at X=0)", GraphNodeB->NodePosX, FFlowGraphLayout::ColumnSpacing);
				TestEqual("New node should stay on its source's row when nothing forces a nudge", GraphNodeB->NodePosY, 0);
			});

			// Acceptance criterion: "Adding a node does not move any existing node unless overlap
			// forced it; forced moves are minimal." This algorithm never moves existing nodes (see
			// FlowGraphLayout.h class comment) - overlap is always resolved by moving the new node
			// instead - so this asserts the stronger guarantee: existing positions are byte-for-byte
			// unchanged.
			It("leaves existing node positions unchanged when adding a node", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutPreserveExisting");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A2");
				const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000C2");

				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":600,\"y\":0},"
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]}"
					"]}"
				), *GuidA, *GuidC);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				// Insert a new Timer node between A and C, chained off A - well clear of C's column.
				FString ApplyError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GLayoutJson_InsertTimerNodeBFromA, false, ApplyError);
				if (!Result.bSuccess)
				{
					AddError(FString::Printf(TEXT("Apply failed: %s | Findings: %s"), *ApplyError, *FString::Join(Result.Findings, TEXT(" ; "))));
				}
				TestTrue("Apply should succeed", Result.bSuccess);

				FGuid NodeGuidA, NodeGuidC;
				FGuid::Parse(GuidA, NodeGuidA);
				FGuid::Parse(GuidC, NodeGuidC);
				UFlowNode* NodeA = TestFlowAsset->GetNode(NodeGuidA);
				UFlowNode* NodeC = TestFlowAsset->GetNode(NodeGuidC);
				if (TestNotNull("Node A should still exist", NodeA) && TestNotNull("Node A should have a graph node", NodeA->GetGraphNode()))
				{
					TestEqual("Node A's X should be unchanged", NodeA->GetGraphNode()->NodePosX, 0);
					TestEqual("Node A's Y should be unchanged", NodeA->GetGraphNode()->NodePosY, 0);
				}
				if (TestNotNull("Node C should still exist", NodeC) && TestNotNull("Node C should have a graph node", NodeC->GetGraphNode()))
				{
					TestEqual("Node C's X should be unchanged", NodeC->GetGraphNode()->NodePosX, 600);
					TestEqual("Node C's Y should be unchanged", NodeC->GetGraphNode()->NodePosY, 0);
				}
			});

			// Proves the overlap-nudge loop actually executes, not just dead code: an isolated new
			// node (no upstream connection) whose default stacking slot is already occupied by an
			// existing node must be nudged to the next free row.
			It("nudges an isolated new node to the next free row when its default slot is occupied", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutNudge");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A3");

				// FlowGraphLayout's default isolated-node stacking slot is (0, RowSpacing) - occupy
				// it explicitly so the new isolated node below is forced to nudge past it.
				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":%d},"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}"
					"]}"
				), *GuidA, FFlowGraphLayout::RowSpacing);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				// Isolated new node - no connection to/from it at all.
				FString ApplyError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GLayoutJson_AddIsolatedNode, false, ApplyError);
				TestTrue("Apply should succeed", Result.bSuccess);

				const FGuid* IsolatedGuid = Result.AliasMap.Find(TEXT("isolated"));
				if (!TestTrue("isolated alias should resolve", IsolatedGuid != nullptr) || !IsolatedGuid)
				{
					return;
				}

				UFlowNode* IsolatedNode = TestFlowAsset->GetNode(*IsolatedGuid);
				if (!TestNotNull("Isolated node should exist", IsolatedNode) || !TestNotNull("Isolated node should have a graph node", IsolatedNode->GetGraphNode()))
				{
					return;
				}

				TestEqual("Isolated node's default slot was occupied, so it should land in the next row instead",
					IsolatedNode->GetGraphNode()->NodePosY, FFlowGraphLayout::RowSpacing * 2);
			});
		});

		It("honors an explicit position on a node created through an alias", [this]()
		{
			// The explicit position and alias resolve to the same newly allocated node GUID.
			const FString AssetPath = TEXT("/Game/Test/TestLayoutAliasPosRegression");
			const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A7");

			const FString CreateText = FString::Printf(TEXT(
				"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
				"\"ops\":["
				"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},"
				"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}"
				"]}"
			), *GuidA);

			TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
			TestNotNull("Initial fixture import should succeed", TestFlowAsset);
			if (!TestFlowAsset) { return; }

			FString ErrorMessage;
			const FFlowReconcileResult Result = ApplyMutationText(
				AssetPath, GLayoutJson_AddTimerWithExplicitPos, false, ErrorMessage);

			TestTrue("Reconcile should succeed", Result.bSuccess);
			if (!Result.bSuccess) { return; }

			TestEqual("AliasMap should have one entry for 'timer'", Result.AliasMap.Num(), 1);
			const FGuid* MintedGuid = Result.AliasMap.Find(TEXT("timer"));
			TestNotNull("AliasMap should contain 'timer'", MintedGuid);
			if (!MintedGuid) { return; }

			const UFlowNode* NewNode = TestFlowAsset->GetNodes().FindRef(*MintedGuid);
			TestNotNull("New node should exist on the asset after apply", NewNode);
			if (!NewNode) { return; }

			const UEdGraphNode* GraphNode = NewNode->GetGraphNode();
			TestNotNull("New node should have an editor graph node", GraphNode);
			if (!GraphNode) { return; }

			// Critical: must match explicit Pos: 700,200, not the auto-placement fallback.
			TestEqual("New node X should match the explicit Pos: 700", GraphNode->NodePosX, 700);
			TestEqual("New node Y should match the explicit Pos: 200", GraphNode->NodePosY, 200);
		});

		Describe("Auto-Format", [this]()
		{
			It("auto-format produces deterministic left-to-right ranks with no overlaps", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutAutoFormat");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000F1");
				const FString GuidB = TEXT("00000000-0000-0000-0000-0000000000F2");
				const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000F3");
				const FString GuidD = TEXT("00000000-0000-0000-0000-0000000000F4");

				// A -> B -> D and A -> C -> D: B and C should land in the same rank (both are one
				// hop from A, two hops from D), A first, D last - proving both the longest-path
				// ranking and the same-rank non-overlap guarantee in one fixture. All start at the
				// same garbage position (0,0) to prove auto-format actually recomputes everything
				// rather than happening to already be correct.
				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_ExecutionSequence\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0}},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Timer\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},"
					"\"properties\":{\"CompletionTime\":\"1.0\",\"StepTime\":\"1.0\"},"
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
					"\"outputPins\":[{\"name\":\"Completed\",\"type\":\"Exec\"},{\"name\":\"Step\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Timer\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},"
					"\"properties\":{\"CompletionTime\":\"1.0\",\"StepTime\":\"1.0\"},"
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
					"\"outputPins\":[{\"name\":\"Completed\",\"type\":\"Exec\"},{\"name\":\"Step\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},"
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"0\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"1\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Completed\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Completed\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}"
					"]}"
				), *GuidA, *GuidB, *GuidC, *GuidD, *GuidA, *GuidB, *GuidA, *GuidC, *GuidB, *GuidD, *GuidC, *GuidD);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				TMap<FGuid, FIntPoint> Positions;
				UFlowGraphEditorLayout::ComputeAutoFormatPositions(TestFlowAsset, TSet<FGuid>(), Positions);

				FGuid NodeGuidA, NodeGuidB, NodeGuidC, NodeGuidD;
				FGuid::Parse(GuidA, NodeGuidA);
				FGuid::Parse(GuidB, NodeGuidB);
				FGuid::Parse(GuidC, NodeGuidC);
				FGuid::Parse(GuidD, NodeGuidD);

				const FIntPoint* PosA = Positions.Find(NodeGuidA);
				const FIntPoint* PosB = Positions.Find(NodeGuidB);
				const FIntPoint* PosC = Positions.Find(NodeGuidC);
				const FIntPoint* PosD = Positions.Find(NodeGuidD);
				if (!TestTrue("All 4 nodes should have a computed position", PosA && PosB && PosC && PosD))
				{
					return;
				}

				TestTrue("A should be strictly left of B (longest-path rank)", PosA->X < PosB->X);
				TestTrue("A should be strictly left of C", PosA->X < PosC->X);
				TestEqual("B and C should share the same rank (both one hop from A, symmetric)", PosB->X, PosC->X);
				TestTrue("D should be strictly right of B and C (it's downstream of both)", PosD->X > PosB->X && PosD->X > PosC->X);
				TestNotEqual("B and C should not overlap despite sharing a rank", PosB->Y, PosC->Y);

				TestEqual("Auto-format should produce exactly 4 unique positions (no overlaps at all)",
					TSet<FIntPoint>({ *PosA, *PosB, *PosC, *PosD }).Num(), 4);
			});

			// A partial selection keeps its original area and does not overlap unselected nodes.
			It("auto-formatting a selection does not overlap unselected nodes", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutSelectionNoOverlap");
				const FString GuidU = TEXT("00000000-0000-0000-0000-0000000000B1"); // unselected, sits at (0,0)
				const FString GuidX = TEXT("00000000-0000-0000-0000-0000000000B2"); // selected cluster, far from origin
				const FString GuidY = TEXT("00000000-0000-0000-0000-0000000000B3");

				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"bHasPosition\":true,\"position\":{\"x\":2000,\"y\":500},"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":2500,\"y\":900},"
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Out\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}"
					"]}"
				), *GuidU, *GuidX, *GuidY, *GuidX, *GuidY);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuidU, NodeGuidX, NodeGuidY;
				FGuid::Parse(GuidU, NodeGuidU);
				FGuid::Parse(GuidX, NodeGuidX);
				FGuid::Parse(GuidY, NodeGuidY);

				TMap<FGuid, FIntPoint> Positions;
				UFlowGraphEditorLayout::ComputeAutoFormatPositions(TestFlowAsset, TSet<FGuid>({ NodeGuidX, NodeGuidY }), Positions);

				TestEqual("Only the 2 selected nodes should get a computed position", Positions.Num(), 2);

				const FIntPoint* PosX = Positions.Find(NodeGuidX);
				const FIntPoint* PosY = Positions.Find(NodeGuidY);
				if (!TestTrue("Both selected nodes should have a computed position", PosX && PosY))
				{
					return;
				}

				// The formatted selection anchors to its own original bounding box.
				TestNotEqual("Selected node X must not land on unselected node U's position (0,0)", *PosX, FIntPoint::ZeroValue);
				TestEqual("Selected node X should anchor at the selection's own bounding-box origin", *PosX, FIntPoint(2000, 500));
				TestTrue("Selected node Y should be right of X, same row", PosY->X > PosX->X && PosY->Y == PosX->Y);
			});

			It("auto-formatting a selection preserves its approximate original location", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutSelectionPreservesLocation");
				const FString GuidP = TEXT("00000000-0000-0000-0000-0000000000C1"); // unselected, elsewhere entirely
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000C2");
				const FString GuidB = TEXT("00000000-0000-0000-0000-0000000000C3");
				const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000C4");

				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"bHasPosition\":true,\"position\":{\"x\":100,\"y\":100},"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_ExecutionSequence\",\"bHasPosition\":true,\"position\":{\"x\":5000,\"y\":1000}},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":5300,\"y\":1000},"
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":5300,\"y\":1300},"
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"0\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"1\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}"
					"]}"
				), *GuidP, *GuidA, *GuidB, *GuidC, *GuidA, *GuidB, *GuidA, *GuidC);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuidA, NodeGuidB, NodeGuidC;
				FGuid::Parse(GuidA, NodeGuidA);
				FGuid::Parse(GuidB, NodeGuidB);
				FGuid::Parse(GuidC, NodeGuidC);

				TMap<FGuid, FIntPoint> Positions;
				UFlowGraphEditorLayout::ComputeAutoFormatPositions(TestFlowAsset, TSet<FGuid>({ NodeGuidA, NodeGuidB, NodeGuidC }), Positions);

				TestEqual("Only the 3 selected nodes should get a computed position", Positions.Num(), 3);

				const FIntPoint* PosA = Positions.Find(NodeGuidA);
				const FIntPoint* PosB = Positions.Find(NodeGuidB);
				const FIntPoint* PosC = Positions.Find(NodeGuidC);
				if (!TestTrue("All 3 selected nodes should have a computed position", PosA && PosB && PosC))
				{
					return;
				}

				// The formatted cluster remains near its original (5000, 1000) location.
				const int32 MinX = FMath::Min3(PosA->X, PosB->X, PosC->X);
				const int32 MinY = FMath::Min3(PosA->Y, PosB->Y, PosC->Y);
				TestEqual("The reformatted selection's bounding-box origin X should match its pre-format origin", MinX, 5000);
				TestEqual("The reformatted selection's bounding-box origin Y should match its pre-format origin", MinY, 1000);
			});

			// Siblings from one predecessor follow its pin order. The fixture's GUID order
			// deliberately differs from pin declaration order.
			It("auto-format orders same-rank siblings by their shared predecessor's pin order, not GUID", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutPinOrderTiebreak");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000D1");
				const FString GuidFirstPinTarget = TEXT("00000000-0000-0000-0000-0000000000FA"); // alphabetically LAST, but fed from pin index 0
				const FString GuidSecondPinTarget = TEXT("00000000-0000-0000-0000-000000000001"); // alphabetically FIRST, but fed from pin index 1

				TestTrue("Sanity: the GUIDs must disagree with pin order for this test to be meaningful",
					GuidFirstPinTarget > GuidSecondPinTarget);

				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_ExecutionSequence\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0}},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},"
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},"
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"0\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"1\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}"
					"]}"
				), *GuidA, *GuidFirstPinTarget, *GuidSecondPinTarget, *GuidA, *GuidFirstPinTarget, *GuidA, *GuidSecondPinTarget);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuidFirst, NodeGuidSecond;
				FGuid::Parse(GuidFirstPinTarget, NodeGuidFirst);
				FGuid::Parse(GuidSecondPinTarget, NodeGuidSecond);

				TMap<FGuid, FIntPoint> Positions;
				UFlowGraphEditorLayout::ComputeAutoFormatPositions(TestFlowAsset, TSet<FGuid>(), Positions);

				const FIntPoint* PosFirst = Positions.Find(NodeGuidFirst);
				const FIntPoint* PosSecond = Positions.Find(NodeGuidSecond);
				if (!TestTrue("Both siblings should have a computed position", PosFirst && PosSecond))
				{
					return;
				}

				TestTrue("The node fed from pin index 0 should be ordered above (or equal to) the node fed from pin index 1, matching pin declaration order rather than GUID string",
					PosFirst->Y <= PosSecond->Y);
				TestNotEqual("The two siblings should not overlap", PosFirst->Y, PosSecond->Y);
			});

			It("auto-format preserves the authored regions of disconnected components", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutDisconnectedRegions");
				const FString GuidA = TEXT("00000000-0000-0000-0000-000000000401");
				const FString GuidB = TEXT("00000000-0000-0000-0000-000000000402");
				const FString GuidC = TEXT("00000000-0000-0000-0000-000000000403");
				const FString GuidD = TEXT("00000000-0000-0000-0000-000000000404");
				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":400,\"y\":0},\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"bHasPosition\":true,\"position\":{\"x\":3000,\"y\":100},\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":3400,\"y\":100},\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Out\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Out\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}]}"
				), *GuidA, *GuidB, *GuidC, *GuidD, *GuidA, *GuidB, *GuidC, *GuidD);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				UEdGraphNode_Comment* Comment = NewObject<UEdGraphNode_Comment>(TestFlowAsset->GetGraph());
				Comment->CreateNewGuid();
				Comment->NodePosX = -100;
				Comment->NodePosY = -100;
				Comment->NodeWidth = 4000;
				Comment->NodeHeight = 1000;
				TestFlowAsset->GetGraph()->AddNode(Comment, false, false);

				FGuid NodeGuidC, NodeGuidD;
				FGuid::Parse(GuidC, NodeGuidC);
				FGuid::Parse(GuidD, NodeGuidD);
				TMap<FGuid, FIntPoint> Positions;
				UFlowGraphEditorLayout::ComputeAutoFormatPositions(TestFlowAsset, TSet<FGuid>(), Positions);

				const FIntPoint& PosC = Positions.FindChecked(NodeGuidC);
				const FIntPoint& PosD = Positions.FindChecked(NodeGuidD);
				TestEqual("The distant component should retain its authored X region", PosC.X, 3000);
				TestEqual("The distant component should retain its authored Y region", PosC.Y, 100);
				TestTrue("The distant component should still flow left to right", PosD.X > PosC.X);
			});

			It("auto-format refits a comment around a partially overlapping node", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutPartialCommentOverlap");
				const FString GuidA = TEXT("00000000-0000-0000-0000-000000000405");
				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"bHasPosition\":true,\"position\":{\"x\":1000,\"y\":1000}}]}"
				), *GuidA);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				UEdGraphNode_Comment* Comment = NewObject<UEdGraphNode_Comment>(TestFlowAsset->GetGraph());
				Comment->CreateNewGuid();
				Comment->NodePosX = 900;
				Comment->NodePosY = 900;
				Comment->NodeWidth = 200;
				Comment->NodeHeight = 250;
				TestFlowAsset->GetGraph()->AddNode(Comment, false, false);

				FGuid NodeGuidA;
				FGuid::Parse(GuidA, NodeGuidA);
				const TMap<FGuid, FIntPoint> Sizes({ { NodeGuidA, FIntPoint(300, 400) } });
				TMap<FGuid, FIntPoint> Positions;
				UFlowGraphEditorLayout::ComputeAutoFormatPositionsWithSizes(
					TestFlowAsset, TSet<FGuid>(), Sizes, Positions);
				UFlowGraphEditorLayout::ApplyPositionsToExistingGraphWithSizes(TestFlowAsset, Positions, Sizes);

				const FIntPoint& Position = Positions.FindChecked(NodeGuidA);
				TestTrue("Comment should contain the partially overlapping node's top-left",
					Position.X >= Comment->NodePosX && Position.Y >= Comment->NodePosY);
				TestTrue("Comment should contain the partially overlapping node's bottom-right",
					Position.X + Sizes.FindChecked(NodeGuidA).X <= Comment->NodePosX + Comment->NodeWidth
					&& Position.Y + Sizes.FindChecked(NodeGuidA).Y <= Comment->NodePosY + Comment->NodeHeight);
			});

			It("auto-format uses measured node rectangles when spacing a rank", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutMeasuredRectangles");
				const FString GuidA = TEXT("00000000-0000-0000-0000-000000000101");
				const FString GuidB = TEXT("00000000-0000-0000-0000-000000000102");
				const FString GuidC = TEXT("00000000-0000-0000-0000-000000000103");
				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_ExecutionSequence\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0}},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0},\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"0\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"1\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}]}"
				), *GuidA, *GuidB, *GuidC, *GuidA, *GuidB, *GuidA, *GuidC);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuidA, NodeGuidB, NodeGuidC;
				FGuid::Parse(GuidA, NodeGuidA);
				FGuid::Parse(GuidB, NodeGuidB);
				FGuid::Parse(GuidC, NodeGuidC);
				const TMap<FGuid, FIntPoint> Sizes({
					{ NodeGuidA, FIntPoint(300, 140) },
					{ NodeGuidB, FIntPoint(420, 520) },
					{ NodeGuidC, FIntPoint(280, 120) }
				});
				TMap<FGuid, FIntPoint> Positions;
				UFlowGraphEditorLayout::ComputeAutoFormatPositionsWithSizes(
					TestFlowAsset, TSet<FGuid>(), Sizes, Positions);

				const FIntPoint& PosB = Positions.FindChecked(NodeGuidB);
				const FIntPoint& PosC = Positions.FindChecked(NodeGuidC);
				const FIntRect RectB(PosB, PosB + Sizes.FindChecked(NodeGuidB));
				const FIntRect RectC(PosC, PosC + Sizes.FindChecked(NodeGuidC));
				TestFalse("Measured same-rank rectangles should not overlap",
					RectB.Min.X < RectC.Max.X && RectB.Max.X > RectC.Min.X
					&& RectB.Min.Y < RectC.Max.Y && RectB.Max.Y > RectC.Min.Y);
				TestTrue("Measured rectangles should retain vertical breathing room",
					FMath::Abs(PosB.Y - PosC.Y) >= FMath::Min(Sizes.FindChecked(NodeGuidB).Y,
						Sizes.FindChecked(NodeGuidC).Y));
			});

			It("auto-format keeps a directed cycle compact and deterministic", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutDirectedCycle");
				const FString GuidA = TEXT("00000000-0000-0000-0000-000000000201");
				const FString GuidB = TEXT("00000000-0000-0000-0000-000000000202");
				const FString GuidC = TEXT("00000000-0000-0000-0000-000000000203");
				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Timer\",\"bHasPosition\":true,\"position\":{\"x\":0,\"y\":0}},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Timer\",\"bHasPosition\":true,\"position\":{\"x\":300,\"y\":0}},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Timer\",\"bHasPosition\":true,\"position\":{\"x\":600,\"y\":0}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Completed\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Completed\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Completed\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}]}"
				), *GuidA, *GuidB, *GuidC, *GuidA, *GuidB, *GuidB, *GuidC, *GuidC, *GuidA);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				TMap<FGuid, FIntPoint> FirstPositions;
				TMap<FGuid, FIntPoint> SecondPositions;
				UFlowGraphEditorLayout::ComputeAutoFormatPositions(TestFlowAsset, TSet<FGuid>(), FirstPositions);
				UFlowGraphEditorLayout::ComputeAutoFormatPositions(TestFlowAsset, TSet<FGuid>(), SecondPositions);
				TestTrue("Cycle layout should be deterministic",
					FirstPositions.OrderIndependentCompareEqual(SecondPositions));

				int32 MinX = MAX_int32;
				int32 MaxX = MIN_int32;
				for (const TPair<FGuid, FIntPoint>& Pair : FirstPositions)
				{
					MinX = FMath::Min(MinX, Pair.Value.X);
					MaxX = FMath::Max(MaxX, Pair.Value.X);
				}
				TestTrue("A three-node cycle should not inflate into dozens of ranks", MaxX - MinX < 2000);
			});

			It("auto-format refits a comment around the same member nodes", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestLayoutCommentMembership");
				const FString GuidA = TEXT("00000000-0000-0000-0000-000000000301");
				const FString GuidB = TEXT("00000000-0000-0000-0000-000000000302");
				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"bHasPosition\":true,\"position\":{\"x\":1000,\"y\":1000}},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"bHasPosition\":true,\"position\":{\"x\":1400,\"y\":1000},\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Out\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}]}"
				), *GuidA, *GuidB, *GuidA, *GuidB);

				TestFlowAsset = ImportAndRegraphFixtureDocument(CreateText, AssetPath);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				UEdGraphNode_Comment* Comment = NewObject<UEdGraphNode_Comment>(TestFlowAsset->GetGraph());
				Comment->CreateNewGuid();
				Comment->NodePosX = 900;
				Comment->NodePosY = 900;
				Comment->NodeWidth = 1000;
				Comment->NodeHeight = 500;
				TestFlowAsset->GetGraph()->AddNode(Comment, false, false);

				FGuid NodeGuidA, NodeGuidB;
				FGuid::Parse(GuidA, NodeGuidA);
				FGuid::Parse(GuidB, NodeGuidB);
				const TMap<FGuid, FIntPoint> Sizes({
					{ NodeGuidA, FIntPoint(300, 140) },
					{ NodeGuidB, FIntPoint(360, 180) }
				});
				TMap<FGuid, FIntPoint> Positions;
				UFlowGraphEditorLayout::ComputeAutoFormatPositionsWithSizes(
					TestFlowAsset, TSet<FGuid>(), Sizes, Positions);
				UFlowGraphEditorLayout::ApplyPositionsToExistingGraphWithSizes(
					TestFlowAsset, Positions, Sizes);

				for (const FGuid& Guid : { NodeGuidA, NodeGuidB })
				{
					const FIntPoint Position = Positions.FindChecked(Guid);
					const FIntPoint Size = Sizes.FindChecked(Guid);
					TestTrue("Comment should contain the member node's top-left",
						Position.X >= Comment->NodePosX && Position.Y >= Comment->NodePosY);
					TestTrue("Comment should contain the member node's bottom-right",
						Position.X + Size.X <= Comment->NodePosX + Comment->NodeWidth
						&& Position.Y + Size.Y <= Comment->NodePosY + Comment->NodeHeight);
				}
			});
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
