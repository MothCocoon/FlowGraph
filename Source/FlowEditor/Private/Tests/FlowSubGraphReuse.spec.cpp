// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FlowAsset.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"
#include "GameFramework/Actor.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"
#include "Policies/FlowPreloadHelper.h"
#include "Policies/FlowPreloadPolicyEnums.h"
#include "Policies/FlowStandardPreloadPolicies.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "Types/FlowInjectComponentsManager.h"
#include "Types/FlowOutputDataPinValues.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"

#include "FlowSubGraphReuseTestTypes.h"

namespace FlowSubGraphReuseTests
{
	void AddNodeToTemplate(UFlowAsset& Template, const FGuid& NodeGuid, UFlowNode* Node)
	{
		const FMapProperty* NodesProperty = FindFProperty<FMapProperty>(UFlowAsset::StaticClass(), TEXT("Nodes"));
		TMap<FGuid, TObjectPtr<UFlowNode>>& Nodes =
			*NodesProperty->ContainerPtrToValuePtr<TMap<FGuid, TObjectPtr<UFlowNode>>>(&Template);
		Nodes.Add(NodeGuid, Node);
	}
}

struct FFlowSubGraphReuseGameInstanceAccess : UGameInstance
{
	static void SetWorldContext(UGameInstance& Instance, FWorldContext* Context)
	{
		FWorldContext* UGameInstance::* ContextMember = &FFlowSubGraphReuseGameInstanceAccess::WorldContext;
		Instance.*ContextMember = Context;
	}
};

BEGIN_DEFINE_SPEC(FFlowSubGraphReuseSpec, "FlowEditor.SubGraphReuse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TStrongObjectPtr<UGameInstance> GameInstance;
	TStrongObjectPtr<UFlowSubsystem> Subsystem;
	TStrongObjectPtr<UFlowAsset> ParentTemplate;
	TStrongObjectPtr<UFlowAsset> ChildTemplate;
	TStrongObjectPtr<UFlowAsset> RootInstance;
	UWorld* World = nullptr;
	AActor* OwnerActor = nullptr;
	ESubGraphFinishPolicy PreviousFinishPolicy = ESubGraphFinishPolicy::Keep;
	FInstancedStruct PreviousPreloadPolicy;
	FNativeFlowAssetEvent PreviousAdded;
	FNativeFlowAssetEvent PreviousRemoved;

	UFlowAsset* CreateChildTemplate(UClass* AssetClass) const
	{
		UFlowAsset* Template = NewObject<UFlowAsset>(GetTransientPackage(), AssetClass);
		const FGuid ChildStartGuid = FGuid::NewGuid();
		const FGuid ChildFinishGuid = FGuid::NewGuid();
		UFlowSubGraphReuseTestStartNode* ChildStart = NewObject<UFlowSubGraphReuseTestStartNode>(Template);
		UFlowSubGraphReuseTestFinishNode* ChildFinish = NewObject<UFlowSubGraphReuseTestFinishNode>(Template);
		FlowSubGraphReuseTests::AddNodeToTemplate(*Template, ChildStartGuid, ChildStart);
		FlowSubGraphReuseTests::AddNodeToTemplate(*Template, ChildFinishGuid, ChildFinish);

		TMap<FName, FConnectedPin> ChildStartConnections;
		ChildStartConnections.Add(UFlowNode::DefaultOutputPin.PinName,
			FConnectedPin(ChildFinishGuid, UFlowNode::DefaultInputPin.PinName));
		ChildStart->SetConnections(ChildStartConnections);
		return Template;
	}

	UFlowNode_SubGraph* FindSubGraphNode() const
	{
		if (!RootInstance.IsValid())
		{
			return nullptr;
		}

		for (const TPair<FGuid, UFlowNode*>& Node : RootInstance->GetNodes())
		{
			if (UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(Node.Value))
			{
				return SubGraphNode;
			}
		}

		return nullptr;
	}

	UFlowSubGraphReuseTestStartNode* FindStartNode(UFlowAsset* FlowAsset) const
	{
		if (!IsValid(FlowAsset))
		{
			return nullptr;
		}

		for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
		{
			if (UFlowSubGraphReuseTestStartNode* StartNode = Cast<UFlowSubGraphReuseTestStartNode>(Node.Value))
			{
				return StartNode;
			}
		}

		return nullptr;
	}

	UFlowSubGraphReuseTestFinishNode* FindFinishNode(UFlowAsset* FlowAsset) const
	{
		if (!IsValid(FlowAsset))
		{
			return nullptr;
		}

		for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
		{
			if (UFlowSubGraphReuseTestFinishNode* FinishNode = Cast<UFlowSubGraphReuseTestFinishNode>(Node.Value))
			{
				return FinishNode;
			}
		}

		return nullptr;
	}

	void SetSubGraphAsset(UFlowNode_SubGraph* SubGraphNode, UFlowAsset* Asset) const
	{
		const FSoftObjectProperty* AssetProperty = FindFProperty<FSoftObjectProperty>(
			UFlowNode_SubGraph::StaticClass(), TEXT("Asset"));
		AssetProperty->SetPropertyValue_InContainer(SubGraphNode, FSoftObjectPtr(Asset));
	}

	FFlowOutputDataPinValues& GetCachedOutputDataPinValues() const
	{
		const FStructProperty* CachedOutputsProperty = FindFProperty<FStructProperty>(
			UFlowNode_SubGraph::StaticClass(), TEXT("CachedOutputDataPinValues"));
		return *CachedOutputsProperty->ContainerPtrToValuePtr<FFlowOutputDataPinValues>(FindSubGraphNode());
	}

	UFlowAsset* FindSubFlow() const
	{
		if (UFlowNode_SubGraph* SubGraphNode = FindSubGraphNode())
		{
			return Subsystem->GetInstancedSubFlows().FindRef(SubGraphNode);
		}

		return nullptr;
	}

	void FinishSubFlow(UFlowAsset* FlowAsset)
	{
		if (UFlowSubGraphReuseTestFinishNode* FinishNode = FindFinishNode(FlowAsset))
		{
			FinishNode->FinishGraph();
		}
	}

	UFlowAsset* StartSubFlow()
	{
		if (UFlowNode_SubGraph* SubGraphNode = FindSubGraphNode())
		{
			SubGraphNode->ExecuteInput(UFlowNode_SubGraph::StartPin.PinName);
		}

		return FindSubFlow();
	}

	void SetFinishPolicy(const ESubGraphFinishPolicy Policy)
	{
		GetMutableDefault<UFlowSettings>()->SubGraphFinishPolicy = Policy;
	}
END_DEFINE_SPEC(FFlowSubGraphReuseSpec)

void FFlowSubGraphReuseSpec::Define()
{
	BeforeEach([this]()
	{
		PreviousAdded = UFlowSubsystem::OnInstancedTemplateAdded;
		PreviousRemoved = UFlowSubsystem::OnInstancedTemplateRemoved;
		UFlowSubsystem::OnInstancedTemplateAdded.Unbind();
		UFlowSubsystem::OnInstancedTemplateRemoved.Unbind();
		PreviousFinishPolicy = GetDefault<UFlowSettings>()->SubGraphFinishPolicy;
		PreviousPreloadPolicy = GetMutableDefault<UFlowSettings>()->PreloadPolicy;
		GetMutableDefault<UFlowSettings>()->PreloadPolicy.InitializeAs<FFlowPreloadPolicy_Standard>();
		GetMutableDefault<UFlowSettings>()->PreloadPolicy.GetMutable<FFlowPreloadPolicy_Standard>().DefaultPreloadTiming =
			EFlowPreloadTiming::ManualOnly;

		GameInstance.Reset(NewObject<UGameInstance>(GEngine));
		World = UWorld::CreateWorld(EWorldType::Game, false);
		World->SetGameInstance(GameInstance.Get());
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.OwningGameInstance = GameInstance.Get();
		Context.SetCurrentWorld(World);
		FFlowSubGraphReuseGameInstanceAccess::SetWorldContext(*GameInstance.Get(), &Context);

		Subsystem.Reset(NewObject<UFlowSubsystem>(GameInstance.Get()));
		ParentTemplate.Reset(NewObject<UFlowAsset>());
		ChildTemplate.Reset(CreateChildTemplate(UFlowAsset::StaticClass()));

		const FGuid ParentStartGuid = FGuid::NewGuid();
		FlowSubGraphReuseTests::AddNodeToTemplate(*ParentTemplate.Get(), ParentStartGuid,
			NewObject<UFlowNode_Start>(ParentTemplate.Get()));

		UFlowNode_SubGraph* ParentSubGraph = NewObject<UFlowNode_SubGraph>(ParentTemplate.Get());
		SetSubGraphAsset(ParentSubGraph, ChildTemplate.Get());
		const FArrayProperty* OutputPinsProperty = FindFProperty<FArrayProperty>(
			UFlowNode::StaticClass(), TEXT("OutputPins"));
		TArray<FFlowPin>& OutputPins = *OutputPinsProperty->ContainerPtrToValuePtr<TArray<FFlowPin>>(ParentSubGraph);
		OutputPins.Add(FFlowPreloadHelper::OUTPIN_Preloaded);
		FlowSubGraphReuseTests::AddNodeToTemplate(*ParentTemplate.Get(), FGuid::NewGuid(), ParentSubGraph);

		OwnerActor = World->SpawnActor<AActor>();
		RootInstance.Reset(Subsystem->CreateRootFlow(OwnerActor, ParentTemplate.Get()));
	});

	AfterEach([this]()
	{
		SetFinishPolicy(PreviousFinishPolicy);
		Subsystem->AbortActiveFlows();
		if (RootInstance.IsValid())
		{
			RootInstance->FinishFlowInstance(EFlowFinishPolicy::Keep);
			RootInstance->DeinitializeInstance();
		}
		GetMutableDefault<UFlowSettings>()->PreloadPolicy = PreviousPreloadPolicy;

		UFlowSubsystem::OnInstancedTemplateAdded = PreviousAdded;
		UFlowSubsystem::OnInstancedTemplateRemoved = PreviousRemoved;
		FFlowSubGraphReuseGameInstanceAccess::SetWorldContext(*GameInstance.Get(), nullptr);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);

		OwnerActor = nullptr;
		World = nullptr;
		RootInstance.Reset();
		ParentTemplate.Reset();
		ChildTemplate.Reset();
		GameInstance.Reset();
		Subsystem.Reset();
	});

	It("creates an initialized parent and subgraph fixture", [this]()
	{
		SetFinishPolicy(ESubGraphFinishPolicy::Keep);

		if (!TestNotNull(TEXT("Root instance"), RootInstance.Get()))
		{
			return;
		}

		TestTrue(TEXT("Root instance initialized"), RootInstance->IsInstanceInitialized());
		TestNotNull(TEXT("Instanced SubGraph node"), FindSubGraphNode());

		UFlowAsset* SubFlow = StartSubFlow();
		if (!TestNotNull(TEXT("Subgraph instance"), SubFlow))
		{
			return;
		}

		TestTrue(TEXT("Subgraph is running before finish"), SubFlow->IsActive());
		FinishSubFlow(SubFlow);
		TestTrue(TEXT("Subgraph instance initialized"), SubFlow->IsInstanceInitialized());
		TestTrue(TEXT("Finished subgraph remains tracked under Keep"), Subsystem->GetInstancedSubFlows().Contains(FindSubGraphNode()));

		UFlowSubGraphReuseTestFinishNode* FinishNode = FindFinishNode(SubFlow);
		TestNotNull(TEXT("Activation-recording node"), FinishNode);
		if (FinishNode)
		{
			TestEqual(TEXT("Activation recorded"), FinishNode->ActivationCount, 1);
		}
	});

	It("deinitializes the finished child and creates a different instance under Deinitialize", [this]()
	{
		SetFinishPolicy(ESubGraphFinishPolicy::Deinitialize);

		UFlowAsset* FirstInstance = StartSubFlow();
		if (!TestNotNull(TEXT("First child instance"), FirstInstance))
		{
			return;
		}

		TStrongObjectPtr<UFlowAsset> FirstInstanceKeepAlive(FirstInstance);
		TestTrue(TEXT("First child is still running"), FirstInstance->IsActive());
		FinishSubFlow(FirstInstance);
		TestFalse(TEXT("Finished child deinitialized"), FirstInstance->IsInstanceInitialized());
		TestFalse(TEXT("Finished child no longer tracked"),
			Subsystem->GetInstancedSubFlows().Contains(FindSubGraphNode()));

		UFlowAsset* SecondInstance = StartSubFlow();
		if (!TestNotNull(TEXT("Second child instance"), SecondInstance))
		{
			return;
		}

		TestNotEqual(TEXT("Rerun creates a new child instance"), SecondInstance, FirstInstance);
		TestTrue(TEXT("New child initialized"), SecondInstance->IsInstanceInitialized());
	});

	It("keeps a finished child tracked under Keep and replaces it on rerun", [this]()
	{
		SetFinishPolicy(ESubGraphFinishPolicy::Keep);

		UFlowAsset* FirstInstance = StartSubFlow();
		if (!TestNotNull(TEXT("First child instance"), FirstInstance))
		{
			return;
		}

		TStrongObjectPtr<UFlowAsset> FirstInstanceKeepAlive(FirstInstance);
		TestTrue(TEXT("First child is still running"), FirstInstance->IsActive());
		FinishSubFlow(FirstInstance);
		TestTrue(TEXT("Finished child remains initialized"), FirstInstance->IsInstanceInitialized());
		TestTrue(TEXT("Finished child remains tracked"),
			Subsystem->GetInstancedSubFlows().Contains(FindSubGraphNode()));
		TestEqual(TEXT("Tracked child remains the finished instance"), FindSubFlow(), FirstInstance);

		UFlowAsset* SecondInstance = StartSubFlow();
		if (!TestNotNull(TEXT("Second child instance"), SecondInstance))
		{
			return;
		}

		TestNotEqual(TEXT("Rerun replaces the child instance"), SecondInstance, FirstInstance);
		TestFalse(TEXT("Replaced child deinitialized"), FirstInstance->IsInstanceInitialized());
		TestEqual(TEXT("New child is tracked"), FindSubFlow(), SecondInstance);
	});

	It("restarts the same initialized child instance under Reuse", [this]()
	{
		SetFinishPolicy(ESubGraphFinishPolicy::Reuse);

		UFlowAsset* FirstInstance = StartSubFlow();
		if (!TestNotNull(TEXT("First child instance"), FirstInstance))
		{
			return;
		}

		TStrongObjectPtr<UFlowAsset> FirstInstanceKeepAlive(FirstInstance);
		FinishSubFlow(FirstInstance);
		TestTrue(TEXT("Finished child remains initialized"), FirstInstance->IsInstanceInitialized());
		TestEqual(TEXT("Finished child remains tracked"), FindSubFlow(), FirstInstance);

		UFlowAsset* SecondInstance = StartSubFlow();
		if (!TestNotNull(TEXT("Second child instance"), SecondInstance))
		{
			return;
		}

		TestEqual(TEXT("Rerun uses the same child instance"), SecondInstance, FirstInstance);
		TestTrue(TEXT("Child was never deinitialized"), FirstInstance->IsInstanceInitialized());
	});

	It("resets node activation state and records before the Reuse restart", [this]()
	{
		SetFinishPolicy(ESubGraphFinishPolicy::Reuse);

		UFlowAsset* FirstInstance = StartSubFlow();
		if (!TestNotNull(TEXT("First child instance"), FirstInstance))
		{
			return;
		}

		TStrongObjectPtr<UFlowAsset> FirstInstanceKeepAlive(FirstInstance);
		FinishSubFlow(FirstInstance);
		UFlowAsset* SecondInstance = StartSubFlow();
		if (!TestNotNull(TEXT("Second child instance"), SecondInstance))
		{
			return;
		}

		UFlowSubGraphReuseTestStartNode* StartNode = FindStartNode(SecondInstance);
		if (!TestNotNull(TEXT("Restarted Start node"), StartNode))
		{
			return;
		}

		TestEqual(TEXT("Reuse preserves the Flow Asset instance"), SecondInstance, FirstInstance);
		TestTrue(FString::Printf(TEXT("No node has a previous activation: %s"),
			*StartNode->OtherNodesWithPreviousActivation), StartNode->bAllNodesNeverActivatedBeforeStart);
		TestTrue(TEXT("No previous activation records remain"),
			StartNode->bNoPreviousRecordedNodesBeforeStart);
	});

	It("clears cached outputs before the Reuse restart", [this]()
	{
		SetFinishPolicy(ESubGraphFinishPolicy::Reuse);

		UFlowAsset* FirstInstance = StartSubFlow();
		if (!TestNotNull(TEXT("First child instance"), FirstInstance))
		{
			return;
		}

		TStrongObjectPtr<UFlowAsset> FirstInstanceKeepAlive(FirstInstance);
		FinishSubFlow(FirstInstance);

		FFlowOutputDataPinValues& CachedOutputs = GetCachedOutputDataPinValues();
		TInstancedStruct<FFlowDataPinValue> PreviousOutput;
		PreviousOutput.InitializeAs<FFlowDataPinValue_Bool>(true);
		CachedOutputs.Values.Add(TEXT("PreviousRun"), MoveTemp(PreviousOutput));
		TestTrue(TEXT("Previous run output is present before rerun"), CachedOutputs.Values.Contains(TEXT("PreviousRun")));

		UFlowAsset* SecondInstance = StartSubFlow();
		if (!TestNotNull(TEXT("Second child instance"), SecondInstance))
		{
			return;
		}

		TestEqual(TEXT("Reuse preserves the Flow Asset instance"), SecondInstance, FirstInstance);
		TestFalse(TEXT("Previous output is no longer exposed"), CachedOutputs.Values.Contains(TEXT("PreviousRun")));
	});

	It("preserves an injected component on the same child instance under Reuse", [this]()
	{
		SetFinishPolicy(ESubGraphFinishPolicy::Reuse);
		FindFinishNode(ChildTemplate.Get())->bInjectComponentOnActivation = true;

		UFlowAsset* FirstInstance = StartSubFlow();
		if (!TestNotNull(TEXT("First child instance"), FirstInstance))
		{
			return;
		}

		TStrongObjectPtr<UFlowAsset> FirstInstanceKeepAlive(FirstInstance);
		UFlowSubGraphReuseTestFinishNode* FirstFinishNode = FindFinishNode(FirstInstance);
		if (!TestNotNull(TEXT("First activation node"), FirstFinishNode))
		{
			return;
		}

		UFlowSubGraphReuseTestComponent* FirstComponent = FirstFinishNode->InjectedComponent;
		if (!TestNotNull(TEXT("Injected component"), FirstComponent))
		{
			return;
		}
		TestTrue(TEXT("Injected component is registered"), FirstComponent->IsRegistered());

		FinishSubFlow(FirstInstance);
		UFlowAsset* SecondInstance = StartSubFlow();
		if (!TestNotNull(TEXT("Second child instance"), SecondInstance))
		{
			return;
		}

		UFlowSubGraphReuseTestFinishNode* SecondFinishNode = FindFinishNode(SecondInstance);
		if (!TestNotNull(TEXT("Restarted activation node"), SecondFinishNode))
		{
			return;
		}

		TestEqual(TEXT("Reuse preserves the child instance"), SecondInstance, FirstInstance);
		TestTrue(TEXT("Rerun retains the injected component object"),
			SecondFinishNode->InjectedComponent == FirstComponent);
		TestTrue(TEXT("Injected component remains registered"), FirstComponent->IsRegistered());
	});

	It("keeps a child-injected component until the root and child release it under Reuse", [this]()
	{
		SetFinishPolicy(ESubGraphFinishPolicy::Reuse);

		UFlowAsset* FirstInstance = StartSubFlow();
		if (!TestNotNull(TEXT("First child instance"), FirstInstance))
		{
			return;
		}

		TStrongObjectPtr<UFlowAsset> FirstInstanceKeepAlive(FirstInstance);
		UFlowInjectComponentsManager* RootManager = RootInstance->GetOrCreateComponentInjectionManager();
		UFlowSubGraphReuseTestComponent* Component = NewObject<UFlowSubGraphReuseTestComponent>(OwnerActor);
		RootManager->InjectComponentOnActor(*OwnerActor, *Component, FirstInstance);
		RootManager->RegisterComponentUser(*RootInstance, *Component);

		TestEqual(TEXT("Child and root are both users"), RootManager->GetComponentUserCount(Component), 2);
		TestEqual(TEXT("Root and child share one injected record"), RootManager->GetInjectedComponentCount(), 1);
		FinishSubFlow(FirstInstance);
		TestTrue(TEXT("Reuse keeps the child user's component"), IsValid(Component));

		UFlowAsset* ReusedInstance = StartSubFlow();
		TestEqual(TEXT("Reuse keeps the same child instance"), ReusedInstance, FirstInstance);
		TestEqual(TEXT("Rerun does not add duplicate component users"), RootManager->GetComponentUserCount(Component), 2);
		TestEqual(TEXT("Rerun does not add a duplicate injected record"), RootManager->GetInjectedComponentCount(), 1);

		FindSubGraphNode()->FlushContent();
		TestFalse(TEXT("Deinitializing the child releases its user"), FirstInstance->IsInstanceInitialized());
		TestTrue(TEXT("The root user retains the component"), IsValid(Component));
		TestEqual(TEXT("Only the root remains a user"), RootManager->GetComponentUserCount(Component), 1);

		RootManager->UnregisterComponentUser(*RootInstance, *Component);
		TestFalse(TEXT("The last root user removes the component"), IsValid(Component));
		TestEqual(TEXT("The released component record is removed"), RootManager->GetInjectedComponentCount(), 0);
	});

	It("deinitializes and recreates the child after FlushContent under Reuse", [this]()
	{
		SetFinishPolicy(ESubGraphFinishPolicy::Reuse);

		UFlowAsset* FirstInstance = StartSubFlow();
		if (!TestNotNull(TEXT("First child instance"), FirstInstance))
		{
			return;
		}

		TStrongObjectPtr<UFlowAsset> FirstInstanceKeepAlive(FirstInstance);
		FinishSubFlow(FirstInstance);
		FindSubGraphNode()->FlushContent();
		TestFalse(TEXT("Flushed child deinitialized"), FirstInstance->IsInstanceInitialized());
		TestNull(TEXT("Flushed child no longer tracked"), FindSubFlow());

		UFlowAsset* SecondInstance = StartSubFlow();
		if (!TestNotNull(TEXT("Child after flush"), SecondInstance))
		{
			return;
		}

		TestNotEqual(TEXT("Flush forces a new child instance"), SecondInstance, FirstInstance);
	});

	It("uses the child asset class policy instead of the project setting", [this]()
	{
		SetFinishPolicy(ESubGraphFinishPolicy::Reuse);
		TStrongObjectPtr<UFlowAsset> OverrideTemplate(
			CreateChildTemplate(UFlowSubGraphReusePolicyOverrideAsset::StaticClass()));
		SetSubGraphAsset(FindSubGraphNode(), OverrideTemplate.Get());

		UFlowAsset* FirstInstance = StartSubFlow();
		if (!TestNotNull(TEXT("First override child"), FirstInstance))
		{
			return;
		}

		TestEqual(TEXT("Child class overrides project setting"),
			FirstInstance->GetSubGraphFinishPolicy(), ESubGraphFinishPolicy::Deinitialize);
		TStrongObjectPtr<UFlowAsset> FirstInstanceKeepAlive(FirstInstance);
		FinishSubFlow(FirstInstance);
		TestFalse(TEXT("Override Deinitialize policy removed the finished instance"),
			FirstInstance->IsInstanceInitialized());

		UFlowAsset* SecondInstance = StartSubFlow();
		if (!TestNotNull(TEXT("Second override child"), SecondInstance))
		{
			return;
		}

		TestNotEqual(TEXT("Override policy creates a new instance"), SecondInstance, FirstInstance);
	});

	It("uses the configured project default for a child asset without a policy override", [this]()
	{
		const ESubGraphFinishPolicy ProjectDefault = GetDefault<UFlowSettings>()->SubGraphFinishPolicy;

		UFlowAsset* FirstInstance = StartSubFlow();
		if (!TestNotNull(TEXT("First child instance"), FirstInstance))
		{
			return;
		}

		TestEqual(TEXT("Child uses the configured project default"),
			FirstInstance->GetSubGraphFinishPolicy(), ProjectDefault);

		TStrongObjectPtr<UFlowAsset> FirstInstanceKeepAlive(FirstInstance);
		FinishSubFlow(FirstInstance);
		const bool bShouldRemainInitialized = ProjectDefault != ESubGraphFinishPolicy::Deinitialize;
		TestEqual(TEXT("Configured policy controls whether the child stays initialized"),
			FirstInstance->IsInstanceInitialized(), bShouldRemainInitialized);

		UFlowAsset* SecondInstance = StartSubFlow();
		if (!TestNotNull(TEXT("Second child instance"), SecondInstance))
		{
			return;
		}

		const bool bShouldReuseInstance = ProjectDefault == ESubGraphFinishPolicy::Reuse;
		TestEqual(TEXT("Only the Reuse policy preserves the child instance"),
			SecondInstance == FirstInstance, bShouldReuseInstance);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
