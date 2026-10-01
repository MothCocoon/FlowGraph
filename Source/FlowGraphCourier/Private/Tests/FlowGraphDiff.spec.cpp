// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FlowGraphDiff.h"

BEGIN_DEFINE_SPEC(FFlowGraphDiffSpec, "FlowGraphCourier.EditorGame.Diff", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	// Builds a one-node Courier v2 JSON document. AddOnsJsonOps is inserted verbatim as extra ops
	// after the node's own UpsertNode op (empty for no addons; each addon op must start with a
	// leading comma), so a test can vary only the addon subtree between two documents and keep the
	// node identity (GUID) stable - making the node a "changed node" in the diff rather than an add/remove.
	static FString MakeSingleNodeDoc(const FString& NodeGuid, const FString& AddOnsJsonOps)
	{
		return FString::Printf(TEXT(
			"{"
				"\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
				"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}"
					"%s"
				"]"
			"}"
		), *NodeGuid, *AddOnsJsonOps);
	}

	// Two-node document with both an exec and a data connection.
	// ConnectionsJsonOps is inserted verbatim as extra ops after both nodes' UpsertNode ops (empty
	// for no connections; each connection op must start with a leading comma).
	static FString MakeTwoNodeDocWithConnections(const FString& NodeAGuid, const FString& NodeBGuid, const FString& ConnectionsJsonOps)
	{
		return FString::Printf(TEXT(
			"{"
				"\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
				"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Start\",\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"},{\"name\":\"Score\",\"type\":\"Float\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"type\":\"/Script/Flow.FlowNode_Finish\",\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"},{\"name\":\"Score\",\"type\":\"Float\"}]}"
					"%s"
				"]"
			"}"
		), *NodeAGuid, *NodeBGuid, *ConnectionsJsonOps);
	}

END_DEFINE_SPEC(FFlowGraphDiffSpec)

void FFlowGraphDiffSpec::Define()
{
	const FString NodeGuid = TEXT("00000000-0000-0000-0000-0000000000A0");
	const FString AddOnGuid = TEXT("00000000-0000-0000-0000-0000000000D0");

	It("reports a node class change at the same GUID", [this, NodeGuid]()
	{
		const FString OldText = MakeSingleNodeDoc(NodeGuid, TEXT(""));
		const FString NewText = OldText.Replace(TEXT("/Script/Flow.FlowNode_Start"), TEXT("/Script/Flow.FlowNode_Timer"));
		FFlowGraphDiffResult Diff;
		FString Error;
		TestTrue("Class-only documents should diff", UFlowGraphDiff::ComputeDiff(OldText, NewText, Diff, Error));
		TestTrue("Class change is a difference", Diff.HasAnyDifference());
		TestTrue("No nodes added or removed", Diff.AddedNodeGuids.IsEmpty() && Diff.RemovedNodeGuids.IsEmpty());
		TestEqual("Exactly one node changed", Diff.ChangedNodes.Num(), 1);
		if (Diff.ChangedNodes.Num() != 1)
		{
			return;
		}
		const FString* ClassDelta = Diff.ChangedNodes[0].ChangedProperties.Find(TEXT("$class"));
		TestNotNull("Changed node reports its old and new classes", ClassDelta);
		if (ClassDelta)
		{
			TestTrue("Both class paths are present", ClassDelta->Contains(TEXT("FlowNode_Start -> /Script/Flow.FlowNode_Timer")));
		}
	});

	Describe("AddOn diff", [this, NodeGuid, AddOnGuid]()
	{
		It("reports an addon added to an existing node", [this, NodeGuid, AddOnGuid]()
		{
			const FString OldText = MakeSingleNodeDoc(NodeGuid, TEXT(""));
			const FString AddOnOps = FString::Printf(TEXT(
				",{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseD\"}}"
			), *AddOnGuid, *NodeGuid);
			const FString NewText = MakeSingleNodeDoc(NodeGuid, AddOnOps);

			FFlowGraphDiffResult Diff;
			FString Error;
			TestTrue("ComputeDiff should succeed", UFlowGraphDiff::ComputeDiff(OldText, NewText, Diff, Error));
			TestTrue("Diff should report a difference", Diff.HasAnyDifference());
			TestEqual("Exactly one node should be reported changed", Diff.ChangedNodes.Num(), 1);
			if (Diff.ChangedNodes.Num() != 1) { return; }

			const FFlowGraphDiffChangedNode& ChangedNode = Diff.ChangedNodes[0];
			TestEqual("The changed node should have one added addon", ChangedNode.AddedAddOns.Num(), 1);
			TestTrue("No removed/changed addons expected", ChangedNode.RemovedAddOns.IsEmpty() && ChangedNode.ChangedAddOns.IsEmpty());
			if (ChangedNode.AddedAddOns.Num() != 1) { return; }

			FGuid ExpectedAddOnGuid;
			FGuid::Parse(AddOnGuid, ExpectedAddOnGuid);
			const FFlowGraphDiffAddOn& Added = ChangedNode.AddedAddOns[0];
			TestEqual("Added addon guid should match", Added.AddOnGuid, ExpectedAddOnGuid);
			TestTrue("Added addon type should be SwitchCase", Added.AddOnType.Contains(TEXT("FlowNodeAddOn_SwitchCase")));
			const FString* CaseNameDelta = Added.ChangedProperties.Find(TEXT("CaseName"));
			TestNotNull("Added addon should report CaseName as a property delta", CaseNameDelta);
			if (CaseNameDelta) { TestTrue("CaseName delta should read from <absent>", CaseNameDelta->Contains(TEXT("<absent> -> CaseD"))); }
		});

		It("reports an addon removed from an existing node", [this, NodeGuid, AddOnGuid]()
		{
			const FString AddOnOps = FString::Printf(TEXT(
				",{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseD\"}}"
			), *AddOnGuid, *NodeGuid);
			const FString OldText = MakeSingleNodeDoc(NodeGuid, AddOnOps);
			const FString NewText = MakeSingleNodeDoc(NodeGuid, TEXT(""));

			FFlowGraphDiffResult Diff;
			FString Error;
			TestTrue("ComputeDiff should succeed", UFlowGraphDiff::ComputeDiff(OldText, NewText, Diff, Error));
			TestTrue("Diff should report a difference", Diff.HasAnyDifference());
			TestEqual("Exactly one node should be reported changed", Diff.ChangedNodes.Num(), 1);
			if (Diff.ChangedNodes.Num() != 1) { return; }

			const FFlowGraphDiffChangedNode& ChangedNode = Diff.ChangedNodes[0];
			TestEqual("The changed node should have one removed addon", ChangedNode.RemovedAddOns.Num(), 1);
			TestTrue("No added/changed addons expected", ChangedNode.AddedAddOns.IsEmpty() && ChangedNode.ChangedAddOns.IsEmpty());
		});

		It("reports an addon property change", [this, NodeGuid, AddOnGuid]()
		{
			const FString OldText = MakeSingleNodeDoc(NodeGuid, FString::Printf(TEXT(
				",{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseD\"}}"
			), *AddOnGuid, *NodeGuid));
			const FString NewText = MakeSingleNodeDoc(NodeGuid, FString::Printf(TEXT(
				",{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseDRenamed\"}}"
			), *AddOnGuid, *NodeGuid));

			FFlowGraphDiffResult Diff;
			FString Error;
			TestTrue("ComputeDiff should succeed", UFlowGraphDiff::ComputeDiff(OldText, NewText, Diff, Error));
			TestTrue("Diff should report a difference", Diff.HasAnyDifference());
			TestEqual("Exactly one node should be reported changed", Diff.ChangedNodes.Num(), 1);
			if (Diff.ChangedNodes.Num() != 1) { return; }

			const FFlowGraphDiffChangedNode& ChangedNode = Diff.ChangedNodes[0];
			TestEqual("The changed node should have one changed addon", ChangedNode.ChangedAddOns.Num(), 1);
			TestTrue("No added/removed addons expected", ChangedNode.AddedAddOns.IsEmpty() && ChangedNode.RemovedAddOns.IsEmpty());
			if (ChangedNode.ChangedAddOns.Num() != 1) { return; }

			const FFlowGraphDiffAddOn& Changed = ChangedNode.ChangedAddOns[0];
			const FString* CaseNameDelta = Changed.ChangedProperties.Find(TEXT("CaseName"));
			TestNotNull("Changed addon should report the CaseName delta", CaseNameDelta);
			if (CaseNameDelta) { TestTrue("CaseName delta should read old -> new", CaseNameDelta->Contains(TEXT("CaseD -> CaseDRenamed"))); }
		});

		It("reports no change for identical addon trees", [this, NodeGuid, AddOnGuid]()
		{
			const FString AddOnOps = FString::Printf(TEXT(
				",{\"kind\":\"UpsertAddon\",\"guid\":\"%s\",\"parentGuid\":\"%s\",\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"CaseD\"}}"
			), *AddOnGuid, *NodeGuid);
			const FString Doc = MakeSingleNodeDoc(NodeGuid, AddOnOps);

			FFlowGraphDiffResult Diff;
			FString Error;
			TestTrue("ComputeDiff should succeed", UFlowGraphDiff::ComputeDiff(Doc, Doc, Diff, Error));
			TestFalse("Identical documents should report no difference", Diff.HasAnyDifference());
			TestTrue("No changed nodes expected", Diff.ChangedNodes.IsEmpty());
		});
	});

	// Connection diffs distinguish exec and data pins.
	Describe("Connection exec/data tagging", [this]()
	{
		const FString NodeA = TEXT("00000000-0000-0000-0000-0000000000B1");
		const FString NodeB = TEXT("00000000-0000-0000-0000-0000000000B2");

		It("added exec connection is tagged bIsExecPin=true", [this, NodeA, NodeB]()
		{
			const FString OldDoc = MakeTwoNodeDocWithConnections(NodeA, NodeB, TEXT(""));
			const FString NewDoc = MakeTwoNodeDocWithConnections(NodeA, NodeB,
				FString::Printf(TEXT(",{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Out\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}"), *NodeA, *NodeB));

			FFlowGraphDiffResult Diff;
			FString Error;
			TestTrue("ComputeDiff should succeed", UFlowGraphDiff::ComputeDiff(OldDoc, NewDoc, Diff, Error));
			TestEqual("Exactly one added connection", Diff.AddedConnections.Num(), 1);
			if (Diff.AddedConnections.Num() != 1) { return; }
			TestTrue("Exec connection should be tagged bIsExecPin=true", Diff.AddedConnections[0].bIsExecPin);
		});

		It("added data connection is tagged bIsExecPin=false", [this, NodeA, NodeB]()
		{
			const FString OldDoc = MakeTwoNodeDocWithConnections(NodeA, NodeB, TEXT(""));
			const FString NewDoc = MakeTwoNodeDocWithConnections(NodeA, NodeB,
				FString::Printf(TEXT(",{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Score\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"Score\"}}"), *NodeA, *NodeB));

			FFlowGraphDiffResult Diff;
			FString Error;
			TestTrue("ComputeDiff should succeed", UFlowGraphDiff::ComputeDiff(OldDoc, NewDoc, Diff, Error));
			TestEqual("Exactly one added connection", Diff.AddedConnections.Num(), 1);
			if (Diff.AddedConnections.Num() != 1) { return; }
			TestFalse("Data connection should be tagged bIsExecPin=false", Diff.AddedConnections[0].bIsExecPin);
		});

		It("removed exec connection is tagged bIsExecPin=true", [this, NodeA, NodeB]()
		{
			const FString OldDoc = MakeTwoNodeDocWithConnections(NodeA, NodeB,
				FString::Printf(TEXT(",{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Out\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"In\"}}"), *NodeA, *NodeB));
			const FString NewDoc = MakeTwoNodeDocWithConnections(NodeA, NodeB, TEXT(""));

			FFlowGraphDiffResult Diff;
			FString Error;
			TestTrue("ComputeDiff should succeed", UFlowGraphDiff::ComputeDiff(OldDoc, NewDoc, Diff, Error));
			TestEqual("Exactly one removed connection", Diff.RemovedConnections.Num(), 1);
			if (Diff.RemovedConnections.Num() != 1) { return; }
			TestTrue("Removed exec connection should be tagged bIsExecPin=true", Diff.RemovedConnections[0].bIsExecPin);
		});

		It("removed data connection is tagged bIsExecPin=false", [this, NodeA, NodeB]()
		{
			const FString OldDoc = MakeTwoNodeDocWithConnections(NodeA, NodeB,
				FString::Printf(TEXT(",{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"%s\",\"pin\":\"Score\"},\"target\":{\"nodeGuid\":\"%s\",\"pin\":\"Score\"}}"), *NodeA, *NodeB));
			const FString NewDoc = MakeTwoNodeDocWithConnections(NodeA, NodeB, TEXT(""));

			FFlowGraphDiffResult Diff;
			FString Error;
			TestTrue("ComputeDiff should succeed", UFlowGraphDiff::ComputeDiff(OldDoc, NewDoc, Diff, Error));
			TestEqual("Exactly one removed connection", Diff.RemovedConnections.Num(), 1);
			if (Diff.RemovedConnections.Num() != 1) { return; }
			TestFalse("Removed data connection should be tagged bIsExecPin=false", Diff.RemovedConnections[0].bIsExecPin);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
