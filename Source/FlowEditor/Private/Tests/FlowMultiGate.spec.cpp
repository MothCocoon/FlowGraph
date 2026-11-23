// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FlowAsset.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"
#include "Nodes/Route/FlowNode_ExecutionMultiGate.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#include "FlowMultiGateTestTypes.h"

struct FFlowMultiGateGameInstanceAccess : UGameInstance
{
	static void SetWorldContext(UGameInstance& Instance, FWorldContext* Context)
	{
		FWorldContext* UGameInstance::* ContextMember = &FFlowMultiGateGameInstanceAccess::WorldContext;
		Instance.*ContextMember = Context;
	}
};

BEGIN_DEFINE_SPEC(FFlowMultiGateSpec, "FlowEditor.MultiGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TStrongObjectPtr<UGameInstance> GameInstance;
	TStrongObjectPtr<UFlowSubsystem> Subsystem;
	TStrongObjectPtr<UFlowAsset> Template;
	TStrongObjectPtr<UFlowAsset> Instance;
	UWorld* World = nullptr;
	FNativeFlowAssetEvent PreviousAdded;
	FNativeFlowAssetEvent PreviousRemoved;
	bool bPreviousDeferTriggers = true;

	template <class T>
	T* AddNode() const
	{
		T* Node = NewObject<T>(Template.Get());
		Node->SetGuid(FGuid::NewGuid());

		const FMapProperty* NodesProperty = FindFProperty<FMapProperty>(UFlowAsset::StaticClass(), TEXT("Nodes"));
		NodesProperty->ContainerPtrToValuePtr<TMap<FGuid, TObjectPtr<UFlowNode>>>(Template.Get())->Add(Node->GetGuid(), Node);
		return Node;
	}

	void ConnectPins(UFlowNode& From, const FName& OutputPinName, const UFlowNode& To, const FName& InputPinName) const
	{
		const FMapProperty* ConnectionsProperty = FindFProperty<FMapProperty>(UFlowNode::StaticClass(), TEXT("Connections"));
		ConnectionsProperty->ContainerPtrToValuePtr<TMap<FName, FConnectedPin>>(&From)->Add(OutputPinName, FConnectedPin(To.GetGuid(), InputPinName));
	}
END_DEFINE_SPEC(FFlowMultiGateSpec)

void FFlowMultiGateSpec::Define()
{
	const FName In = UFlowNode::DefaultInputPin.PinName;

	BeforeEach([this]()
	{
		PreviousAdded = UFlowSubsystem::OnInstancedTemplateAdded;
		PreviousRemoved = UFlowSubsystem::OnInstancedTemplateRemoved;
		UFlowSubsystem::OnInstancedTemplateAdded.Unbind();
		UFlowSubsystem::OnInstancedTemplateRemoved.Unbind();
		bPreviousDeferTriggers = GetDefault<UFlowSettings>()->bDeferTriggeredOutputsWhileTriggering;

		GameInstance.Reset(NewObject<UGameInstance>(GEngine));
		World = UWorld::CreateWorld(EWorldType::Game, false);
		World->SetGameInstance(GameInstance.Get());
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.OwningGameInstance = GameInstance.Get();
		Context.SetCurrentWorld(World);
		FFlowMultiGateGameInstanceAccess::SetWorldContext(*GameInstance.Get(), &Context);

		Subsystem.Reset(NewObject<UFlowSubsystem>(GameInstance.Get()));
		Template.Reset(NewObject<UFlowAsset>());
	});

	AfterEach([this]()
	{
		Subsystem->AbortActiveFlows();
		if (Instance.IsValid())
		{
			Instance->FinishFlowInstance(EFlowFinishPolicy::Keep);
			Instance->DeinitializeInstance();
		}
		GetMutableDefault<UFlowSettings>()->bDeferTriggeredOutputsWhileTriggering = bPreviousDeferTriggers;

		UFlowSubsystem::OnInstancedTemplateAdded = PreviousAdded;
		UFlowSubsystem::OnInstancedTemplateRemoved = PreviousRemoved;
		FFlowMultiGateGameInstanceAccess::SetWorldContext(*GameInstance.Get(), nullptr);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);

		World = nullptr;
		Instance.Reset();
		Template.Reset();
		GameInstance.Reset();
		Subsystem.Reset();
	});

	It("starts a new loop cycle from an input triggered synchronously by the last output", [this, In]()
	{
		// Legacy triggering runs the looped input before the last output returns
		GetMutableDefault<UFlowSettings>()->bDeferTriggeredOutputsWhileTriggering = false;

		UFlowNode_ExecutionMultiGate* MultiGate = AddNode<UFlowNode_ExecutionMultiGate>();
		UFlowMultiGateTestSink* Sink = AddNode<UFlowMultiGateTestSink>();
		FindFProperty<FBoolProperty>(UFlowNode_ExecutionMultiGate::StaticClass(), TEXT("bLoop"))->SetPropertyValue_InContainer(MultiGate, true);
		ConnectPins(*MultiGate, TEXT("0"), *Sink, In);
		ConnectPins(*MultiGate, TEXT("1"), *MultiGate, In);

		Instance.Reset(Subsystem->CreateRootFlow(GameInstance.Get(), Template.Get()));
		if (!TestNotNull(TEXT("Instance"), Instance.Get()))
		{
			return;
		}

		const UFlowNode* InstanceMultiGate = Instance->GetNode(MultiGate->GetGuid());
		const UFlowMultiGateTestSink* InstanceSink = Cast<UFlowMultiGateTestSink>(Instance->GetNode(Sink->GetGuid()));
		if (!TestNotNull(TEXT("Instance MultiGate"), InstanceMultiGate) || !TestNotNull(TEXT("Instance sink"), InstanceSink))
		{
			return;
		}

		Instance->TriggerInput(MultiGate->GetGuid(), In, FConnectedPin());
		TestEqual(TEXT("Inputs received by the first output after the first cycle"), InstanceSink->ExecuteInputCount, 1);

		Instance->TriggerInput(MultiGate->GetGuid(), In, FConnectedPin());
		TestEqual(TEXT("Inputs received by the first output after the looped input"), InstanceSink->ExecuteInputCount, 2);
		TestEqual(TEXT("MultiGate state in the new cycle"), InstanceMultiGate->GetActivationState(), EFlowNodeState::Active);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
