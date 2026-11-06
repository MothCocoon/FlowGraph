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
#include "Interfaces/FlowFastForwardGate.h"
#include "Nodes/Actor/FlowNode_OnActorRegistered.h"
#include "Nodes/Graph/FlowNode_CustomInput.h"
#include "Nodes/Graph/FlowNode_CustomOutput.h"
#include "Nodes/Graph/FlowNode_Finish.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"
#include "Nodes/Route/FlowNode_Branch.h"
#include "Nodes/Route/FlowNode_ExecutionMultiGate.h"
#include "Nodes/Route/FlowNode_Timer.h"
#include "TimerManager.h"
#include "Policies/FlowPreloadPolicyEnums.h"
#include "Policies/FlowStandardPreloadPolicies.h"
#include "FastForward/FlowFastForwardRouteFinder.h"
#include "FastForward/FlowFastForwardSubsystem.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#include "FlowFastForwardTestTypes.h"

struct FFlowFastForwardGameInstanceAccess : UGameInstance
{
	static void SetWorldContext(UGameInstance& Instance, FWorldContext* Context)
	{
		FWorldContext* UGameInstance::* ContextMember = &FFlowFastForwardGameInstanceAccess::WorldContext;
		Instance.*ContextMember = Context;
	}
};

BEGIN_DEFINE_SPEC(FFlowFastForwardSpec, "FlowEditor.FastForward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TStrongObjectPtr<UGameInstance> GameInstance;
	TStrongObjectPtr<UFlowSubsystem> Subsystem;
	TStrongObjectPtr<UFlowFastForwardSubsystem> FastForwardSubsystem;
	TStrongObjectPtr<UFlowAsset> RootTemplate;
	TStrongObjectPtr<UFlowAsset> ChildTemplate;
	TStrongObjectPtr<UFlowAsset> RootInstance;
	UWorld* World = nullptr;
	AActor* OwnerActor = nullptr;
	FInstancedStruct PreviousPreloadPolicy;
	FNativeFlowAssetEvent PreviousAdded;
	FNativeFlowAssetEvent PreviousRemoved;

	template <class T>
	T* AddNode(UFlowAsset& Template) const
	{
		T* Node = NewObject<T>(&Template);
		Node->SetGuid(FGuid::NewGuid());

		const FMapProperty* NodesProperty = FindFProperty<FMapProperty>(UFlowAsset::StaticClass(), TEXT("Nodes"));
		NodesProperty->ContainerPtrToValuePtr<TMap<FGuid, TObjectPtr<UFlowNode>>>(&Template)->Add(Node->GetGuid(), Node);
		return Node;
	}

	UFlowNode_SubGraph* AddSubGraphNode(UFlowAsset& Template, UFlowAsset& Asset) const
	{
		UFlowNode_SubGraph* SubGraphNode = AddNode<UFlowNode_SubGraph>(Template);

		const FSoftObjectProperty* AssetProperty = FindFProperty<FSoftObjectProperty>(UFlowNode_SubGraph::StaticClass(), TEXT("Asset"));
		AssetProperty->SetPropertyValue_InContainer(SubGraphNode, FSoftObjectPtr(&Asset));
		return SubGraphNode;
	}

	void ConnectPins(UFlowNode& From, const FName& OutputPinName, const UFlowNode& To, const FName& InputPinName) const
	{
		const FMapProperty* ConnectionsProperty = FindFProperty<FMapProperty>(UFlowNode::StaticClass(), TEXT("Connections"));
		ConnectionsProperty->ContainerPtrToValuePtr<TMap<FName, FConnectedPin>>(&From)->Add(OutputPinName, FConnectedPin(To.GetGuid(), InputPinName));
	}

	void Connect(UFlowNode& From, const UFlowNode& To) const
	{
		ConnectPins(From, UFlowNode::DefaultOutputPin.PinName, To, UFlowNode::DefaultInputPin.PinName);
	}

	void AddOutputPin(UFlowNode& Node, const FName& PinName) const
	{
		const FArrayProperty* OutputPinsProperty = FindFProperty<FArrayProperty>(UFlowNode::StaticClass(), TEXT("OutputPins"));
		OutputPinsProperty->ContainerPtrToValuePtr<TArray<FFlowPin>>(&Node)->Add(FFlowPin(PinName));
	}

	template <class TValue, class TProperty>
	void SetProperty(UObject& Object, const FName& PropertyName, const TValue& Value) const
	{
		const TProperty* Property = FindFProperty<TProperty>(Object.GetClass(), PropertyName);
		*Property->template ContainerPtrToValuePtr<TValue>(&Object) = Value;
	}

	bool StartFastForwardToNode(const UFlowNode* TargetNode)
	{
		FFlowFastForwardRoute Route;
		const bool bFound = TestEqual(TEXT("Result"), FindRoute(TargetNode, Route), EFlowFastForwardRouteResult::Found);

		if (bFound)
		{
			FastForwardSubsystem->StartFastForward(Route);
		}

		return bFound;
	}

	void CreateRootInstance()
	{
		RootInstance.Reset(Subsystem->CreateRootFlow(OwnerActor, RootTemplate.Get()));
	}

	UFlowNode* GetInstanceNode(const UFlowAsset* Instance, const UFlowNode& TemplateNode) const
	{
		return Instance ? Instance->GetNode(TemplateNode.GetGuid()) : nullptr;
	}

	void SetInstanceNodeState(const UFlowAsset* Instance, const UFlowNode& TemplateNode, const EFlowNodeState State) const
	{
		if (UFlowNode* InstanceNode = GetInstanceNode(Instance, TemplateNode))
		{
			InstanceNode->SetActivationState(State);
		}
	}

	UFlowAsset* StartSubFlow(const UFlowNode_SubGraph& TemplateSubGraphNode) const
	{
		UFlowAsset* SubFlow = nullptr;

		if (UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(GetInstanceNode(RootInstance.Get(), TemplateSubGraphNode)))
		{
			SubGraphNode->ExecuteInput(UFlowNode_SubGraph::StartPin.PinName);
			SubFlow = Subsystem->GetInstancedSubFlows().FindRef(SubGraphNode);
		}

		return SubFlow;
	}

	EFlowFastForwardRouteResult FindRoute(const UFlowNode* TargetNode, FFlowFastForwardRoute& OutRoute) const
	{
		EFlowFastForwardRouteResult Result = EFlowFastForwardRouteResult::InvalidTarget;

		if (TargetNode)
		{
			FFlowFastForwardRouteFinder RouteFinder(*Subsystem.Get());
			Result = RouteFinder.FindRoute(*TargetNode, OutRoute);
		}

		return Result;
	}

	void TestStep(const FString& What, const FFlowFastForwardRoute& Route, const int32 SegmentIndex, const int32 StepIndex,
		const UFlowNode& Node, const FName& InputPinName, const FName& OutputPinName)
	{
		if (Route.Segments.IsValidIndex(SegmentIndex) && Route.Segments[SegmentIndex].Steps.IsValidIndex(StepIndex))
		{
			const FFlowFastForwardStep& Step = Route.Segments[SegmentIndex].Steps[StepIndex];
			TestEqual(*(What + TEXT(" node")), Step.NodeGuid.ToString(), Node.GetGuid().ToString());
			TestEqual(*(What + TEXT(" input pin")), Step.InputPinName.ToString(), InputPinName.ToString());
			TestEqual(*(What + TEXT(" output pin")), Step.OutputPinName.ToString(), OutputPinName.ToString());
		}
		else
		{
			AddError(FString::Printf(TEXT("%s is missing"), *What));
		}
	}
END_DEFINE_SPEC(FFlowFastForwardSpec)

void FFlowFastForwardSpec::Define()
{
	const FName In = UFlowNode::DefaultInputPin.PinName;
	const FName Out = UFlowNode::DefaultOutputPin.PinName;
	const FName Start = UFlowNode_SubGraph::StartPin.PinName;

	BeforeEach([this]()
	{
		PreviousAdded = UFlowSubsystem::OnInstancedTemplateAdded;
		PreviousRemoved = UFlowSubsystem::OnInstancedTemplateRemoved;
		UFlowSubsystem::OnInstancedTemplateAdded.Unbind();
		UFlowSubsystem::OnInstancedTemplateRemoved.Unbind();
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
		FFlowFastForwardGameInstanceAccess::SetWorldContext(*GameInstance.Get(), &Context);

		Subsystem.Reset(NewObject<UFlowSubsystem>(GameInstance.Get()));
		FastForwardSubsystem.Reset(NewObject<UFlowFastForwardSubsystem>(GameInstance.Get()));
		RootTemplate.Reset(NewObject<UFlowAsset>());
		ChildTemplate.Reset(NewObject<UFlowAsset>());
		OwnerActor = World->SpawnActor<AActor>();
	});

	AfterEach([this]()
	{
		FastForwardSubsystem->StopFastForward();
		Subsystem->AbortActiveFlows();
		if (RootInstance.IsValid())
		{
			RootInstance->FinishFlowInstance(EFlowFinishPolicy::Keep);
			RootInstance->DeinitializeInstance();
		}
		GetMutableDefault<UFlowSettings>()->PreloadPolicy = PreviousPreloadPolicy;

		UFlowSubsystem::OnInstancedTemplateAdded = PreviousAdded;
		UFlowSubsystem::OnInstancedTemplateRemoved = PreviousRemoved;
		FFlowFastForwardGameInstanceAccess::SetWorldContext(*GameInstance.Get(), nullptr);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);

		OwnerActor = nullptr;
		World = nullptr;
		RootInstance.Reset();
		RootTemplate.Reset();
		ChildTemplate.Reset();
		GameInstance.Reset();
		FastForwardSubsystem.Reset();
		Subsystem.Reset();
	});

	It("starts from the nearest active node", [this, In, Out]()
	{
		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* B = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* C = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		Connect(*A, *X);
		Connect(*B, *C);
		Connect(*C, *X);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);
		SetInstanceNodeState(RootInstance.Get(), *B, EFlowNodeState::Active);

		FFlowFastForwardRoute ShortRoute;
		TestEqual(TEXT("Result with both branches active"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), ShortRoute), EFlowFastForwardRouteResult::Found);
		TestEqual(TEXT("Steps with both branches active"), ShortRoute.Segments.Num() == 1 ? ShortRoute.Segments[0].Steps.Num() : 0, 2);
		TestStep(TEXT("Shorter branch"), ShortRoute, 0, 0, *A, NAME_None, Out);

		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Completed);

		FFlowFastForwardRoute LongRoute;
		TestEqual(TEXT("Result with longer branch active"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), LongRoute), EFlowFastForwardRouteResult::Found);
		TestEqual(TEXT("Steps with longer branch active"), LongRoute.Segments.Num() == 1 ? LongRoute.Segments[0].Steps.Num() : 0, 3);
		TestStep(TEXT("Longer branch"), LongRoute, 0, 0, *B, NAME_None, Out);
		TestStep(TEXT("Longer branch middle node"), LongRoute, 0, 1, *C, In, Out);
		TestStep(TEXT("Longer branch target"), LongRoute, 0, 2, *X, In, NAME_None);
	});

	It("doesn't restart a running SubGraph", [this]()
	{
		const FName AltPin = TEXT("Alt");

		UFlowNode_Start* ChildStart = AddNode<UFlowNode_Start>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* Branch = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* Y = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		Branch->AddTestOutputPin(AltPin);
		Connect(*ChildStart, *Branch);
		Connect(*Branch, *Y);
		ConnectPins(*Branch, AltPin, *X, UFlowNode::DefaultInputPin.PinName);

		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		ConnectPins(*A, UFlowNode::DefaultOutputPin.PinName, *SubGraph, UFlowNode_SubGraph::StartPin.PinName);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);
		UFlowAsset* SubFlow = StartSubFlow(*SubGraph);
		if (!TestNotNull(TEXT("SubGraph instance"), SubFlow))
		{
			return;
		}

		SetInstanceNodeState(RootInstance.Get(), *SubGraph, EFlowNodeState::Active);
		SetInstanceNodeState(SubFlow, *Branch, EFlowNodeState::Completed);
		SetInstanceNodeState(SubFlow, *Y, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(SubFlow, *X), Route), EFlowFastForwardRouteResult::NoActiveNodeOnRoute);
		TestTrue(TEXT("Route is empty"), Route.IsEmpty());
	});

	It("enters a graph not instanced yet", [this, In, Out, Start]()
	{
		UFlowNode_Start* ChildStart = AddNode<UFlowNode_Start>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* B = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		Connect(*ChildStart, *B);
		Connect(*B, *X);

		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		ConnectPins(*A, Out, *SubGraph, Start);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(X, Route), EFlowFastForwardRouteResult::Found);
		TestTrue(TEXT("Start instance"), Route.StartInstance.Get() == RootInstance.Get());
		TestEqual(TEXT("Segments"), Route.Segments.Num(), 2);
		if (Route.Segments.Num() == 2)
		{
			TestTrue(TEXT("Parent segment template"), Route.Segments[0].TemplateAsset.Get() == RootTemplate.Get());
			TestTrue(TEXT("Child segment template"), Route.Segments[1].TemplateAsset.Get() == ChildTemplate.Get());
			TestEqual(TEXT("Parent steps"), Route.Segments[0].Steps.Num(), 2);
			TestEqual(TEXT("Child steps"), Route.Segments[1].Steps.Num(), 3);
		}
		TestStep(TEXT("Active node"), Route, 0, 0, *A, NAME_None, Out);
		TestStep(TEXT("SubGraph node"), Route, 0, 1, *SubGraph, Start, NAME_None);
		TestStep(TEXT("Child Start node"), Route, 1, 0, *ChildStart, NAME_None, Out);
		TestStep(TEXT("Child middle node"), Route, 1, 1, *B, In, Out);
		TestStep(TEXT("Target node"), Route, 1, 2, *X, In, NAME_None);
	});

	It("enters a running graph through the Custom Input", [this, In, Out]()
	{
		const FName EventName = TEXT("Event");

		UFlowNode_CustomInput* CustomInput = AddNode<UFlowNode_CustomInput>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		CustomInput->SetEventName(EventName);
		Connect(*CustomInput, *X);

		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		ConnectPins(*A, Out, *SubGraph, EventName);

		CreateRootInstance();
		UFlowAsset* SubFlow = StartSubFlow(*SubGraph);
		if (!TestNotNull(TEXT("SubGraph instance"), SubFlow))
		{
			return;
		}

		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);
		SetInstanceNodeState(RootInstance.Get(), *SubGraph, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(SubFlow, *X), Route), EFlowFastForwardRouteResult::Found);
		TestEqual(TEXT("Segments"), Route.Segments.Num(), 2);
		TestStep(TEXT("Active node"), Route, 0, 0, *A, NAME_None, Out);
		TestStep(TEXT("SubGraph node"), Route, 0, 1, *SubGraph, EventName, NAME_None);
		TestStep(TEXT("Custom Input node"), Route, 1, 0, *CustomInput, NAME_None, Out);
		TestStep(TEXT("Target node"), Route, 1, 1, *X, In, NAME_None);
	});

	It("doesn't enter a graph not instanced yet through the Custom Input", [this, Out]()
	{
		const FName EventName = TEXT("Event");

		UFlowNode_CustomInput* CustomInput = AddNode<UFlowNode_CustomInput>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		CustomInput->SetEventName(EventName);
		Connect(*CustomInput, *X);

		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		ConnectPins(*A, Out, *SubGraph, EventName);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(X, Route), EFlowFastForwardRouteResult::NoActiveNodeOnRoute);
	});

	It("skips an input that can't lead to the required output", [this, In, Out]()
	{
		UFlowFastForwardTestNode* A1 = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* A2 = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* B = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_Timer* Timer = AddNode<UFlowNode_Timer>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		ConnectPins(*A1, Out, *Timer, TEXT("Skip"));
		Connect(*A2, *B);
		ConnectPins(*B, Out, *Timer, In);
		ConnectPins(*Timer, TEXT("Completed"), *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A1, EFlowNodeState::Active);
		SetInstanceNodeState(RootInstance.Get(), *A2, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), Route), EFlowFastForwardRouteResult::Found);
		TestStep(TEXT("Active node reaching the timer input"), Route, 0, 0, *A2, NAME_None, Out);
		TestStep(TEXT("Timer"), Route, 0, 2, *Timer, In, TEXT("Completed"));
	});

	It("stops at the Start node chosen as the target", [this, Out]()
	{
		UFlowNode_Start* ChildStart = AddNode<UFlowNode_Start>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* B = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		Connect(*ChildStart, *B);

		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		ConnectPins(*A, Out, *SubGraph, UFlowNode_SubGraph::StartPin.PinName);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		if (!TestEqual(TEXT("Result"), FindRoute(ChildStart, Route), EFlowFastForwardRouteResult::Found))
		{
			return;
		}

		FastForwardSubsystem->StartFastForward(Route);

		TestFalse(TEXT("Fast Forward finished"), FastForwardSubsystem->IsFastForwarding());
		TestNull(TEXT("Fast Forward gate released"), FFlowFastForwardGate::GetGate());
	});

	It("stops when a node finishes by another output", [this, In, Out]()
	{
		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardWrongOutputTestNode* W = AddNode<UFlowFastForwardWrongOutputTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		Connect(*A, *W);
		ConnectPins(*W, Out, *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		// Node and subsystem both report the broken route
		AddExpectedMessagePlain(TEXT("Fast Forward stopped"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

		FFlowFastForwardRoute Route;
		if (!TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), Route), EFlowFastForwardRouteResult::Found))
		{
			return;
		}

		FastForwardSubsystem->StartFastForward(Route);

		const UFlowFastForwardTestNode* InstanceX = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *X));
		TestFalse(TEXT("Fast Forward stopped"), FastForwardSubsystem->IsFastForwarding());
		TestNull(TEXT("Fast Forward gate released"), FFlowFastForwardGate::GetGate());
		TestTrue(TEXT("Target node not reached"), InstanceX && InstanceX->ExecuteInputCount == 0);
	});

	It("enters the parent of a finished instance", [this, In, Out, Start]()
	{
		UFlowNode_Start* ChildStart = AddNode<UFlowNode_Start>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		Connect(*ChildStart, *X);

		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		ConnectPins(*A, Out, *SubGraph, Start);

		CreateRootInstance();
		UFlowAsset* SubFlow = StartSubFlow(*SubGraph);
		if (!TestNotNull(TEXT("SubGraph instance"), SubFlow))
		{
			return;
		}

		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);
		SetInstanceNodeState(RootInstance.Get(), *SubGraph, EFlowNodeState::Completed);
		SetInstanceNodeState(SubFlow, *X, EFlowNodeState::Completed);

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(SubFlow, *X), Route), EFlowFastForwardRouteResult::Found);
		TestEqual(TEXT("Segments"), Route.Segments.Num(), 2);
		TestStep(TEXT("Active node"), Route, 0, 0, *A, NAME_None, Out);
		TestStep(TEXT("SubGraph node"), Route, 0, 1, *SubGraph, Start, NAME_None);
		TestStep(TEXT("Child Start node"), Route, 1, 0, *ChildStart, NAME_None, Out);
		TestStep(TEXT("Target node"), Route, 1, 1, *X, In, NAME_None);
	});

	It("fast forwards the route into a graph not instanced yet", [this]()
	{
		UFlowNode_Start* ChildStart = AddNode<UFlowNode_Start>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* B = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		Connect(*ChildStart, *B);
		Connect(*B, *X);

		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		ConnectPins(*A, UFlowNode::DefaultOutputPin.PinName, *SubGraph, UFlowNode_SubGraph::StartPin.PinName);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		if (!TestEqual(TEXT("Result"), FindRoute(X, Route), EFlowFastForwardRouteResult::Found))
		{
			return;
		}

		FastForwardSubsystem->StartFastForward(Route);

		const UFlowFastForwardTestNode* InstanceA = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *A));
		UFlowNode_SubGraph* InstanceSubGraph = Cast<UFlowNode_SubGraph>(GetInstanceNode(RootInstance.Get(), *SubGraph));
		const UFlowAsset* SubFlow = InstanceSubGraph ? Subsystem->GetInstancedSubFlows().FindRef(InstanceSubGraph) : nullptr;
		const UFlowFastForwardTestNode* InstanceB = Cast<UFlowFastForwardTestNode>(GetInstanceNode(SubFlow, *B));
		const UFlowFastForwardTestNode* InstanceX = Cast<UFlowFastForwardTestNode>(GetInstanceNode(SubFlow, *X));
		if (!TestNotNull(TEXT("SubGraph instance"), SubFlow)
			|| !TestNotNull(TEXT("Active node"), InstanceA)
			|| !TestNotNull(TEXT("Middle node"), InstanceB)
			|| !TestNotNull(TEXT("Target node"), InstanceX))
		{
			return;
		}

		TestEqual(TEXT("Active node fast forwarded"), InstanceA->FastForwardInputCount, 1);
		TestEqual(TEXT("Active node still active after forced output"), InstanceA->GetActivationState(), EFlowNodeState::Active);
		TestEqual(TEXT("SubGraph node executed"), InstanceSubGraph->GetActivationState(), EFlowNodeState::Active);
		TestEqual(TEXT("Middle node fast forwarded"), InstanceB->FastForwardInputCount, 1);
		TestEqual(TEXT("Middle node logic executed"), InstanceB->ExecuteInputCount, 1);
		TestEqual(TEXT("Middle node still active after forced output"), InstanceB->GetActivationState(), EFlowNodeState::Active);
		TestEqual(TEXT("Target node executed"), InstanceX->ExecuteInputCount, 1);
		TestEqual(TEXT("Target node not fast forwarded"), InstanceX->FastForwardInputCount, 0);
		TestEqual(TEXT("Target node active"), InstanceX->GetActivationState(), EFlowNodeState::Active);
		TestFalse(TEXT("Fast Forward finished"), FastForwardSubsystem->IsFastForwarding());
		TestNull(TEXT("Fast Forward gate released"), FFlowFastForwardGate::GetGate());
	});

	It("leaves a running graph through its Finish node", [this, In, Out]()
	{
		UFlowNode_Start* ChildStart = AddNode<UFlowNode_Start>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* W = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		UFlowNode_Finish* ChildFinish = AddNode<UFlowNode_Finish>(*ChildTemplate.Get());
		Connect(*ChildStart, *W);
		ConnectPins(*W, Out, *ChildFinish, In);

		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		ConnectPins(*SubGraph, UFlowNode_SubGraph::FinishPin.PinName, *X, In);

		CreateRootInstance();
		UFlowAsset* SubFlow = StartSubFlow(*SubGraph);
		if (!TestNotNull(TEXT("SubGraph instance"), SubFlow))
		{
			return;
		}

		SetInstanceNodeState(RootInstance.Get(), *SubGraph, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), Route), EFlowFastForwardRouteResult::Found);
		TestTrue(TEXT("Start instance"), Route.StartInstance.Get() == SubFlow);
		TestEqual(TEXT("Segments"), Route.Segments.Num(), 2);
		TestStep(TEXT("Active node in the SubGraph"), Route, 0, 0, *W, NAME_None, Out);
		TestStep(TEXT("Finish node"), Route, 0, 1, *ChildFinish, In, NAME_None);
		TestStep(TEXT("Target node"), Route, 1, 0, *X, In, NAME_None);
	});

	It("fast forwards the route through a whole SubGraph", [this, In, Out]()
	{
		UFlowNode_Start* ChildStart = AddNode<UFlowNode_Start>(*ChildTemplate.Get());
		UFlowFastForwardTestNode* B = AddNode<UFlowFastForwardTestNode>(*ChildTemplate.Get());
		UFlowNode_Finish* ChildFinish = AddNode<UFlowNode_Finish>(*ChildTemplate.Get());
		Connect(*ChildStart, *B);
		ConnectPins(*B, Out, *ChildFinish, In);

		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		ConnectPins(*A, Out, *SubGraph, UFlowNode_SubGraph::StartPin.PinName);
		ConnectPins(*SubGraph, UFlowNode_SubGraph::FinishPin.PinName, *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		if (!TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), Route), EFlowFastForwardRouteResult::Found))
		{
			return;
		}

		TestEqual(TEXT("Segments"), Route.Segments.Num(), 3);
		TestStep(TEXT("Child middle node"), Route, 1, 1, *B, In, Out);
		TestStep(TEXT("Child Finish node"), Route, 1, 2, *ChildFinish, In, NAME_None);
		TestStep(TEXT("Target node"), Route, 2, 0, *X, In, NAME_None);

		FastForwardSubsystem->StartFastForward(Route);

		const UFlowNode* InstanceSubGraph = GetInstanceNode(RootInstance.Get(), *SubGraph);
		const UFlowFastForwardTestNode* InstanceX = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *X));
		if (!TestNotNull(TEXT("SubGraph node"), InstanceSubGraph) || !TestNotNull(TEXT("Target node"), InstanceX))
		{
			return;
		}

		TestEqual(TEXT("SubGraph finished by its graph"), InstanceSubGraph->GetActivationState(), EFlowNodeState::Completed);
		TestEqual(TEXT("Target node executed"), InstanceX->ExecuteInputCount, 1);
		TestEqual(TEXT("Target node active"), InstanceX->GetActivationState(), EFlowNodeState::Active);
		TestFalse(TEXT("Fast Forward finished"), FastForwardSubsystem->IsFastForwarding());
	});

	It("leaves a graph through the Custom Output", [this, In, Out]()
	{
		const FName EventName = TEXT("Done");

		UFlowNode_Start* ChildStart = AddNode<UFlowNode_Start>(*ChildTemplate.Get());
		UFlowNode_CustomOutput* CustomOutput = AddNode<UFlowNode_CustomOutput>(*ChildTemplate.Get());
		CustomOutput->SetEventName(EventName);
		Connect(*ChildStart, *CustomOutput);

		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		AddOutputPin(*SubGraph, EventName);
		ConnectPins(*A, Out, *SubGraph, UFlowNode_SubGraph::StartPin.PinName);
		ConnectPins(*SubGraph, EventName, *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		if (StartFastForwardToNode(GetInstanceNode(RootInstance.Get(), *X)))
		{
			const UFlowFastForwardTestNode* InstanceX = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *X));
			TestTrue(TEXT("Target node executed"), InstanceX && InstanceX->ExecuteInputCount == 1);
			TestFalse(TEXT("Fast Forward finished"), FastForwardSubsystem->IsFastForwarding());
		}
	});

	It("doesn't pass through a disabled node", [this]()
	{
		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* B = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		B->SetTestSignalMode(EFlowSignalMode::Disabled);
		Connect(*A, *B);
		Connect(*B, *X);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), Route), EFlowFastForwardRouteResult::NoActiveNodeOnRoute);
	});

	It("triggers the Branch output required by the route", [this, In, Out]()
	{
		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_Branch* Branch = AddNode<UFlowNode_Branch>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* Y = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		ConnectPins(*A, Out, *Branch, TEXT("Evaluate"));
		ConnectPins(*Branch, TEXT("False"), *X, In);
		ConnectPins(*Branch, TEXT("True"), *Y, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		// Branch without predicates evaluates to True, so the route forces the other output
		if (StartFastForwardToNode(GetInstanceNode(RootInstance.Get(), *X)))
		{
			const UFlowFastForwardTestNode* InstanceX = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *X));
			const UFlowFastForwardTestNode* InstanceY = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *Y));
			TestTrue(TEXT("Route output triggered"), InstanceX && InstanceX->ExecuteInputCount == 1);
			TestTrue(TEXT("Evaluated output not triggered"), InstanceY && InstanceY->ExecuteInputCount == 0);
		}
	});

	It("triggers the MultiGate output required by the route", [this, In, Out]()
	{
		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_ExecutionMultiGate* MultiGate = AddNode<UFlowNode_ExecutionMultiGate>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* Y = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		ConnectPins(*A, Out, *MultiGate, In);
		ConnectPins(*MultiGate, TEXT("0"), *Y, In);
		ConnectPins(*MultiGate, TEXT("1"), *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		// Gate would open its first output, the route requires the second one
		if (StartFastForwardToNode(GetInstanceNode(RootInstance.Get(), *X)))
		{
			const UFlowFastForwardTestNode* InstanceX = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *X));
			const UFlowFastForwardTestNode* InstanceY = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *Y));
			TestTrue(TEXT("Route output triggered"), InstanceX && InstanceX->ExecuteInputCount == 1);
			TestTrue(TEXT("Next output in order not triggered"), InstanceY && InstanceY->ExecuteInputCount == 0);
		}
	});

	It("fires every Timer step before completing", [this, In, Out]()
	{
		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_Timer* Timer = AddNode<UFlowNode_Timer>(*RootTemplate.Get());
		UFlowFastForwardTestNode* StepListener = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		SetProperty<float, FFloatProperty>(*Timer, TEXT("CompletionTime"), 1.0f);
		SetProperty<float, FFloatProperty>(*Timer, TEXT("StepTime"), 0.25f);
		ConnectPins(*A, Out, *Timer, In);
		ConnectPins(*Timer, TEXT("Step"), *StepListener, In);
		ConnectPins(*Timer, TEXT("Completed"), *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		if (StartFastForwardToNode(GetInstanceNode(RootInstance.Get(), *X)))
		{
			const UFlowNode* InstanceTimer = GetInstanceNode(RootInstance.Get(), *Timer);
			const UFlowFastForwardTestNode* InstanceStepListener = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *StepListener));
			const UFlowFastForwardTestNode* InstanceX = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *X));
			TestTrue(TEXT("Steps before completion"), InstanceStepListener && InstanceStepListener->ExecuteInputCount == 3);
			TestTrue(TEXT("Timer completed"), InstanceTimer && InstanceTimer->GetActivationState() == EFlowNodeState::Completed);
			TestTrue(TEXT("Target node executed"), InstanceX && InstanceX->ExecuteInputCount == 1);
		}
	});

	It("receives the observed event directly without its source", [this, In, Out]()
	{
		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_OnActorRegistered* Observer = AddNode<UFlowNode_OnActorRegistered>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		SetProperty<FGameplayTagContainer, FStructProperty>(*Observer, TEXT("IdentityTags"), FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("Flow.NodeStyle.Node.Condition"))));
		ConnectPins(*A, Out, *Observer, TEXT("Start"));
		ConnectPins(*Observer, TEXT("Success"), *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		// No actor has the identity tag, so nothing could send the event
		if (StartFastForwardToNode(GetInstanceNode(RootInstance.Get(), *X)))
		{
			const UFlowNode* InstanceObserver = GetInstanceNode(RootInstance.Get(), *Observer);
			const UFlowFastForwardTestNode* InstanceX = Cast<UFlowFastForwardTestNode>(GetInstanceNode(RootInstance.Get(), *X));
			TestTrue(TEXT("Target node executed"), InstanceX && InstanceX->ExecuteInputCount == 1);
			TestTrue(TEXT("Observer completed by reaching Success Limit"), InstanceObserver && InstanceObserver->GetActivationState() == EFlowNodeState::Completed);
		}
	});

	It("reports a signal from outside the route and a stalled route", [this]()
	{
		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardWaitingTestNode* W = AddNode<UFlowFastForwardWaitingTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* B = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		Connect(*A, *W);
		Connect(*W, *B);
		Connect(*B, *X);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		AddExpectedMessagePlain(TEXT("from outside the route"), ELogVerbosity::Error);
		AddExpectedMessagePlain(TEXT("hasn't reached the next node"), ELogVerbosity::Error);

		if (StartFastForwardToNode(GetInstanceNode(RootInstance.Get(), *X)))
		{
			TestTrue(TEXT("Fast Forward waits for the waiting node"), FastForwardSubsystem->IsFastForwarding());

			RootInstance->TriggerInput(X->GetGuid(), UFlowNode::DefaultInputPin.PinName, FConnectedPin());

			// Timer set before the first tick starts counting after that tick, and the timer manager ticks once per frame
			const uint64 CachedFrameCounter = GFrameCounter;
			GameInstance->GetTimerManager().Tick(0.0f);
			++GFrameCounter;
			GameInstance->GetTimerManager().Tick(6.0f);
			GFrameCounter = CachedFrameCounter;

			TestTrue(TEXT("Fast Forward keeps waiting"), FastForwardSubsystem->IsFastForwarding());
		}
	});

	It("finds the route through any output leading to the target", [this, In, Out]()
	{
		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_Timer* Timer = AddNode<UFlowNode_Timer>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		ConnectPins(*A, Out, *Timer, TEXT("Skip"));
		ConnectPins(*Timer, TEXT("Completed"), *X, In);
		ConnectPins(*Timer, TEXT("Skipped"), *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), Route), EFlowFastForwardRouteResult::Found);
		TestStep(TEXT("Timer"), Route, 0, 1, *Timer, TEXT("Skip"), TEXT("Skipped"));
	});

	It("doesn't start the route by an output requiring another input", [this, In, Out]()
	{
		UFlowFastForwardTestNode* B = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_Timer* Timer = AddNode<UFlowNode_Timer>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		ConnectPins(*B, Out, *Timer, TEXT("Skip"));
		ConnectPins(*Timer, TEXT("Skipped"), *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *B, EFlowNodeState::Active);
		SetInstanceNodeState(RootInstance.Get(), *Timer, EFlowNodeState::Active);

		// Active timer triggers Skipped only after receiving Skip
		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), Route), EFlowFastForwardRouteResult::Found);
		TestStep(TEXT("Active node sending Skip"), Route, 0, 0, *B, NAME_None, Out);
		TestStep(TEXT("Timer"), Route, 0, 1, *Timer, TEXT("Skip"), TEXT("Skipped"));
	});

	It("passes through a SubGraph in PassThrough mode without entering its graph", [this, In, Out]()
	{
		UFlowNode_Start* ChildStart = AddNode<UFlowNode_Start>(*ChildTemplate.Get());
		UFlowNode_Finish* ChildFinish = AddNode<UFlowNode_Finish>(*ChildTemplate.Get());
		Connect(*ChildStart, *ChildFinish);

		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_SubGraph* SubGraph = AddSubGraphNode(*RootTemplate.Get(), *ChildTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		SetProperty<EFlowSignalMode, FEnumProperty>(*SubGraph, TEXT("SignalMode"), EFlowSignalMode::PassThrough);
		ConnectPins(*A, Out, *SubGraph, UFlowNode_SubGraph::StartPin.PinName);
		ConnectPins(*SubGraph, UFlowNode_SubGraph::FinishPin.PinName, *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), Route), EFlowFastForwardRouteResult::Found);
		TestEqual(TEXT("Segments"), Route.Segments.Num(), 1);
		TestStep(TEXT("SubGraph node"), Route, 0, 1, *SubGraph, UFlowNode_SubGraph::StartPin.PinName, UFlowNode_SubGraph::FinishPin.PinName);
	});

	It("doesn't route through an already opened MultiGate output", [this, In, Out]()
	{
		UFlowFastForwardTestNode* A = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		UFlowNode_ExecutionMultiGate* MultiGate = AddNode<UFlowNode_ExecutionMultiGate>(*RootTemplate.Get());
		UFlowFastForwardTestNode* X = AddNode<UFlowFastForwardTestNode>(*RootTemplate.Get());
		ConnectPins(*A, Out, *MultiGate, In);
		ConnectPins(*MultiGate, TEXT("0"), *X, In);

		CreateRootInstance();
		SetInstanceNodeState(RootInstance.Get(), *A, EFlowNodeState::Active);
		if (UFlowNode* InstanceMultiGate = GetInstanceNode(RootInstance.Get(), *MultiGate))
		{
			SetProperty<TArray<bool>, FArrayProperty>(*InstanceMultiGate, TEXT("Completed"), TArray<bool>{true, false});
		}

		FFlowFastForwardRoute Route;
		TestEqual(TEXT("Result"), FindRoute(GetInstanceNode(RootInstance.Get(), *X), Route), EFlowFastForwardRouteResult::NoActiveNodeOnRoute);
	});
}

#endif
