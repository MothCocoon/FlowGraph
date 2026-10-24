// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FlowGraphReconciler.h"
#include "FlowGraphImporter.h"
#include "FlowGraphRegrapher.h"
#include "Graph/FlowGraph.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowCourierConverter.h"
#include "FlowCourierDocument.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowPin.h"
#include "UObject/UnrealType.h"
#include "Misc/PackageName.h"
#include "AddOns/FlowNodeAddOn.h"
#include "AddOns/FlowNodeAddOn_SwitchCase.h"
#include "Json/FlowGraphReconcilerFixtures.h"

#include "Nodes/Actor/FlowNode_ExecuteComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"

BEGIN_DEFINE_SPEC(FFlowGraphReconcilerSpec, "FlowGraphCourier.EditorGame.Reconciler", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UFlowAsset* TestFlowAsset = nullptr;
	FString TestImportText;

	// Imports a Courier v2 JSON fixture document into a brand-new asset - the document-based
	// equivalent of the deleted UFlowGraphImporter::ImportFlowGraphFromText, used only to build
	// each test's starting asset (never for the mutation under test, which goes through
	// FFlowGraphReconciler::ComputeReconcilePlan/ExecuteReconcilePlan directly).
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

	// Counts editor nodes standing in for a runtime UFlowNode. Addon sub-nodes and comment nodes are
	// excluded: neither has an entry in UFlowAsset::Nodes, so neither belongs in a parity count.
	static int32 CountTopLevelFlowGraphNodes(const UFlowAsset* FlowAsset)
	{
		const UFlowGraph* FlowGraph = FlowAsset ? Cast<UFlowGraph>(FlowAsset->GetGraph()) : nullptr;
		if (!FlowGraph)
		{
			return 0;
		}

		int32 Count = 0;
		for (const UEdGraphNode* EdNode : FlowGraph->Nodes)
		{
			const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
			if (FlowGraphNode && !FlowGraphNode->GetParentNode() && Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase()))
			{
				++Count;
			}
		}
		return Count;
	}

	// Frees the asset's object path so a later run of the same test creates the asset again instead
	// of patching the one the previous run left behind. MarkAsGarbage alone does not do this: the
	// object keeps its path until it is collected, and the reconciler resolves a target by name so it
	// can see assets created earlier in the same session. Without this, every create-path test
	// silently becomes a patch-path test on a second run and fails reporting zero nodes added.
	static void DiscardTestAsset(UFlowAsset* FlowAsset)
	{
		if (!FlowAsset)
		{
			return;
		}

		UPackage* Package = FlowAsset->GetOutermost();

		// A unique name is required rather than reusing the current one: an earlier discard in this
		// same session may still be holding that name in the transient package, uncollected.
		const FName TransientName = MakeUniqueObjectName(GetTransientPackage(), FlowAsset->GetClass(), FlowAsset->GetFName());
		FlowAsset->ClearFlags(RF_Standalone | RF_Public);
		FlowAsset->Rename(*TransientName.ToString(), GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
		FlowAsset->MarkAsGarbage();

		// These tests never save. Leaving the package dirty only invites someone to save fixture junk
		// into the project.
		if (Package && Package != GetTransientPackage())
		{
			Package->SetDirtyFlag(false);
		}
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
END_DEFINE_SPEC(FFlowGraphReconcilerSpec)

void FFlowGraphReconcilerSpec::Define()
{
	Describe("FlowGraphReconciler", [this]()
	{
		BeforeEach([this]()
		{
			TestFlowAsset = nullptr;
			TestImportText.Empty();
		});

		AfterEach([this]()
		{
			DiscardTestAsset(TestFlowAsset);
			TestFlowAsset = nullptr;
		});

		// Three-node fixture: A (Start) -> B (Timer) -> C (Finish), an exec chain using node types
		// and pin names already proven to import correctly via FlowGraphImporter.spec.cpp.
		// B's CompletionTime float property is the distinguishing value for property-identity checks.
		auto MakeThreeNodeText = [](const FString& AGuid, const FString& BGuid, const FString& CGuid, const FString& BCompletionTime) -> FString
		{
			return FString::Printf(TEXT(
				"{"
				"\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
				"\"ops\":["
				"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Timer\",\"properties\":{\"CompletionTime\":\"%s\",\"StepTime\":\"1.0\"},"
				"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],\"outputPins\":[{\"name\":\"Completed\",\"type\":\"Exec\"},{\"name\":\"Step\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
				"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Out\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}},"
				"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Completed\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}"
				"]}"
			), *AGuid, *BGuid, *BCompletionTime, *CGuid, *AGuid, *BGuid, *BGuid, *CGuid);
		};

		// Spec-scope constants - defined here (not inside BeforeAll) so Describe captures below
		// can copy them by value at lambda definition time, which is legal and safe.
		const FString GuidA = TEXT("00000000-0000-0000-0000-00000000000A");
		const FString GuidB = TEXT("00000000-0000-0000-0000-00000000000B");
		const FString GuidC = TEXT("00000000-0000-0000-0000-00000000000C");

		Describe("Preserve Untouched Identity", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("preserves untouched node identities when editing one property", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcilePreserveIdentity");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuidA, NodeGuidB, NodeGuidC;
				FGuid::Parse(GuidA, NodeGuidA);
				FGuid::Parse(GuidB, NodeGuidB);
				FGuid::Parse(GuidC, NodeGuidC);

				UFlowNode* OriginalA = TestFlowAsset->GetNode(NodeGuidA);
				UFlowNode* OriginalB = TestFlowAsset->GetNode(NodeGuidB);
				UFlowNode* OriginalC = TestFlowAsset->GetNode(NodeGuidC);
				TestNotNull("Node A should exist", OriginalA);
				TestNotNull("Node B should exist", OriginalB);
				TestNotNull("Node C should exist", OriginalC);

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_UpdateNodeBProperty, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);

				UFlowNode* AfterA = TestFlowAsset->GetNode(NodeGuidA);
				UFlowNode* AfterB = TestFlowAsset->GetNode(NodeGuidB);
				UFlowNode* AfterC = TestFlowAsset->GetNode(NodeGuidC);

				TestEqual("Node A should be the same UObject instance", AfterA, OriginalA);
				TestEqual("Node B should be the same UObject instance", AfterB, OriginalB);
				TestEqual("Node C should be the same UObject instance", AfterC, OriginalC);

				FProperty* CompletionTimeProperty = AfterB ? AfterB->GetClass()->FindPropertyByName(TEXT("CompletionTime")) : nullptr;
				if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(CompletionTimeProperty))
				{
					TestEqual("Node B's CompletionTime should reflect the mutation", FloatProperty->GetPropertyValue(FloatProperty->ContainerPtrToValuePtr<void>(AfterB)), 9.0f);
				}
				else
				{
					AddError(TEXT("CompletionTime property should be found on node B"));
				}

				TestEqual("Plan should report exactly one updated node", Result.Plan.NodesUpdated.Num(), 1);
				TestTrue("Plan should report node B as updated", Result.Plan.NodesUpdated.Contains(NodeGuidB));
				TestEqual("Plan should report no added nodes", Result.Plan.NodesAdded.Num(), 0);
				TestEqual("Plan should report no deleted nodes", Result.Plan.NodesDeleted.Num(), 0);
				TestEqual("NodesPreserved should count A and C", Result.NodesPreserved, 2);
			});
		});

		// Updating an existing node applies newly declared pins with their real types before
		// connections to those pins are resolved.
		Describe("Reconcile Applies Declared Pins To Updated Node", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("adds a newly-declared output pin with its real type when updating an existing node", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileApplyDeclaredPinsOnUpdate");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuidA;
				FGuid::Parse(GuidA, NodeGuidA);

				UFlowNode* OriginalA = TestFlowAsset->GetNode(NodeGuidA);
				TestNotNull("Node A should exist", OriginalA);
				if (OriginalA)
				{
					const bool bScorePinAlreadyPresent = OriginalA->GetOutputPins().ContainsByPredicate([](const FFlowPin& Pin) { return Pin.PinName == TEXT("Score"); });
					TestFalse("Score pin should not exist before the mutation", bScorePinAlreadyPresent);
				}

				// Node A (FlowNode_Start) is re-declared with its existing exec pin plus a new "Score" data pin -
				// same shape as a mutation document re-declaring an existing node's dynamic/declared pins.
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_AddScorePinOnNodeA, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestTrue("Plan should report node A as updated", Result.Plan.NodesUpdated.Contains(NodeGuidA));

				UFlowNode* AfterA = TestFlowAsset->GetNode(NodeGuidA);
				TestEqual("Node A should be the same UObject instance", AfterA, OriginalA);
				if (!AfterA) { return; }

				const FFlowPin* ScorePin = AfterA->GetOutputPins().FindByPredicate([](const FFlowPin& Pin) { return Pin.PinName == TEXT("Score"); });
				TestNotNull("Score pin should exist on node A after the mutation", ScorePin);
				if (ScorePin)
				{
					TestEqual("Score pin should carry its declared Float type, not a wildcard stand-in", ScorePin->GetPinTypeName().ToString(), FString(TEXT("Float")));
				}
			});
		});

		Describe("Node Add", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("adds a new node via the NEW alias and returns the minted GUID in the alias map", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileAddNode");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_AddNodeViaAlias, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestEqual("Plan should report exactly one added node", Result.Plan.NodesAdded.Num(), 1);

				const FGuid* MintedGuid = Result.AliasMap.Find(TEXT("extraTimer"));
				TestNotNull("Alias map should contain the alias", MintedGuid);
				if (MintedGuid)
				{
					UFlowNode* NewNode = TestFlowAsset->GetNode(*MintedGuid);
					TestNotNull("The minted GUID should resolve to a real node on the asset", NewNode);
				}

				TestEqual("Asset should now have 4 nodes", TestFlowAsset->GetNodes().Num(), 4);
			});
		});

		Describe("Alias In Connections", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("connects a new:<alias> node referenced from a connection line in the same document", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileAliasInConnections");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				// B.Completed already points to C from the fixture - retargeting it to the new
				// aliased node in the same document proves the alias resolves before connections
				// are applied, not just that node creation itself succeeds.
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_AddNodeViaAliasAndConnect, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);

				const FGuid* MintedGuid = Result.AliasMap.Find(TEXT("extraFinish"));
				TestNotNull("Alias map should contain the alias", MintedGuid);
				if (!MintedGuid) { return; }

				FGuid NodeGuidB;
				FGuid::Parse(GuidB, NodeGuidB);
				UFlowNode* NodeB = TestFlowAsset->GetNode(NodeGuidB);
				TestNotNull("Node B should exist", NodeB);
				if (!NodeB) { return; }

				const FConnectedPin ActualConnection = NodeB->GetConnection(TEXT("Completed"));
				TestEqual("B.Completed should now point at the aliased node's minted GUID", ActualConnection.NodeGuid, *MintedGuid);
				TestEqual("B.Completed should target the In pin", ActualConnection.PinName, FName(TEXT("In")));
			});

			It("fails with a clear error when a connection references an alias that was never defined", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileUnknownAlias");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_ConnectionToUndefinedAlias, false, ReconcileError);
				TestFalse("Reconcile should fail", Result.bSuccess);
				const bool bFindingMentionsAlias = Result.ValidationFindings.ContainsByPredicate(
					[](const FFlowValidationFinding& Finding) { return Finding.Message.Contains(TEXT("neverDefined")); });
				TestTrue("A validation finding should mention the unresolved alias", bFindingMentionsAlias);
			});
		});

		Describe("Node Delete", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("deletes a node and drops only its referencing connections", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileDeleteNode");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuidA, NodeGuidB, NodeGuidC;
				FGuid::Parse(GuidA, NodeGuidA);
				FGuid::Parse(GuidB, NodeGuidB);
				FGuid::Parse(GuidC, NodeGuidC);

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_DeleteNodeB, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestEqual("Plan should report exactly one deleted node", Result.Plan.NodesDeleted.Num(), 1);

				TestNull("Node B should no longer exist", TestFlowAsset->GetNode(NodeGuidB));
				TestEqual("Asset should now have 2 nodes", TestFlowAsset->GetNodes().Num(), 2);

				UFlowNode* NodeA = TestFlowAsset->GetNode(NodeGuidA);
				if (TestNotNull("Node A should still exist", NodeA))
				{
					FConnectedPin Connection = NodeA->GetConnection(TEXT("Out"));
					TestFalse("Node A's connection to the deleted node B should be gone", Connection.NodeGuid == NodeGuidB);
				}
			});

			// A delete that removes only the runtime half leaves a node that is still drawn, still
			// holds its UFlowNode alive in the package, and whose surviving pin links get harvested
			// back into the runtime map by the next regraph - the "ghost node" defect.
			It("destroys the editor node as well, leaving no node drawn without a runtime counterpart", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileDeleteNodeEditorHalf");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				TestTrue("Fixture should regraph", UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset));
				TestEqual("Every runtime node should have an editor node before the delete", CountTopLevelFlowGraphNodes(TestFlowAsset), 3);

				FGuid NodeGuidB;
				FGuid::Parse(GuidB, NodeGuidB);

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_DeleteNodeB, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);

				TestNull("Node B should no longer exist at runtime", TestFlowAsset->GetNode(NodeGuidB));
				TestEqual("The deleted node's editor node should be gone too", CountTopLevelFlowGraphNodes(TestFlowAsset), 2);

				TArray<FString> ParityIssues;
				UFlowGraphRegrapher::CollectGraphParityIssues(TestFlowAsset, ParityIssues);
				TestEqual("Editor graph and runtime map should agree after the delete", ParityIssues.Num(), 0);
			});
		});

		Describe("Orphaned Editor Node Repair", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			// Reproduces an asset already damaged by a runtime-only delete, then proves a regraph
			// both detects and repairs it rather than preserving the ghost forever.
			It("prunes an editor node whose runtime node is gone, and reports it until it is pruned", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileOrphanedEditorNode");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				TestTrue("Fixture should regraph", UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset));

				FGuid NodeGuidB;
				FGuid::Parse(GuidB, NodeGuidB);

				// Unregister only the runtime node to create an orphaned editor node.
				TestFlowAsset->UnregisterNode(NodeGuidB);
				TestEqual("The orphaned editor node should still be present", CountTopLevelFlowGraphNodes(TestFlowAsset), 3);

				TArray<FString> ParityIssues;
				UFlowGraphRegrapher::CollectGraphParityIssues(TestFlowAsset, ParityIssues);
				TestTrue("Parity check should report the orphaned editor node", ParityIssues.Num() > 0);

				TestEqual("Pruning should destroy exactly the orphaned node", UFlowGraphRegrapher::PruneOrphanedEditorNodes(TestFlowAsset), 1);
				TestEqual("Only the two live nodes should remain drawn", CountTopLevelFlowGraphNodes(TestFlowAsset), 2);

				UFlowGraphRegrapher::CollectGraphParityIssues(TestFlowAsset, ParityIssues);
				TestEqual("Parity check should be clean after pruning", ParityIssues.Num(), 0);
			});

			// Node-level parity alone would call this asset healthy: every node is both drawn and
			// registered. Only comparing the wires catches a connection that executes without being
			// drawn, which is how a graph can read correctly and behave otherwise.
			It("reports a runtime connection that has no editor link", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileWireParity");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				TestTrue("Fixture should regraph", UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset));

				TArray<FString> ParityIssues;
				UFlowGraphRegrapher::CollectGraphParityIssues(TestFlowAsset, ParityIssues);
				TestEqual("A freshly regraphed fixture should have no parity issues", ParityIssues.Num(), 0);

				FGuid NodeGuidA;
				FGuid::Parse(GuidA, NodeGuidA);

				// Break only the editor half of A.Out -> B.In, leaving the runtime connection intact.
				UFlowGraph* FlowGraph = Cast<UFlowGraph>(TestFlowAsset->GetGraph());
				UFlowGraphNode* GraphNodeA = nullptr;
				if (FlowGraph)
				{
					for (UEdGraphNode* EdNode : FlowGraph->Nodes)
					{
						UFlowGraphNode* Candidate = Cast<UFlowGraphNode>(EdNode);
						const UFlowNode* CandidateFlowNode = Candidate ? Cast<UFlowNode>(Candidate->GetFlowNodeBase()) : nullptr;
						if (CandidateFlowNode && CandidateFlowNode->GetGuid() == NodeGuidA)
						{
							GraphNodeA = Candidate;
							break;
						}
					}
				}
				if (!TestNotNull("Node A's editor node should be findable", GraphNodeA)) { return; }

				for (UEdGraphPin* Pin : GraphNodeA->Pins)
				{
					if (Pin && Pin->PinName == TEXT("Out"))
					{
						Pin->BreakAllPinLinks();
					}
				}

				UFlowGraphRegrapher::CollectGraphParityIssues(TestFlowAsset, ParityIssues);
				TestTrue("Parity check should report the undrawn runtime connection", ParityIssues.ContainsByPredicate(
					[](const FString& Issue) { return Issue.Contains(TEXT("has no editor link")); }));
			});
		});

		Describe("AddOn Reconcile", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("deletes a single addon by GUID leaving siblings intact", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileDeleteAddOn");
				const FString GuidNodeA1 = TEXT("00000000-0000-0000-0000-0000000000A1");
				const FString GuidAddOnD = TEXT("00000000-0000-0000-0000-0000000000D1");
				const FString GuidAddOnE = TEXT("00000000-0000-0000-0000-0000000000E1");

				// Fixture: node A1 (Start) with two SwitchCase addons, D1 and E1.
				const FString FixtureText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseD\"}},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseE\"}}"
					"]}"
				), *GuidNodeA1, *GuidAddOnD, *GuidNodeA1, *GuidAddOnE, *GuidNodeA1);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, FixtureText, ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuidA, AddOnGuidD, AddOnGuidE;
				FGuid::Parse(GuidNodeA1, NodeGuidA);
				FGuid::Parse(GuidAddOnD, AddOnGuidD);
				FGuid::Parse(GuidAddOnE, AddOnGuidE);

				UFlowNode* NodeA = TestFlowAsset->GetNode(NodeGuidA);
				TestNotNull("Node A should exist", NodeA);
				if (!NodeA) { return; }
				TestEqual("Node A should start with 2 addons", NodeA->GetFlowNodeAddOnChildren().Num(), 2);

				UFlowNodeAddOn* OriginalAddOnE = nullptr;
				for (UFlowNodeAddOn* AddOn : NodeA->GetFlowNodeAddOnChildren())
				{
					if (AddOn && AddOn->GetGuid() == AddOnGuidE)
					{
						OriginalAddOnE = AddOn;
					}
				}
				TestNotNull("AddOn E should exist before the mutation", OriginalAddOnE);

				// Mutate node A1, removing addon D by GUID. Addon E is not mentioned at all - it
				// should be left alone (this block is not marked bReplaceAddons - see the
				// "bReplaceAddons" describe block below for the delete-by-omission counterpart).
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_DeleteOneAddonLeavesSibling, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestEqual("Plan should report exactly one deleted addon", Result.Plan.AddOnsDeleted.Num(), 1);
				const bool bReportedD = Result.Plan.AddOnsDeleted.ContainsByPredicate(
					[&AddOnGuidD](const FFlowReconcileAddonEntry& Entry) { return Entry.AddOnGuid == AddOnGuidD; });
				TestTrue("Plan should report addon D as deleted", bReportedD);

				const TArray<UFlowNodeAddOn*>& RemainingAddOns = NodeA->GetFlowNodeAddOnChildren();
				TestEqual("Node A should now have exactly 1 addon", RemainingAddOns.Num(), 1);

				bool bFoundD = false;
				UFlowNodeAddOn* RemainingAddOnE = nullptr;
				for (UFlowNodeAddOn* AddOn : RemainingAddOns)
				{
					if (AddOn && AddOn->GetGuid() == AddOnGuidD) { bFoundD = true; }
					if (AddOn && AddOn->GetGuid() == AddOnGuidE) { RemainingAddOnE = AddOn; }
				}
				TestFalse("AddOn D should no longer be present", bFoundD);
				TestEqual("AddOn E should be the same UObject instance", RemainingAddOnE, OriginalAddOnE);
			});

			It("reports an added addon with owner node guid, resolved type, and document properties", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileAddOnAddedEntry");
				const FString GuidNodeA1 = TEXT("00000000-0000-0000-0000-0000000000A1");

				// Fixture: node A1 (Start) with no addons.
				const FString FixtureText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}]}"
				), *GuidNodeA1);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, FixtureText, ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuidA;
				FGuid::Parse(GuidNodeA1, NodeGuidA);

				// Add a SwitchCase addon (via newAlias) to node A1.
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_AddAddonToNodeA1, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestEqual("Plan should report exactly one added addon", Result.Plan.AddOnsAdded.Num(), 1);
				if (Result.Plan.AddOnsAdded.Num() != 1) { return; }

				const FFlowReconcileAddonEntry& Entry = Result.Plan.AddOnsAdded[0];
				TestEqual("Added entry should carry the owning node guid", Entry.OwnerNodeGuid, NodeGuidA);
				const FGuid* MintedAddOnGuid = Result.AliasMap.Find(TEXT("newCase"));
				TestNotNull("Alias map should contain the minted addon guid", MintedAddOnGuid);
				if (MintedAddOnGuid) { TestEqual("Added entry should carry the minted addon guid", Entry.AddOnGuid, *MintedAddOnGuid); }
				TestTrue("Added entry type should be the SwitchCase class", Entry.AddOnType.Contains(TEXT("FlowNodeAddOn_SwitchCase")));
				const FString* CaseNameValue = Entry.Properties.Find(TEXT("CaseName"));
				TestNotNull("Added entry should carry the CaseName property from the document", CaseNameValue);
				if (CaseNameValue) { TestEqual("CaseName property value should round-trip", *CaseNameValue, FString(TEXT("CaseF"))); }
			});

			It("reports an updated addon with owner node guid and the changed property", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileAddOnUpdatedEntry");
				const FString GuidNodeA1 = TEXT("00000000-0000-0000-0000-0000000000A1");
				const FString GuidAddOnF1 = TEXT("00000000-0000-0000-0000-0000000000F1");

				// Fixture: node A1 (Start) with one SwitchCase addon F1 (CaseName CaseF).
				const FString FixtureText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseF\"}}"
					"]}"
				), *GuidNodeA1, *GuidAddOnF1, *GuidNodeA1);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, FixtureText, ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuidA, AddOnGuidF;
				FGuid::Parse(GuidNodeA1, NodeGuidA);
				FGuid::Parse(GuidAddOnF1, AddOnGuidF);

				// Update addon F1's CaseName in place (matched by GUID).
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_UpdateAddonCaseName, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestEqual("Plan should report exactly one updated addon", Result.Plan.AddOnsUpdated.Num(), 1);
				if (Result.Plan.AddOnsUpdated.Num() != 1) { return; }

				const FFlowReconcileAddonEntry& Entry = Result.Plan.AddOnsUpdated[0];
				TestEqual("Updated entry should carry the owning node guid", Entry.OwnerNodeGuid, NodeGuidA);
				TestEqual("Updated entry should carry the addon guid", Entry.AddOnGuid, AddOnGuidF);
				const FString* CaseNameValue = Entry.Properties.Find(TEXT("CaseName"));
				TestNotNull("Updated entry should carry the changed CaseName property", CaseNameValue);
				if (CaseNameValue) { TestEqual("CaseName should reflect the new value", *CaseNameValue, FString(TEXT("CaseFRenamed"))); }
			});

			It("reports a deleted addon with owner node guid and the live class type", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileAddOnDeletedEntry");
				const FString GuidNodeA1 = TEXT("00000000-0000-0000-0000-0000000000A1");
				const FString GuidAddOnF1 = TEXT("00000000-0000-0000-0000-0000000000F1");

				const FString FixtureText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseF\"}}"
					"]}"
				), *GuidNodeA1, *GuidAddOnF1, *GuidNodeA1);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, FixtureText, ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuidA, AddOnGuidF;
				FGuid::Parse(GuidNodeA1, NodeGuidA);
				FGuid::Parse(GuidAddOnF1, AddOnGuidF);

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_DeleteAddonF1, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestEqual("Plan should report exactly one deleted addon", Result.Plan.AddOnsDeleted.Num(), 1);
				if (Result.Plan.AddOnsDeleted.Num() != 1) { return; }

				const FFlowReconcileAddonEntry& Entry = Result.Plan.AddOnsDeleted[0];
				TestEqual("Deleted entry should carry the owning node guid", Entry.OwnerNodeGuid, NodeGuidA);
				TestEqual("Deleted entry should carry the addon guid", Entry.AddOnGuid, AddOnGuidF);
				TestTrue("Deleted entry type should be read from the live object", Entry.AddOnType.Contains(TEXT("FlowNodeAddOn_SwitchCase")));
				TestTrue("Deleted entry should carry no document properties", Entry.Properties.IsEmpty());
			});
		});

		// Own fixture (not MakeThreeNodeText): FlowNode_ExecuteComponent's Instanced ComponentTemplate
		// is the only shipped property this feature can construct into, and it is unrelated to the
		// three-node exec chain the rest of this file's fixtures share.
		Describe("Instanced Subobject Identity On Reconcile", [this]()
		{
			It("keeps the same subobject instance when re-applying the same class with changed fields", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileInstancedSameClass");
				const FString NodeGuidText = TEXT("00000000-0000-0000-0000-0000000000F1");

				const FString FixtureText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_ExecuteComponent\","
					"\"properties\":{\"ComponentTemplate\":\"/Script/Engine.BillboardComponent(ScreenSize=1.000000)\"}}]}"
				), *NodeGuidText);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, FixtureText, ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuid;
				FGuid::Parse(NodeGuidText, NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Node should exist", Node);
				if (!Node) { return; }

				FProperty* TemplateProperty = Node->GetClass()->FindPropertyByName(TEXT("ComponentTemplate"));
				FObjectProperty* ObjectProperty = CastField<FObjectProperty>(TemplateProperty);
				TestNotNull("ComponentTemplate should be an object property", ObjectProperty);
				if (!ObjectProperty) { return; }

				UObject* OriginalSubobject = ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(Node));
				TestNotNull("Subobject should exist after the fixture import", OriginalSubobject);
				if (!OriginalSubobject) { return; }

				const FString ReapplyText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Patch\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"properties\":{\"ComponentTemplate\":\"/Script/Engine.BillboardComponent(ScreenSize=9.000000)\"}}]}"
				), *NodeGuidText);

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, ReapplyText, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);

				UObject* SubobjectAfterReapply = ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(Node));
				TestEqual("Re-applying the same class should preserve the subobject's identity, matching how UpsertNode preserves a node's GUID",
					SubobjectAfterReapply, OriginalSubobject);

				UBillboardComponent* Billboard = Cast<UBillboardComponent>(SubobjectAfterReapply);
				TestNotNull("Subobject should still cast to UBillboardComponent", Billboard);
				if (Billboard)
				{
					TestEqual("The re-applied field value should have taken effect on the preserved subobject", Billboard->ScreenSize, 9.0f);
				}
			});

			It("replaces the subobject when re-applying a different class", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileInstancedClassSwap");
				const FString NodeGuidText = TEXT("00000000-0000-0000-0000-0000000000F2");

				const FString FixtureText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_ExecuteComponent\","
					"\"properties\":{\"ComponentTemplate\":\"/Script/Engine.BillboardComponent(ScreenSize=1.000000)\"}}]}"
				), *NodeGuidText);

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, FixtureText, ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuid;
				FGuid::Parse(NodeGuidText, NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Node should exist", Node);
				if (!Node) { return; }

				FProperty* TemplateProperty = Node->GetClass()->FindPropertyByName(TEXT("ComponentTemplate"));
				FObjectProperty* ObjectProperty = CastField<FObjectProperty>(TemplateProperty);
				TestNotNull("ComponentTemplate should be an object property", ObjectProperty);
				if (!ObjectProperty) { return; }

				UObject* OriginalSubobject = ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(Node));
				TestNotNull("Subobject should exist after the fixture import", OriginalSubobject);
				TestEqual("Original subobject should be a BillboardComponent",
					OriginalSubobject ? OriginalSubobject->GetClass() : nullptr, UBillboardComponent::StaticClass());

				// UArrowComponent has no dependency on any content and, like UBillboardComponent, is
				// an ordinary non-abstract editinlinenew-eligible scene component - a real, different
				// shipped class, not a synthetic stand-in.
				const FString ReapplyText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Patch\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"properties\":{\"ComponentTemplate\":\"/Script/Engine.ArrowComponent(ArrowSize=2.000000)\"}}]}"
				), *NodeGuidText);

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, ReapplyText, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);

				UObject* SubobjectAfterSwap = ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(Node));
				TestNotNull("A subobject should still be present after the class swap", SubobjectAfterSwap);
				if (!SubobjectAfterSwap) { return; }

				TestNotEqual("Changing the requested class should replace the subobject rather than mutate the old one in place",
					SubobjectAfterSwap, OriginalSubobject);
				TestEqual("The new subobject should be the newly-requested class",
					SubobjectAfterSwap->GetClass(), UArrowComponent::StaticClass());

				UArrowComponent* Arrow = Cast<UArrowComponent>(SubobjectAfterSwap);
				if (Arrow)
				{
					TestEqual("The new subobject's field should reflect the new document", Arrow->ArrowSize, 2.0f);
				}
			});
		});

		Describe("bReplaceAddons", [this]()
		{
			auto MakeTwoAddOnFixture = [](const FString& NodeGuid, const FString& AddOnGuidD, const FString& AddOnGuidE) -> FString
			{
				return FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseD\"}},"
					"{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseE\"}}"
					"]}"
				), *NodeGuid, *AddOnGuidD, *NodeGuid, *AddOnGuidE, *NodeGuid);
			};

			It("removes an omitted addon when bReplaceAddons is true", [this, MakeTwoAddOnFixture]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileFullBlockDeletesOmittedAddOn");
				const FString GuidNodeA = TEXT("00000000-0000-0000-0000-0000000000A1");
				const FString GuidAddOnD = TEXT("00000000-0000-0000-0000-0000000000D1");
				const FString GuidAddOnE = TEXT("00000000-0000-0000-0000-0000000000E1");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeTwoAddOnFixture(GuidNodeA, GuidAddOnD, GuidAddOnE), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuidA, AddOnGuidD, AddOnGuidE;
				FGuid::Parse(GuidNodeA, NodeGuidA);
				FGuid::Parse(GuidAddOnD, AddOnGuidD);
				FGuid::Parse(GuidAddOnE, AddOnGuidE);

				UFlowNode* NodeA = TestFlowAsset->GetNode(NodeGuidA);
				TestNotNull("Node A should exist", NodeA);
				if (!NodeA) { return; }
				TestEqual("Node A should start with 2 addons", NodeA->GetFlowNodeAddOnChildren().Num(), 2);

				// bReplaceAddons true, and D is re-affirmed but E is omitted entirely (no
				// UpsertAddon, no DeleteAddon) - under bReplaceAddons that omission alone means
				// "delete E".
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_ReplaceAddonsTrueDeletesOmitted, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestTrue("Plan should report addon E as deleted (omitted under bReplaceAddons)", Result.Plan.AddOnsDeleted.ContainsByPredicate([AddOnGuidE](const FFlowReconcileAddonEntry& Entry) { return Entry.AddOnGuid == AddOnGuidE; }));

				const TArray<UFlowNodeAddOn*>& RemainingAddOns = NodeA->GetFlowNodeAddOnChildren();
				TestEqual("Node A should now have exactly 1 addon", RemainingAddOns.Num(), 1);
				bool bFoundD = false, bFoundE = false;
				for (UFlowNodeAddOn* AddOn : RemainingAddOns)
				{
					if (AddOn && AddOn->GetGuid() == AddOnGuidD) { bFoundD = true; }
					if (AddOn && AddOn->GetGuid() == AddOnGuidE) { bFoundE = true; }
				}
				TestTrue("AddOn D should still be present (re-affirmed under bReplaceAddons)", bFoundD);
				TestFalse("AddOn E should be gone (omitted under bReplaceAddons)", bFoundE);
			});

			It("does not delete an omitted addon when bReplaceAddons is false", [this, MakeTwoAddOnFixture]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileNonFullBlockPreservesOmittedAddOn");
				const FString GuidNodeA = TEXT("00000000-0000-0000-0000-0000000000A1");
				const FString GuidAddOnD = TEXT("00000000-0000-0000-0000-0000000000D1");
				const FString GuidAddOnE = TEXT("00000000-0000-0000-0000-0000000000E1");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeTwoAddOnFixture(GuidNodeA, GuidAddOnD, GuidAddOnE), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuidA, AddOnGuidE;
				FGuid::Parse(GuidNodeA, NodeGuidA);
				FGuid::Parse(GuidAddOnE, AddOnGuidE);

				UFlowNode* NodeA = TestFlowAsset->GetNode(NodeGuidA);
				TestNotNull("Node A should exist", NodeA);
				if (!NodeA) { return; }

				// Same shape as above (D re-affirmed, E omitted) but with bReplaceAddons false (the
				// default) - merge semantics mean E must survive untouched.
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_ReplaceAddonsFalsePreservesOmitted, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestTrue("Plan should report no deleted addons", Result.Plan.AddOnsDeleted.IsEmpty());
				TestEqual("Node A should still have 2 addons", NodeA->GetFlowNodeAddOnChildren().Num(), 2);

				bool bFoundE = false;
				for (UFlowNodeAddOn* AddOn : NodeA->GetFlowNodeAddOnChildren())
				{
					if (AddOn && AddOn->GetGuid() == AddOnGuidE) { bFoundE = true; }
				}
				TestTrue("AddOn E should still exist", bFoundE);
			});
		});

		Describe("Idempotency", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("applying the same node-add and connection-add mutation twice reports nothing further added on the second apply", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileIdempotent");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FString ReconcileError;
				FFlowReconcileResult FirstResult = ApplyMutationText(AssetPath, GReconcilerJson_AddNodeAndConnectionIdempotent, false, ReconcileError);
				TestTrue("First reconcile should succeed", FirstResult.bSuccess);
				TestEqual("First apply should report one added node", FirstResult.Plan.NodesAdded.Num(), 1);
				TestEqual("First apply should report one added connection", FirstResult.Plan.ConnectionsAdded.Num(), 1);

				FFlowReconcileResult SecondResult = ApplyMutationText(AssetPath, GReconcilerJson_AddNodeAndConnectionIdempotent, false, ReconcileError);
				TestTrue("Second reconcile should succeed", SecondResult.bSuccess);
				TestEqual("Second apply should report no added nodes - the node already exists", SecondResult.Plan.NodesAdded.Num(), 0);
				TestEqual("Second apply should report no added connections - it's already present", SecondResult.Plan.ConnectionsAdded.Num(), 0);
				TestEqual("Second apply should report no deleted nodes", SecondResult.Plan.NodesDeleted.Num(), 0);
				TestEqual("Asset should still have exactly 4 nodes after both applies", TestFlowAsset->GetNodes().Num(), 4);
			});
		});

		Describe("Partial Document Safety", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("does not delete a node omitted from a partial document", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileNoAccidentalDelete");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				// Document only mentions node B - A and C are omitted entirely, not DeleteNode marked.
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_PartialDocumentDoesNotDelete, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestEqual("Asset should still have all 3 nodes", TestFlowAsset->GetNodes().Num(), 3);
			});
		});

		Describe("Scope", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("deletes a node named in scope but absent from the document, while leaving an unscoped omitted node untouched", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileScopeDelete");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuidB;
				FGuid::Parse(GuidB, NodeGuidB);
				UFlowNode* OriginalB = TestFlowAsset->GetNode(NodeGuidB);
				TestNotNull("Node B should exist before the mutation", OriginalB);

				// scopeNodeGuids names only C. The document mentions neither B nor C via any op:
				// B is not in scope so must be left alone (the safe default); C is in scope, so
				// being absent here means "delete it", exactly as if a DeleteNode op had been written.
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_ScopedDeleteOfNodeC, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);

				FGuid NodeGuidC;
				FGuid::Parse(GuidC, NodeGuidC);
				TestEqual("Plan should report exactly one deleted node (C)", Result.Plan.NodesDeleted.Num(), 1);
				TestTrue("Plan should report C as deleted", Result.Plan.NodesDeleted.Contains(NodeGuidC));

				TestEqual("Asset should now have 2 nodes (A, B) - C deleted via scope", TestFlowAsset->GetNodes().Num(), 2);
				TestNull("Node C should no longer exist", TestFlowAsset->GetNode(NodeGuidC));
				TestEqual("Node B should be the same UObject instance - unscoped omission never deletes", TestFlowAsset->GetNode(NodeGuidB), OriginalB);
			});

			It("omits scopeNodeGuids entirely without deleting anything", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileScopeOmitted");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				// No scopeNodeGuids at all (empty means nothing is implicitly deleted) - same
				// document shape as the "partial document" test above, just re-asserted here as
				// the direct counterpart to the scoped-delete case.
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_PartialDocumentDoesNotDelete, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestTrue("Plan should report no deleted nodes", Result.Plan.NodesDeleted.IsEmpty());
				TestEqual("Asset should still have all 3 nodes", TestFlowAsset->GetNodes().Num(), 3);
			});
		});

		Describe("Full Mode Implicit Deletion", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("deletes a node and its connections when a Full-mode document omits them entirely", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileFullModeDelete");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid NodeGuidA, NodeGuidB, NodeGuidC;
				FGuid::Parse(GuidA, NodeGuidA);
				FGuid::Parse(GuidB, NodeGuidB);
				FGuid::Parse(GuidC, NodeGuidC);
				UFlowNode* OriginalA = TestFlowAsset->GetNode(NodeGuidA);
				UFlowNode* OriginalB = TestFlowAsset->GetNode(NodeGuidB);
				TestNotNull("Node A should exist before the mutation", OriginalA);
				TestNotNull("Node B should exist before the mutation", OriginalB);

				// Full-mode document re-describes only A, B, and the A->B connection. C and the
				// B->C connection are not mentioned at all - no scopeNodeGuids, no DeleteNode, no
				// RemoveConnection. Full mode's "the document is the entire graph" contract must
				// still delete both, unlike Patch mode where unscoped omission means untouched.
				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_FullModeOmitsNodeC, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);

				TestEqual("Plan should report exactly one deleted node (C)", Result.Plan.NodesDeleted.Num(), 1);
				TestTrue("Plan should report C as deleted", Result.Plan.NodesDeleted.Contains(NodeGuidC));
				TestFalse("Plan should report the B->C connection as removed", Result.Plan.ConnectionsRemoved.IsEmpty());

				TestEqual("Asset should now have 2 nodes (A, B) - C deleted implicitly by Full mode", TestFlowAsset->GetNodes().Num(), 2);
				TestNull("Node C should no longer exist", TestFlowAsset->GetNode(NodeGuidC));
				TestEqual("Node A should be the same UObject instance - re-declared, not recreated", TestFlowAsset->GetNode(NodeGuidA), OriginalA);
				TestEqual("Node B should be the same UObject instance - re-declared, not recreated", TestFlowAsset->GetNode(NodeGuidB), OriginalB);

				// The A->B connection was re-declared in the document, so it must survive.
				const FConnectedPin ConnectionFromA = OriginalA->GetConnection(TEXT("Out"));
				TestEqual("A's Out pin should still connect to B - the declared connection is not deleted", ConnectionFromA.NodeGuid, NodeGuidB);
			});
		});

		// Retargeting an exec output updates both runtime and editor connections, and the plan
		// reports the superseded connection under ConnectionsRemoved.
		Describe("Connection Retarget", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("retargeting an exec output pin actually changes the runtime connection, not just the plan", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileConnectionRetarget");

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				// A.Out currently points to B.In (per MakeThreeNodeText). Retarget it to a new
				// node D.In without any delete marker for the old A->B connection - exec output
				// pins are one-target-only at runtime, so this add alone must fully replace it.
				const FString GuidD = TEXT("00000000-0000-0000-0000-00000000000D");

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_RetargetExecOutputPin, false, ReconcileError);
				TestTrue("Apply should succeed", Result.bSuccess);

				// ConnectionLabel is built from FGuid::ToString(), which defaults to the
				// non-hyphenated Digits format - do not compare against the hyphenated literals
				// used in the mutation text.
				FGuid NodeGuidA;
				FGuid::Parse(GuidA, NodeGuidA);
				FGuid NodeGuidB;
				FGuid::Parse(GuidB, NodeGuidB);
				FGuid NodeGuidD;
				FGuid::Parse(GuidD, NodeGuidD);

				const FString AddedLabel = FString::Printf(TEXT("%s.Out -> %s.In"), *NodeGuidA.ToString(), *NodeGuidD.ToString());
				const FString RemovedLabel = FString::Printf(TEXT("%s.Out -> %s.In"), *NodeGuidA.ToString(), *NodeGuidB.ToString());
				TestTrue("Plan should report the new connection as added", Result.Plan.ConnectionsAdded.Contains(AddedLabel));
				TestTrue("Plan should honestly report the superseded connection as removed (never swallow)", Result.Plan.ConnectionsRemoved.Contains(RemovedLabel));
				UFlowNode* NodeA = TestFlowAsset->GetNode(NodeGuidA);
				if (TestNotNull("Node A should still exist", NodeA))
				{
					const FConnectedPin ActualConnection = NodeA->GetConnection(TEXT("Out"));
					TestEqual("A.Out must actually point at the new node D at runtime (not silently reverted to B)", ActualConnection.NodeGuid, NodeGuidD);
				}

				TestNotNull("Node B should still exist as a node - only its inbound connection from A was superseded, B itself was never mentioned for deletion", TestFlowAsset->GetNode(NodeGuidB));
			});

						It("verifies a retargeted connection after deleting the old endpoint", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileRetargetThenDelete");

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const FString GuidD = TEXT("00000000-0000-0000-0000-00000000000D");
				const FString MutationText = FString::Printf(TEXT(R"JSON(
{
	"formatVersion": 2,
	"mode": "Patch",
	"assetClass": "/Script/Flow.FlowAsset",
	"bWorldBound": true,
	"ops": [
		{ "kind": "UpsertNode", "guid": "%s", "type": "/Script/Flow.FlowNode_Finish",
		  "inputPins": [ { "name": "In", "type": "Exec" } ] },
		{ "kind": "AddConnection", "source": { "nodeGuid": "%s", "pin": "Out" },
		  "target": { "nodeGuid": "%s", "pin": "In" } },
		{ "kind": "DeleteNode", "guid": "%s" }
	]
}
)JSON"), *GuidD, *GuidA, *GuidD, *GuidB);

				FString ReconcileError;
				const FFlowReconcileResult Result = ApplyMutationText(AssetPath, MutationText, false, ReconcileError);
				TestTrue("Apply should succeed", Result.bSuccess);

				FGuid NodeGuidA;
				FGuid::Parse(GuidA, NodeGuidA);
				FGuid NodeGuidD;
				FGuid::Parse(GuidD, NodeGuidD);
				UFlowNode* NodeA = TestFlowAsset->GetNode(NodeGuidA);
				if (TestNotNull("Node A should still exist", NodeA))
				{
					const FConnectedPin Connection = NodeA->GetConnection(TEXT("Out"));
					TestEqual("A.Out should point to D after the old endpoint is deleted", Connection.NodeGuid, NodeGuidD);
				}
				FGuid NodeGuidB;
				FGuid::Parse(GuidB, NodeGuidB);
				TestNull("The old endpoint B should be deleted", TestFlowAsset->GetNode(NodeGuidB));
			});
			// A data connection is stored on its target node under the target pin name. Reapplying
			// the same connection must recognize that entry and remain idempotent.
			It("applying the same data-connection-add mutation twice reports nothing further added on the second apply", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileDataConnectionIdempotent");
				const FString GuidSource = TEXT("00000000-0000-0000-0000-0000000000E1");
				const FString GuidTarget = TEXT("00000000-0000-0000-0000-0000000000E2");

				// Establish the data connection during the initial *full* import (not via the
				// reconciler) - UFlowGraphImporter::SetupConnections calls EnsureTargetDataPinExists
				// while wiring, which is what actually materializes the "FormatText" input pin on
				// the target node's runtime InputPins array in the first place. Only after that can
				// the reconciler's own pin-existence validation see it as real.
				const FString CreateText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_FormatText\",\"outputPins\":[{\"name\":\"Formatted Text\",\"type\":\"Text\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_FormatText\",\"inputPins\":[{\"name\":\"FormatText\",\"type\":\"Text\"}],\"outputPins\":[{\"name\":\"Formatted Text\",\"type\":\"Text\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Formatted Text\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"FormatText\"}}"
					"]}"
				), *GuidSource, *GuidTarget, *GuidSource, *GuidTarget);

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, CreateText, ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				// Re-declare the *same* connection via the reconciler - this must be recognized as
				// already present (idempotent), not wrongly treated as new.
				FString FirstApplyError;
				FFlowReconcileResult FirstResult = ApplyMutationText(AssetPath, GReconcilerJson_ReaffirmDataConnection, false, FirstApplyError);
				if (!FirstResult.bSuccess)
				{
					AddError(FString::Printf(TEXT("First apply failed: %s"), *FirstApplyError));
				}
				TestTrue("First apply (re-declaring an already-wired data connection) should succeed", FirstResult.bSuccess);
				TestTrue("First apply's plan should already be empty - the data connection pre-exists from fixture creation (idempotency)", FirstResult.Plan.IsEmpty());

				FString SecondApplyError;
				FFlowReconcileResult SecondResult = ApplyMutationText(AssetPath, GReconcilerJson_ReaffirmDataConnection, false, SecondApplyError);
				TestTrue("Second apply should succeed", SecondResult.bSuccess);
				TestTrue("Second apply's plan should also be empty - still idempotent", SecondResult.Plan.IsEmpty());
			});
		});

		Describe("Dry Run", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("returns a non-empty plan without mutating the asset", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileDryRun");

				FString ErrorMessage;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ErrorMessage);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_DryRunUpdateNodeB, true, ReconcileError);
				TestTrue("Dry-run should report success", Result.bSuccess);
				TestFalse("Dry-run plan should not be empty", Result.Plan.IsEmpty());

				FGuid NodeGuidB;
				FGuid::Parse(GuidB, NodeGuidB);
				UFlowNode* NodeB = TestFlowAsset->GetNode(NodeGuidB);
				if (TestNotNull("Node B should still exist", NodeB))
				{
					FProperty* CompletionTimeProperty = NodeB->GetClass()->FindPropertyByName(TEXT("CompletionTime"));
					if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(CompletionTimeProperty))
					{
						TestEqual("Dry-run must not have mutated node B's CompletionTime", FloatProperty->GetPropertyValue(FloatProperty->ContainerPtrToValuePtr<void>(NodeB)), 5.0f);
					}
					else
					{
						AddError(TEXT("CompletionTime property should be found"));
					}
				}
			});
		});

		Describe("Metrics (P2)", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			// Acceptance criterion: nodes_touched and nodes_preserved are reported correctly
			// for update, add, delete, and mixed operations.
			It("reports nodes_touched=1, nodes_preserved=2 when updating exactly one node in a 3-node asset", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileMetricsUpdate");

				FString FixtureError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), FixtureError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FString ReconcileError;
				const FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_MetricsUpdateNodeB, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);

				// 1 update, 0 adds, 0 deletes, 3 existing -> touched=1, preserved=3-1=2
				TestEqual("nodes_touched should be 1 (one update)", Result.NodesTouched, 1);
				TestEqual("nodes_preserved should be 2 (two untouched nodes)", Result.NodesPreserved, 2);
			});

			It("reports nodes_touched=1, nodes_preserved=3 when adding one new node to a 3-node asset", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileMetricsAdd");

				FString FixtureError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), FixtureError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FString ReconcileError;
				const FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_MetricsAddNode, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);

				// 1 add, 0 updates, 0 deletes, 3 existing -> touched=1, preserved=3-0=3
				TestEqual("nodes_touched should be 1 (one add)", Result.NodesTouched, 1);
				TestEqual("nodes_preserved should be 3 (all originals untouched)", Result.NodesPreserved, 3);
			});

			It("reports nodes_touched=1, nodes_preserved=2 when deleting one node from a 3-node asset", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileMetricsDelete");

				FString FixtureError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), FixtureError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FString ReconcileError;
				const FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_MetricsDeleteNodeC, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);

				// 0 adds, 0 updates, 1 delete, 3 existing -> touched=1, preserved=3-0-1=2
				TestEqual("nodes_touched should be 1 (one delete)", Result.NodesTouched, 1);
				TestEqual("nodes_preserved should be 2 (two surviving nodes)", Result.NodesPreserved, 2);
			});

			It("reports InputSizeBytes matching the mutation text byte length", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileMetricsBytes");

				FString FixtureError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), FixtureError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const FString MutationDocument = GReconcilerJson_MetricsUpdateNodeB;

				FString ReconcileError;
				const FFlowReconcileResult Result = ApplyMutationText(AssetPath, MutationDocument, false, ReconcileError);
				TestTrue("Reconcile should succeed", Result.bSuccess);
				TestEqual("InputSizeBytes should equal the mutation document's byte length", Result.InputSizeBytes, MutationDocument.Len());
			});
		});

		Describe("Create Path", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
		{
			It("creates the asset when none exists using the AssetClass header", [this]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileCreatePath");

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(AssetPath, GReconcilerJson_CreateAssetOneNode, false, ReconcileError);
				TestTrue("Reconcile should succeed on the create path", Result.bSuccess);
				TestEqual("Plan should report one added node", Result.Plan.NodesAdded.Num(), 1);

				TestFlowAsset = FindObject<UFlowAsset>(nullptr, *(AssetPath + TEXT(".") + FPackageName::GetShortName(AssetPath)));
				TestNotNull("Asset should now exist and be loadable", TestFlowAsset);
			});

			// A Full document that creates an asset reports its requested connections after
			// verifying that they landed.
			It("reports added connections when creating a new asset with connections in one Full-mode patch", [this, MakeThreeNodeText, GuidA, GuidB, GuidC]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestReconcileCreatePathWithConnections");

				FString ReconcileError;
				FFlowReconcileResult Result = ApplyMutationText(
					AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), false, ReconcileError);
				TestTrue("Reconcile should succeed on the create path", Result.bSuccess);
				TestEqual("Plan should report three added nodes", Result.Plan.NodesAdded.Num(), 3);
				TestEqual("Plan should report both connections as added", Result.Plan.ConnectionsAdded.Num(), 2);
				TestEqual("Plan should report no findings", Result.Findings.Num(), 0);

				// The report must be verified against the asset, not merely echoed from the document:
				// a create that silently dropped a connection has to show up as a finding.
				UFlowAsset* CreatedAsset = FindObject<UFlowAsset>(nullptr, *(AssetPath + TEXT(".") + FPackageName::GetShortName(AssetPath)));
				TArray<FString> ParityIssues;
				UFlowGraphRegrapher::CollectGraphParityIssues(CreatedAsset, ParityIssues);
				TestEqual("A created asset's editor graph and runtime data should agree", ParityIssues.Num(), 0);

				TestFlowAsset = FindObject<UFlowAsset>(nullptr, *(AssetPath + TEXT(".") + FPackageName::GetShortName(AssetPath)));
				TestNotNull("Asset should now exist and be loadable", TestFlowAsset);
			});
		});

		// FindOrCreateNode returns the same node for repeated exact-match requests.
		Describe("FindOrCreateNode", [this, MakeThreeNodeText]()
		{
			It("finds an existing node whose properties match exactly, without creating anything", [this, MakeThreeNodeText]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestFindOrCreateNode_Match");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A1");
				const FString GuidB = TEXT("00000000-0000-0000-0000-0000000000B1");
				const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000C1");

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

				FGuid OutGuid;
				bool bOutCreated = true;
				FString ErrorMessage;
				const bool bResult = FFlowGraphReconciler::FindOrCreateNode(
					AssetPath, TEXT("FlowNode_Timer"), { { TEXT("CompletionTime"), TEXT("5.0") } },
					/*bDryRun=*/false, OutGuid, bOutCreated, ErrorMessage);

				FGuid ExpectedGuidB;
				FGuid::Parse(GuidB, ExpectedGuidB);

				TestTrue("FindOrCreateNode should succeed", bResult);
				TestFalse("Should report a match, not a creation", bOutCreated);
				TestEqual("Matched guid should be node B", OutGuid, ExpectedGuidB);
				TestEqual("No node should have been added on a match", TestFlowAsset->GetNodes().Num(), NodeCountBefore);
			});

			It("returns a hard error for a MatchProperties key the node type does not have", [this, MakeThreeNodeText]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestFindOrCreateNode_UnknownProp");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A2");
				const FString GuidB = TEXT("00000000-0000-0000-0000-0000000000B2");
				const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000C2");

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				FGuid OutGuid;
				bool bOutCreated = false;
				FString ErrorMessage;
				const bool bResult = FFlowGraphReconciler::FindOrCreateNode(
					AssetPath, TEXT("FlowNode_Timer"), { { TEXT("NotARealProperty"), TEXT("x") } },
					/*bDryRun=*/false, OutGuid, bOutCreated, ErrorMessage);

				TestFalse("FindOrCreateNode should fail on an unknown property name", bResult);
				TestFalse("Error message should not be empty", ErrorMessage.IsEmpty());
			});

			It("creates exactly one new node when no existing node matches", [this, MakeThreeNodeText]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestFindOrCreateNode_Create");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A3");
				const FString GuidB = TEXT("00000000-0000-0000-0000-0000000000B3");
				const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000C3");

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

				FGuid OutGuid;
				bool bOutCreated = false;
				FString ErrorMessage;
				const bool bResult = FFlowGraphReconciler::FindOrCreateNode(
					AssetPath, TEXT("FlowNode_Timer"), { { TEXT("CompletionTime"), TEXT("99.0") } },
					/*bDryRun=*/false, OutGuid, bOutCreated, ErrorMessage);

				TestTrue("FindOrCreateNode should succeed", bResult);
				TestTrue("Should report a creation, not a match", bOutCreated);
				TestTrue("A valid guid should be minted for the new node", OutGuid.IsValid());
				TestEqual("Exactly one node should have been added", TestFlowAsset->GetNodes().Num(), NodeCountBefore + 1);

				UFlowNode* NewNode = TestFlowAsset->GetNode(OutGuid);
				TestNotNull("New node should exist on the asset", NewNode);
				if (NewNode)
				{
					TestEqual("New node should be a FlowNode_Timer", NewNode->GetClass()->GetName(), FString(TEXT("FlowNode_Timer")));
				}
			});

			It("dry run reports a would-be creation without mutating the asset", [this, MakeThreeNodeText]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestFindOrCreateNode_DryRun");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A4");
				const FString GuidB = TEXT("00000000-0000-0000-0000-0000000000B4");
				const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000C4");

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

				FGuid OutGuid;
				bool bOutCreated = false;
				FString ErrorMessage;
				const bool bResult = FFlowGraphReconciler::FindOrCreateNode(
					AssetPath, TEXT("FlowNode_Timer"), { { TEXT("CompletionTime"), TEXT("42.5") } },
					/*bDryRun=*/true, OutGuid, bOutCreated, ErrorMessage);

				TestTrue("Dry-run FindOrCreateNode should succeed", bResult);
				TestTrue("Dry run should still report it would create", bOutCreated);
				TestEqual("Dry run must not mutate the asset's node count", TestFlowAsset->GetNodes().Num(), NodeCountBefore);
			});

			It("breaks ties between multiple matches deterministically by lowest GUID", [this, MakeThreeNodeText]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestFindOrCreateNode_Tiebreak");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A5");
				const FString GuidB = TEXT("00000000-0000-0000-0000-0000000000B5");
				const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000C5");

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				// Explicit, deliberately out-of-lexical-order GUIDs so the tiebreak can't
				// accidentally pass by matching insertion order instead of string comparison.
				const FString HigherGuid = TEXT("FFFFFFFF-0000-0000-0000-000000000001");
				const FString LowerGuid  = TEXT("00000000-0000-0000-0000-000000000002");

				FString ReconcileError;
				const FFlowReconcileResult SetupResult = ApplyMutationText(AssetPath, GReconcilerJson_AddTwoMatchingTimerNodes, false, ReconcileError);
				TestTrue("Fixture setup patch should succeed", SetupResult.bSuccess);

				FGuid OutGuid;
				bool bOutCreated = true;
				FString ErrorMessage;
				const bool bResult = FFlowGraphReconciler::FindOrCreateNode(
					AssetPath, TEXT("FlowNode_Timer"), { { TEXT("CompletionTime"), TEXT("77.0") }, { TEXT("StepTime"), TEXT("2.0") } },
					/*bDryRun=*/false, OutGuid, bOutCreated, ErrorMessage);

				FGuid ExpectedLowerGuid;
				FGuid::Parse(LowerGuid, ExpectedLowerGuid);

				TestTrue("FindOrCreateNode should succeed", bResult);
				TestFalse("Should report a match, not a creation", bOutCreated);
				TestEqual("Should deterministically return the lexicographically lowest guid", OutGuid, ExpectedLowerGuid);
			});

			It("is idempotent: calling it twice with identical args mints exactly one node and returns the same GUID both times", [this, MakeThreeNodeText]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestFindOrCreateNode_CallTwice");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A6");
				const FString GuidB = TEXT("00000000-0000-0000-0000-0000000000B6");
				const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000C6");

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();
				const TMap<FString, FString> MatchProperties = { { TEXT("CompletionTime"), TEXT("123.0") } };

				FGuid FirstGuid;
				bool bFirstCreated = false;
				FString FirstError;
				const bool bFirstResult = FFlowGraphReconciler::FindOrCreateNode(
					AssetPath, TEXT("FlowNode_Timer"), MatchProperties, /*bDryRun=*/false, FirstGuid, bFirstCreated, FirstError);

				TestTrue("First call should succeed", bFirstResult);
				TestTrue("First call should report a creation (no prior match)", bFirstCreated);
				TestEqual("Exactly one node should exist after the first call", TestFlowAsset->GetNodes().Num(), NodeCountBefore + 1);

				FGuid SecondGuid;
				bool bSecondCreated = true;
				FString SecondError;
				const bool bSecondResult = FFlowGraphReconciler::FindOrCreateNode(
					AssetPath, TEXT("FlowNode_Timer"), MatchProperties, /*bDryRun=*/false, SecondGuid, bSecondCreated, SecondError);

				TestTrue("Second call should succeed", bSecondResult);
				TestFalse("Second call should report a match, not another creation", bSecondCreated);
				TestEqual("Second call should return the same GUID the first call minted", SecondGuid, FirstGuid);
				TestEqual("No further node should have been added on the second call", TestFlowAsset->GetNodes().Num(), NodeCountBefore + 1);
			});

			It("is idempotent across non-canonical value formatting: '123' matches a node created from '123.0'", [this, MakeThreeNodeText]()
			{
				const FString AssetPath = TEXT("/Game/Test/TestFindOrCreateNode_NonCanonicalValue");
				const FString GuidA = TEXT("00000000-0000-0000-0000-0000000000A7");
				const FString GuidB = TEXT("00000000-0000-0000-0000-0000000000B7");
				const FString GuidC = TEXT("00000000-0000-0000-0000-0000000000C7");

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, MakeThreeNodeText(GuidA, GuidB, GuidC, TEXT("5.0")), ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

				FGuid FirstGuid;
				bool bFirstCreated = false;
				FString FirstError;
				const bool bFirstResult = FFlowGraphReconciler::FindOrCreateNode(
					AssetPath, TEXT("FlowNode_Timer"), { { TEXT("CompletionTime"), TEXT("123.0") } },
					/*bDryRun=*/false, FirstGuid, bFirstCreated, FirstError);

				TestTrue("First call (canonical '123.0') should succeed", bFirstResult);
				TestTrue("First call should report a creation", bFirstCreated);
				TestEqual("Exactly one node should exist after the first call", TestFlowAsset->GetNodes().Num(), NodeCountBefore + 1);

				// Second call passes the same value in a non-canonical form. Without normalizing
				// both sides through the same parse/stringify path the create path uses, this
				// would fail to match the node the first call created and mint a duplicate.
				FGuid SecondGuid;
				bool bSecondCreated = true;
				FString SecondError;
				const bool bSecondResult = FFlowGraphReconciler::FindOrCreateNode(
					AssetPath, TEXT("FlowNode_Timer"), { { TEXT("CompletionTime"), TEXT("123") } },
					/*bDryRun=*/false, SecondGuid, bSecondCreated, SecondError);

				TestTrue("Second call (non-canonical '123') should succeed", bSecondResult);
				TestFalse("Second call should report a match, not another creation", bSecondCreated);
				TestEqual("Second call should return the same GUID the first call minted", SecondGuid, FirstGuid);
				TestEqual("No duplicate node should have been created", TestFlowAsset->GetNodes().Num(), NodeCountBefore + 1);
			});

			It("returns an error when a MatchProperties value cannot be parsed for its property's type", [this]()
			{
				// FlowNode_Timer's properties are all numeric (FCString::Atof/Atoi never fail to
				// parse, they just fall back to 0). Use FlowNode_DefineProperties' NamedProperties
				// (TArray<FFlowNamedDataPinProperty>) instead - already proven elsewhere in this
				// test suite (see FlowCourierFormat.spec.cpp) to make FArrayProperty::ImportText_Direct
				// fail on genuinely malformed text.
				const FString AssetPath = TEXT("/Game/Test/TestFindOrCreateNode_UnparsableValue");
				const FString SetupText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000009\",\"type\":\"/Script/Flow.FlowNode_DefineProperties\"}]}"
				);

				FString ImportError;
				TestFlowAsset = ImportFixtureDocument(AssetPath, SetupText, ImportError);
				TestNotNull("Fixture import should succeed", TestFlowAsset);
				if (!TestFlowAsset) { return; }

				const int32 NodeCountBefore = TestFlowAsset->GetNodes().Num();

				FGuid OutGuid;
				bool bOutCreated = false;
				FString ErrorMessage;
				const bool bResult = FFlowGraphReconciler::FindOrCreateNode(
					AssetPath, TEXT("FlowNode_DefineProperties"),
					{ { TEXT("NamedProperties"), TEXT("((Name=\"TestProp\",DataPinValue=/Script/Flow.FlowDataPinValue_Text(Values=(\"unterminated") } },
					/*bDryRun=*/false, OutGuid, bOutCreated, ErrorMessage);

				TestFalse("FindOrCreateNode should fail on an unparsable MatchProperties value", bResult);
				TestFalse("Error message should not be empty", ErrorMessage.IsEmpty());
				TestEqual("Nothing should have been created on a normalization failure", TestFlowAsset->GetNodes().Num(), NodeCountBefore);
			});

			It("returns an error when no asset exists at TargetAssetPath", [this]()
			{
				FGuid OutGuid;
				bool bOutCreated = false;
				FString ErrorMessage;
				const bool bResult = FFlowGraphReconciler::FindOrCreateNode(
					TEXT("/Game/Test/TestFindOrCreateNode_NoSuchAsset"), TEXT("FlowNode_Timer"), { { TEXT("CompletionTime"), TEXT("1.0") } },
					/*bDryRun=*/false, OutGuid, bOutCreated, ErrorMessage);

				TestFalse("FindOrCreateNode should fail when the target asset does not exist", bResult);
				TestFalse("Error message should not be empty", ErrorMessage.IsEmpty());
			});
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
