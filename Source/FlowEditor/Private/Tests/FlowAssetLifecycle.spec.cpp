// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FlowAsset.h"
#include "FlowAssetLifecycleTestTypes.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"
#include "Nodes/Developer/FlowNode_Log.h"
#include "Policies/FlowPreloadPolicy.h"
#include "Policies/FlowStandardPreloadPolicies.h"
#include "UObject/UnrealType.h"

namespace
{
	TInstancedStruct<FFlowPreloadPolicy>& GetMutablePreloadPolicy(UFlowAsset& Asset)
	{
		const FStructProperty* Property = FindFProperty<FStructProperty>(UFlowAsset::StaticClass(), TEXT("PreloadPolicy"));
		check(Property);
		return *Property->ContainerPtrToValuePtr<TInstancedStruct<FFlowPreloadPolicy>>(&Asset);
	}

	void SetSignalMode(UFlowNode& Node, const EFlowSignalMode SignalMode)
	{
		const FEnumProperty* Property = FindFProperty<FEnumProperty>(UFlowNode::StaticClass(), TEXT("SignalMode"));
		check(Property);
		Property->GetUnderlyingProperty()->SetIntPropertyValue(
			Property->ContainerPtrToValuePtr<void>(&Node), static_cast<int64>(SignalMode));
	}
}

BEGIN_DEFINE_SPEC(FFlowAssetLifecycleSpec, "FlowEditor.AssetLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FFlowAssetLifecycleSpec)

void FFlowAssetLifecycleSpec::Define()
{
	Describe("Preload policy initialization", [this]()
	{
		It("initializes the asset policy when a preloadable node is initialized before the asset", [this]()
		{
			UFlowAsset* Asset = NewObject<UFlowAsset>();
			UFlowNode_SubGraph* Node = NewObject<UFlowNode_SubGraph>(Asset);
			TestFalse("The asset policy starts uninitialized", GetMutablePreloadPolicy(*Asset).IsValid());

			Node->InitializeInstance();

			TestTrue("The preloadable node initializes the asset policy before reading it", GetMutablePreloadPolicy(*Asset).IsValid());
		});

		It("preserves an already initialized customized policy", [this]()
		{
			UFlowAsset* Asset = NewObject<UFlowAsset>();
			TInstancedStruct<FFlowPreloadPolicy>& Policy = GetMutablePreloadPolicy(*Asset);
			Policy.InitializeAs<FFlowPreloadPolicy_Standard>();
			Policy.GetMutable<FFlowPreloadPolicy_Standard>().DefaultPreloadTiming = EFlowPreloadTiming::ManualOnly;
			Policy.GetMutable<FFlowPreloadPolicy_Standard>().DefaultFlushTiming = EFlowFlushTiming::OnNodeFinish;

			Asset->EnsurePreloadPolicyInitialized();

			const FFlowPreloadPolicy_Standard* StandardPolicy = Policy.GetPtr<FFlowPreloadPolicy_Standard>();
			TestNotNull("The customized standard policy remains valid", StandardPolicy);
			if (StandardPolicy)
			{
				TestEqual("The customized preload timing is preserved", StandardPolicy->DefaultPreloadTiming, EFlowPreloadTiming::ManualOnly);
				TestEqual("The customized flush timing is preserved", StandardPolicy->DefaultFlushTiming, EFlowFlushTiming::OnNodeFinish);
			}
		});
	});

	Describe("Node deactivation", [this]()
	{
		It("completes an orphaned node safely", [this]()
		{
			UFlowNode_LogTestable* Orphan = NewObject<UFlowNode_LogTestable>();
			AddExpectedError("Ensure condition failed", EAutomationExpectedErrorFlags::Contains, -1);
			AddExpectedError("Flow Node is orphaned and could not retrieve its Flow Asset", EAutomationExpectedErrorFlags::Contains, -1);
			AddExpectedError("Handled ensure", EAutomationExpectedErrorFlags::Contains, -1);
			AddExpectedError("Stack:", EAutomationExpectedErrorFlags::Contains, -1);
			AddExpectedErrorPlain("Script Stack (0 frames)", EAutomationExpectedErrorFlags::Contains, -1);
			AddExpectedError("\\[Callstack\\]", EAutomationExpectedErrorFlags::Contains, -1);
			AddExpectedError("^$", EAutomationExpectedErrorFlags::Contains, -1);

			Orphan->DeactivateForTest();

			TestEqual("The orphaned node reaches Completed", Orphan->GetActivationState(), EFlowNodeState::Completed);
			TestEqual("The orphaned node is cleaned up", Orphan->CleanupCount, 1);
		});

		It("preserves abort and pass-through behavior", [this]()
		{
			UFlowAsset* Asset = NewObject<UFlowAsset>();
			Asset->FinishFlowInstance(EFlowFinishPolicy::Abort);
			UFlowNode_LogTestable* AbortNode = NewObject<UFlowNode_LogTestable>(Asset);
			AbortNode->DeactivateForTest();
			TestEqual("Abort policy still sets Aborted", AbortNode->GetActivationState(), EFlowNodeState::Aborted);
			TestEqual("The aborted node is cleaned up", AbortNode->CleanupCount, 1);

			UFlowNode_LogTestable* PassThroughOrphan = NewObject<UFlowNode_LogTestable>();
			SetSignalMode(*PassThroughOrphan, EFlowSignalMode::PassThrough);
			PassThroughOrphan->DeactivateForTest();
			TestEqual("PassThrough still does not deactivate the node", PassThroughOrphan->GetActivationState(), EFlowNodeState::NeverActivated);
			TestEqual("PassThrough still skips cleanup", PassThroughOrphan->CleanupCount, 0);
		});
	});
}

#endif
