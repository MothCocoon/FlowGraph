// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FlowAsset.h"
#include "FlowSharedComponentTestTypes.h"
#include "FlowSubsystem.h"
#include "GameFramework/Actor.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Types/FlowInjectComponentsManager.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"
#include "UObject/WeakObjectPtrTemplates.h"

namespace FlowSharedComponentManagerTests
{
	void AddNodeToTemplate(UFlowAsset& Template, const FGuid& NodeGuid, UFlowNode* Node)
	{
		const FMapProperty* NodesProperty = FindFProperty<FMapProperty>(UFlowAsset::StaticClass(), TEXT("Nodes"));
		TMap<FGuid, TObjectPtr<UFlowNode>>& Nodes =
			*NodesProperty->ContainerPtrToValuePtr<TMap<FGuid, TObjectPtr<UFlowNode>>>(&Template);
		Nodes.Add(NodeGuid, Node);
	}
}

struct FFlowSharedComponentGameInstanceAccess : UGameInstance
{
	static void SetWorldContext(UGameInstance& Instance, FWorldContext* Context)
	{
		FWorldContext* UGameInstance::* ContextMember = &FFlowSharedComponentGameInstanceAccess::WorldContext;
		Instance.*ContextMember = Context;
	}
};

BEGIN_DEFINE_SPEC(FFlowSharedComponentManagerSpec, "FlowEditor.SharedComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TStrongObjectPtr<UGameInstance> GameInstance;
	TStrongObjectPtr<UFlowSubsystem> Subsystem;
	TStrongObjectPtr<UFlowAsset> Template;
	TStrongObjectPtr<UFlowAsset> Root;
	TStrongObjectPtr<UFlowInjectComponentsManager> Manager;
	TStrongObjectPtr<UObject> FirstUser;
	TStrongObjectPtr<UObject> SecondUser;
	UWorld* World = nullptr;
	AActor* Actor = nullptr;
	FNativeFlowAssetEvent PreviousAdded;
	FNativeFlowAssetEvent PreviousRemoved;
	UFlowSharedComponentTestComponent* Inject(UObject* User, UFlowAsset* Flow = nullptr);
	static bool HasInjectionManagerWithOuter(UObject& Outer);
	static UFlowInjectComponentsManager* GetAssignedInjectionManager(const UFlowAsset& Flow);
END_DEFINE_SPEC(FFlowSharedComponentManagerSpec)

bool FFlowSharedComponentManagerSpec::HasInjectionManagerWithOuter(UObject& Outer)
{
	bool bFound = false;
	const auto CheckObject = [&bFound](UObject* Object)
	{
		bFound |= Object->IsA<UFlowInjectComponentsManager>();
	};
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 8
	constexpr bool bIncludeNestedObjects = false;
	ForEachObjectWithOuter(&Outer, CheckObject, bIncludeNestedObjects);
#else
	ForEachObjectWithOuter(&Outer, CheckObject, EGetObjectsFlags::None);
#endif
	return bFound;
}

UFlowInjectComponentsManager* FFlowSharedComponentManagerSpec::GetAssignedInjectionManager(const UFlowAsset& Flow)
{
	const FObjectProperty* Property = FindFProperty<FObjectProperty>(UFlowAsset::StaticClass(), TEXT("ComponentInjector"));
	return Cast<UFlowInjectComponentsManager>(Property->GetObjectPropertyValue_InContainer(&Flow));
}

UFlowSharedComponentTestComponent* FFlowSharedComponentManagerSpec::Inject(UObject* User, UFlowAsset* Flow)
{
	UFlowSharedComponentTestComponent* Component = NewObject<UFlowSharedComponentTestComponent>(Actor);
	Component->ComponentTags.Add(TEXT("PreservedState"));
	(Flow ? Flow : Root.Get())->GetOrCreateComponentInjectionManagerInRootFlow()->InjectComponentOnActor(*Actor, *Component, User);
	TestTrue(TEXT("Injection registered"), Component->IsRegistered());
	return Component;
}

void FFlowSharedComponentManagerSpec::Define()
{
	BeforeEach([this]()
	{
		PreviousAdded = UFlowSubsystem::OnInstancedTemplateAdded;
		PreviousRemoved = UFlowSubsystem::OnInstancedTemplateRemoved;
		UFlowSubsystem::OnInstancedTemplateAdded.Unbind();
		UFlowSubsystem::OnInstancedTemplateRemoved.Unbind();
		GameInstance.Reset(NewObject<UGameInstance>(GEngine));
		World = UWorld::CreateWorld(EWorldType::Game, false);
		World->SetGameInstance(GameInstance.Get());
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.OwningGameInstance = GameInstance.Get();
		Context.SetCurrentWorld(World);
		FFlowSharedComponentGameInstanceAccess::SetWorldContext(*GameInstance.Get(), &Context);
		Subsystem.Reset(NewObject<UFlowSubsystem>(GameInstance.Get()));
		Actor = World->SpawnActor<AActor>();
		Template.Reset(NewObject<UFlowAsset>());
		Root.Reset(Subsystem->CreateFlowInstance(Actor, Template.Get()));
		Manager.Reset(Root->GetOrCreateComponentInjectionManager());
		FirstUser.Reset(NewObject<UFlowSharedComponentTestObserver>());
		SecondUser.Reset(NewObject<UFlowSharedComponentTestObserver>());
	});

	AfterEach([this]()
	{
		Subsystem->Deinitialize();
		UFlowSubsystem::OnInstancedTemplateAdded = PreviousAdded;
		UFlowSubsystem::OnInstancedTemplateRemoved = PreviousRemoved;
		FFlowSharedComponentGameInstanceAccess::SetWorldContext(*GameInstance.Get(), nullptr);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		FirstUser.Reset();
		SecondUser.Reset();
		Manager.Reset();
		Root.Reset();
		Template.Reset();
		Subsystem.Reset();
		GameInstance.Reset();
	});

	It("injects on the root actor and keeps the instance until its last user unregisters", [this]()
	{
		TestEqual(TEXT("No injection before request"), Manager->GetInjectedComponentCount(), 0);
		UFlowSharedComponentTestComponent* Component = Inject(FirstUser.Get());
		TestEqual(TEXT("Root actor is the target"), Component->GetOwner(), Actor);
		TestEqual(TEXT("Injection automatically registers its user"), Manager->GetComponentUserCount(Component), 1);
		Manager->RegisterComponentUser(*SecondUser.Get(), *Component);
		Manager->RegisterComponentUser(*FirstUser.Get(), *Component);
		TestEqual(TEXT("Unique user registrations"), Manager->GetComponentUserCount(Component), 2);
		Manager->UnregisterComponentUser(*FirstUser.Get(), *Component);
		TestTrue(TEXT("Component outlives its first user"), IsValid(Component));
		TestTrue(TEXT("Shared state preserved"), Component->ComponentTags.Contains(TEXT("PreservedState")));
		Manager->UnregisterComponentUser(*SecondUser.Get(), *Component);
		TestFalse(TEXT("Last release destroys the component before returning"), IsValid(Component));
		TestEqual(TEXT("Destroyed exactly once"), Component->DestructionCount, 1);
		TestEqual(TEXT("Finished entry removed"), Manager->GetInjectedComponentCount(), 0);
	});

	It("subscribes to exact component instances without class or configuration identities", [this]()
	{
		UActorComponent* First = Inject(FirstUser.Get());
		UActorComponent* Second = Inject(FirstUser.Get());
		TestTrue(TEXT("Same-class injections remain distinct"), First != Second);
		Manager->RegisterComponentUser(*SecondUser.Get(), *First);
		TestEqual(TEXT("First instance users"), Manager->GetComponentUserCount(First), 2);
		TestEqual(TEXT("Second instance users"), Manager->GetComponentUserCount(Second), 1);
		Manager->UnregisterComponentUser(*FirstUser.Get(), *Second);
		TestFalse(TEXT("Second instance independently released"), IsValid(Second));
		TestTrue(TEXT("First instance preserved"), IsValid(First));
	});

	It("ignores users of components it did not inject", [this]()
	{
		USceneComponent* Existing = NewObject<USceneComponent>(Actor);
		Existing->RegisterComponent();
		Manager->RegisterComponentUser(*FirstUser.Get(), *Existing);
		TestEqual(TEXT("External component has no user record"), Manager->GetComponentUserCount(Existing), 0);
		TestEqual(TEXT("External component is not tracked"), Manager->GetInjectedComponentCount(), 0);
		Manager->UnregisterComponentUser(*FirstUser.Get(), *Existing);
		Root->DeinitializeInstance();
		TestTrue(TEXT("External component remains registered"), Existing->IsRegistered());
	});

	It("uses the manager as the user when omitted or null", [this]()
	{
		UFlowSharedComponentTestComponent* Component = NewObject<UFlowSharedComponentTestComponent>(Actor);
		Manager->InjectComponentOnActor(*Actor, *Component);
		Manager->InjectComponentOnActor(*Actor, *Component, nullptr);
		TestEqual(TEXT("Both overloads register the same manager user"), Manager->GetComponentUserCount(Component), 1);
		TestEqual(TEXT("Repeated injection does not register the component again"), Component->RegistrationCount, 1);
		Manager->RegisterComponentUser(*FirstUser.Get(), *Component);
		Manager->UnregisterComponentUser(*Manager.Get(), *Component);
		TestTrue(TEXT("Explicit user keeps the component after the manager user leaves"), IsValid(Component));
		Manager->UnregisterComponentUser(*FirstUser.Get(), *Component);
		TestFalse(TEXT("Final explicit user removes the component"), IsValid(Component));
	});

	It("adds users through injection without reinjecting the same component", [this]()
	{
		UFlowSharedComponentTestComponent* Component = Inject(FirstUser.Get());
		Manager->InjectComponentOnActor(*Actor, *Component, SecondUser.Get());
		Manager->InjectComponentOnActor(*Actor, *Component, FirstUser.Get());
		TestEqual(TEXT("Both users registered"), Manager->GetComponentUserCount(Component), 2);
		TestEqual(TEXT("One injected component"), Manager->GetInjectedComponentCount(), 1);
		TestEqual(TEXT("Component registered once"), Component->RegistrationCount, 1);
	});

	It("applies the removal flag only to manager shutdown", [this]()
	{
		Manager->bRemoveInjectedComponentsWhenDeinitializing = false;
		UActorComponent* ExplicitUserComponent = Inject(FirstUser.Get());
		TestEqual(TEXT("Injection tracks its user"), Manager->GetComponentUserCount(ExplicitUserComponent), 1);
		Manager->UnregisterComponentUser(*FirstUser.Get(), *ExplicitUserComponent);
		TestFalse(TEXT("Last release still destroys component immediately"), IsValid(ExplicitUserComponent));
		TestEqual(TEXT("Last release clears bookkeeping"), Manager->GetInjectedComponentCount(), 0);
		UActorComponent* DefaultUserComponent = Inject(nullptr);
		UActorComponent* OutstandingUserComponent = Inject(SecondUser.Get());
		Manager->ShutdownRuntime();
		TestTrue(TEXT("Shutdown preserves default-user component"), DefaultUserComponent->IsRegistered());
		TestTrue(TEXT("Shutdown preserves explicit-user component"), OutstandingUserComponent->IsRegistered());
		TestEqual(TEXT("Shutdown clears bookkeeping"), Manager->GetInjectedComponentCount(), 0);
	});

	It("registers batch injection users consistently with single injection", [this]()
	{
		USceneComponent* First = NewObject<USceneComponent>(Actor);
		USceneComponent* Second = NewObject<USceneComponent>(Actor);
		Manager->InjectComponentsOnActor(*Actor, {First, Second}, FirstUser.Get());
		TestEqual(TEXT("First automatically registers user"), Manager->GetComponentUserCount(First), 1);
		TestEqual(TEXT("Second automatically registers user"), Manager->GetComponentUserCount(Second), 1);
		Manager->UnregisterAllComponentUsers(*FirstUser.Get());
		TestFalse(TEXT("First released immediately"), IsValid(First));
		TestFalse(TEXT("Second released immediately"), IsValid(Second));
	});

	It("ignores unregistering an object that did not subscribe", [this]()
	{
		UActorComponent* Component = Inject(FirstUser.Get());
		Manager->UnregisterComponentUser(*SecondUser.Get(), *Component);
		TestEqual(TEXT("Original registration remains"), Manager->GetComponentUserCount(Component), 1);
		TestTrue(TEXT("Original user preserves injection"), IsValid(Component));
	});

	It("uses itself as the root for initialized root flows", [this]()
	{
		TestNull(TEXT("Uninitialized template has no runtime root"), Template->GetOwningRootFlowInstance());
		TestEqual(TEXT("Root instance is its own root"), Root->GetOwningRootFlowInstance(), Root.Get());
		TestEqual(TEXT("Root manager accessor returns its local manager"),
			Root->FindComponentInjectionManagerInRootFlow(), Manager.Get());
	});

	It("distinguishes a child local manager from its root manager", [this]()
	{
		UFlowAsset* Child = Subsystem->CreateFlowInstance(Actor, Template.Get(), FString(), /*bStartReplication=*/true, Root.Get());
		if (!TestNotNull(TEXT("Child created"), Child))
		{
			return;
		}
		TestNull(TEXT("Child has no manager before one is requested"), Child->FindComponentInjectionManager());
		UFlowInjectComponentsManager* Local = Child->GetOrCreateComponentInjectionManager();
		TestTrue(TEXT("Child owns a distinct manager"), Local != Manager.Get());
		TestEqual(TEXT("Local manager outer"), Local->GetOuter(), static_cast<UObject*>(Child));
		TestEqual(TEXT("Local accessor reuses its manager"), Child->GetOrCreateComponentInjectionManager(), Local);
		TestEqual(TEXT("Local lookup finds the created manager"), Child->FindComponentInjectionManager(), Local);
		TestEqual(TEXT("Root lookup returns the root manager"), Child->FindComponentInjectionManagerInRootFlow(), Manager.Get());
		TestEqual(TEXT("Root accessor returns the root manager"), Child->GetOrCreateComponentInjectionManagerInRootFlow(), Manager.Get());
		USceneComponent* LocalComponent = NewObject<USceneComponent>(Actor);
		Local->InjectComponentOnActor(*Actor, *LocalComponent, Child);
		Manager->RegisterComponentUser(*Root.Get(), *LocalComponent);
		TestEqual(TEXT("Root does not track another manager's component"), Manager->GetComponentUserCount(LocalComponent), 0);
		UActorComponent* SharedComponent = Inject(Child, Child);
		Manager->RegisterComponentUser(*Root.Get(), *SharedComponent);
		Child->DeinitializeInstance();
		TestFalse(TEXT("Child shutdown removes local component"), IsValid(LocalComponent));
		TestTrue(TEXT("Parent retains shared component"), IsValid(SharedComponent));
		TestEqual(TEXT("Only parent remains a user"), Manager->GetComponentUserCount(SharedComponent), 1);
		Root->DeinitializeInstance();
		TestFalse(TEXT("Parent shutdown removes shared component"), IsValid(SharedComponent));
	});

	It("releases node users from local and root managers during cleanup", [this]()
	{
		UFlowAsset* Child = Subsystem->CreateFlowInstance(Actor, Template.Get(), FString(), /*bStartReplication=*/true, Root.Get());
		UFlowNode_Start* Node = NewObject<UFlowNode_Start>(Child);
		USceneComponent* LocalComponent = NewObject<USceneComponent>(Actor);
		Child->GetOrCreateComponentInjectionManager()->InjectComponentOnActor(*Actor, *LocalComponent, Node);
		UActorComponent* SharedComponent = Inject(Node, Child);
		Manager->RegisterComponentUser(*Root.Get(), *SharedComponent);
		Node->Cleanup();
		TestFalse(TEXT("Local node-only injection removed"), IsValid(LocalComponent));
		TestTrue(TEXT("Root user retains shared injection"), IsValid(SharedComponent));
		TestEqual(TEXT("Node released its root registration"), Manager->GetComponentUserCount(SharedComponent), 1);
		Root->DeinitializeInstance();
		TestFalse(TEXT("Root teardown releases injection"), IsValid(SharedComponent));
	});

	It("keeps monitoring an actor until all independently used components are removed", [this]()
	{
		TStrongObjectPtr<UFlowSharedComponentTestObserver> Observer(NewObject<UFlowSharedComponentTestObserver>());
		Manager->BeforeActorRemovedDelegate.AddDynamic(Observer.Get(), &UFlowSharedComponentTestObserver::OnActorRemoved);
		UActorComponent* First = Inject(FirstUser.Get());
		UActorComponent* Second = Inject(SecondUser.Get());
		Manager->UnregisterComponentUser(*FirstUser.Get(), *First);
		TestEqual(TEXT("One injection remains"), Manager->GetInjectedComponentCount(), 1);
		TestEqual(TEXT("Actor removal not notified early"), Observer->RemovalCount, 0);
		TestFalse(TEXT("Actor has not begun play"), Actor->HasActorBegunPlay());
		TestTrue(TEXT("Actor destroyed"), Actor->Destroy());
		TestFalse(TEXT("Remaining injection removed on actor destruction"), IsValid(Second));
		TestEqual(TEXT("Actor removal notified once"), Observer->RemovalCount, 1);
		TestTrue(TEXT("Notification names the removed actor"), Observer->LastRemovedActor.Get(/*bEvenIfPendingKill=*/true) == Actor);
		TestEqual(TEXT("Actor bookkeeping removed"), Manager->GetInjectedComponentCount(), 0);
		Manager->ShutdownRuntime();
		TestEqual(TEXT("Shutdown does not repeat actor notification"), Observer->RemovalCount, 1);
	});

	It("notifies actor removal only after the last component released by its user is destroyed", [this]()
	{
		TStrongObjectPtr<UFlowSharedComponentTestObserver> Observer(NewObject<UFlowSharedComponentTestObserver>());
		UActorComponent* First = Inject(FirstUser.Get());
		UActorComponent* Last = Inject(SecondUser.Get());
		Observer->WatchedComponent = Last;
		Manager->BeforeActorRemovedDelegate.AddDynamic(Observer.Get(), &UFlowSharedComponentTestObserver::OnActorRemoved);
		Manager->UnregisterComponentUser(*FirstUser.Get(), *First);
		TestEqual(TEXT("No notification while a component remains"), Observer->RemovalCount, 0);
		Manager->UnregisterComponentUser(*SecondUser.Get(), *Last);
		TestEqual(TEXT("Notified once"), Observer->RemovalCount, 1);
		TestFalse(TEXT("Last component was destroyed before subscribers ran"), Observer->bWatchedComponentAliveAtRemoval);
	});

	It("notifies actor removal only after all of the actor's components are destroyed in bulk", [this]()
	{
		TStrongObjectPtr<UFlowSharedComponentTestObserver> Observer(NewObject<UFlowSharedComponentTestObserver>());
		UActorComponent* First = Inject(FirstUser.Get());
		UActorComponent* Second = Inject(SecondUser.Get());
		Manager->BeforeActorRemovedDelegate.AddDynamic(Observer.Get(), &UFlowSharedComponentTestObserver::OnActorRemoved);
		// Bulk removal walks the records from the end, so First is the last record removed for the actor
		Observer->WatchedComponent = First;
		Manager->RemoveAllInjectedComponentsAndStopMonitoringActor(*Actor);
		TestEqual(TEXT("Notified once"), Observer->RemovalCount, 1);
		TestFalse(TEXT("Last component was destroyed before subscribers ran"), Observer->bWatchedComponentAliveAtRemoval);
		TestFalse(TEXT("Other component destroyed"), IsValid(Second));
	});

	It("removes one actor's records without affecting another actor", [this]()
	{
		UActorComponent* First = Inject(FirstUser.Get());
		AActor* OtherActor = World->SpawnActor<AActor>();
		USceneComponent* OtherComponent = NewObject<USceneComponent>(OtherActor);
		Manager->InjectComponentOnActor(*OtherActor, *OtherComponent, FirstUser.Get());
		UActorComponent* Second = Inject(SecondUser.Get());
		Manager->RemoveAllInjectedComponentsAndStopMonitoringActor(*Actor);
		TestFalse(TEXT("First matching record removed"), IsValid(First));
		TestFalse(TEXT("Second matching record removed"), IsValid(Second));
		TestTrue(TEXT("Other actor's component remains registered"), OtherComponent->IsRegistered());
		TestEqual(TEXT("Only the other actor's record remains"), Manager->GetInjectedComponentCount(), 1);
		TestEqual(TEXT("Other actor retains its registered user"), Manager->GetComponentUserCount(OtherComponent), 1);
		Manager->UnregisterAllComponentUsers(*FirstUser.Get());
		TestFalse(TEXT("Last remaining use removes the other component"), IsValid(OtherComponent));
		TestEqual(TEXT("All records removed"), Manager->GetInjectedComponentCount(), 0);
	});

	It("removes an actor's injections when it ends play without being destroyed", [this]()
	{
		AFlowSharedComponentTestActor* PlayingActor = World->SpawnActor<AFlowSharedComponentTestActor>();
		TStrongObjectPtr<UFlowSharedComponentTestObserver> Observer(NewObject<UFlowSharedComponentTestObserver>());
		PlayingActor->OnDestroyed.AddDynamic(Observer.Get(), &UFlowSharedComponentTestObserver::OnActorRemoved);
		USceneComponent* Component = NewObject<USceneComponent>(PlayingActor);
		Manager->InjectComponentOnActor(*PlayingActor, *Component, FirstUser.Get());
		PlayingActor->DispatchBeginPlay();
		if (!TestTrue(TEXT("Actor entered play"), PlayingActor->HasActorBegunPlay()))
		{
			return;
		}
		PlayingActor->EndPlayForTest(EEndPlayReason::LevelTransition);
		TestFalse(TEXT("Actor ended play"), PlayingActor->HasActorBegunPlay());
		TestEqual(TEXT("Actor was not destroyed"), Observer->RemovalCount, 0);
		TestFalse(TEXT("End of play removed the injection"), IsValid(Component));
		TestEqual(TEXT("End of play cleared records"), Manager->GetInjectedComponentCount(), 0);
	});

	It("never creates a manager for an instance that does not inject", [this]()
	{
		TStrongObjectPtr<UFlowAsset> NodeTemplate(NewObject<UFlowAsset>());
		FlowSharedComponentManagerTests::AddNodeToTemplate(*NodeTemplate.Get(), FGuid::NewGuid(), NewObject<UFlowNode_Start>(NodeTemplate.Get()));
		UFlowAsset* Instance = Subsystem->CreateFlowInstance(Actor, NodeTemplate.Get());
		if (!TestNotNull(TEXT("Instance created"), Instance))
		{
			return;
		}
		UFlowNode_Start* Node = NewObject<UFlowNode_Start>(Instance);
		Node->Cleanup();
		TestFalse(TEXT("Node cleanup creates no manager"), HasInjectionManagerWithOuter(*Instance));
		Instance->DeinitializeInstance();
		TestFalse(TEXT("Deinitialization creates no manager"), HasInjectionManagerWithOuter(*Instance));
	});

	It("does not create a manager on a root that has already shut down", [this]()
	{
		UFlowAsset* Child = Subsystem->CreateFlowInstance(Actor, Template.Get(), FString(), /*bStartReplication=*/true, Root.Get());
		if (!TestNotNull(TEXT("Child created"), Child))
		{
			return;
		}
		UFlowNode_Start* Node = NewObject<UFlowNode_Start>(Child);
		Root->DeinitializeInstance();
		TestNull(TEXT("Root shutdown released its manager"), GetAssignedInjectionManager(*Root));
		Node->Cleanup();
		TestNull(TEXT("Child node cleanup does not recreate the root manager"), GetAssignedInjectionManager(*Root));
		TestNull(TEXT("Child node cleanup does not create a child manager"), GetAssignedInjectionManager(*Child));
	});

	It("drops its owning root when deinitialized", [this]()
	{
		UFlowAsset* Child = Subsystem->CreateFlowInstance(Actor, Template.Get(), FString(), /*bStartReplication=*/true, Root.Get());
		if (!TestNotNull(TEXT("Child created"), Child))
		{
			return;
		}
		Child->DeinitializeInstance();
		TestNull(TEXT("Deinitialized child has no root"), Child->GetOwningRootFlowInstance());
		Root->DeinitializeInstance();
		TestNull(TEXT("Deinitialized root has no root"), Root->GetOwningRootFlowInstance());
	});

	It("uses the supplied instance as the root when it has no resolved root", [this]()
	{
		TStrongObjectPtr<UFlowAsset> Unresolved(NewObject<UFlowAsset>());
		UFlowAsset* Child = Subsystem->CreateFlowInstance(Actor, Template.Get(), FString(), /*bStartReplication=*/true, Unresolved.Get());
		if (!TestNotNull(TEXT("Child created"), Child))
		{
			return;
		}
		TestEqual(TEXT("Supplied instance becomes the root"), Child->GetOwningRootFlowInstance(), Unresolved.Get());
	});

	It("does not count or keep alive a garbage-collected user", [this]()
	{
		const TWeakObjectPtr<UFlowSharedComponentTestObserver> CollectedUser(NewObject<UFlowSharedComponentTestObserver>());
		UActorComponent* Component = Inject(CollectedUser.Get());
		Manager->RegisterComponentUser(*SecondUser.Get(), *Component);
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		TestFalse(TEXT("User was collected"), CollectedUser.IsValid());
		TestEqual(TEXT("Collected user is not counted"), Manager->GetComponentUserCount(Component), 1);
		AddExpectedMessage(TEXT("garbage collected without unregistering"), ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains, 1, false);
		Manager->UnregisterComponentUser(*SecondUser.Get(), *Component);
		TestFalse(TEXT("Collected user does not keep the component alive"), IsValid(Component));
		TestEqual(TEXT("Record removed"), Manager->GetInjectedComponentCount(), 0);
	});

	It("tolerates a notification that removes another actor's records during actor removal", [this]()
	{
		AActor* OtherActor = World->SpawnActor<AActor>();
		USceneComponent* OtherFirst = NewObject<USceneComponent>(OtherActor);
		USceneComponent* OtherSecond = NewObject<USceneComponent>(OtherActor);
		Manager->InjectComponentOnActor(*OtherActor, *OtherFirst, FirstUser.Get());
		Manager->InjectComponentOnActor(*OtherActor, *OtherSecond, FirstUser.Get());
		UActorComponent* Component = Inject(FirstUser.Get());
		TStrongObjectPtr<UFlowSharedComponentReentrantObserver> Observer(NewObject<UFlowSharedComponentReentrantObserver>());
		Observer->Manager = Manager.Get();
		Observer->TriggerActor = Actor;
		Observer->OtherActor = OtherActor;
		Manager->BeforeActorRemovedDelegate.AddDynamic(Observer.Get(), &UFlowSharedComponentReentrantObserver::OnActorRemoved);
		Manager->RemoveAllInjectedComponentsAndStopMonitoringActor(*Actor);
		TestFalse(TEXT("Triggering actor's component removed"), IsValid(Component));
		TestFalse(TEXT("Other actor's first component removed"), IsValid(OtherFirst));
		TestFalse(TEXT("Other actor's second component removed"), IsValid(OtherSecond));
		TestEqual(TEXT("No records remain"), Manager->GetInjectedComponentCount(), 0);
	});

	It("tolerates a notification that removes another actor's records during user release", [this]()
	{
		AActor* OtherActor = World->SpawnActor<AActor>();
		USceneComponent* OtherComponent = NewObject<USceneComponent>(OtherActor);
		Manager->InjectComponentOnActor(*OtherActor, *OtherComponent, SecondUser.Get());
		UActorComponent* Component = Inject(FirstUser.Get());
		TStrongObjectPtr<UFlowSharedComponentReentrantObserver> Observer(NewObject<UFlowSharedComponentReentrantObserver>());
		Observer->Manager = Manager.Get();
		Observer->TriggerActor = Actor;
		Observer->OtherActor = OtherActor;
		Manager->BeforeActorRemovedDelegate.AddDynamic(Observer.Get(), &UFlowSharedComponentReentrantObserver::OnActorRemoved);
		Manager->UnregisterAllComponentUsers(*FirstUser.Get());
		TestFalse(TEXT("Released component removed"), IsValid(Component));
		TestFalse(TEXT("Other actor's component removed by the notification"), IsValid(OtherComponent));
		TestEqual(TEXT("No records remain"), Manager->GetInjectedComponentCount(), 0);
	});

	It("removes a component whose only user was collected without unregistering on the next full release", [this]()
	{
		const TWeakObjectPtr<UFlowSharedComponentTestObserver> CollectedUser(NewObject<UFlowSharedComponentTestObserver>());
		UActorComponent* Component = Inject(CollectedUser.Get());
		UActorComponent* OtherComponent = Inject(SecondUser.Get());
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		TestFalse(TEXT("User was collected"), CollectedUser.IsValid());
		TestTrue(TEXT("Component stays until a release reaches the manager"), IsValid(Component));
		AddExpectedMessage(TEXT("garbage collected without unregistering"), ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains, 1, false);
		Manager->UnregisterAllComponentUsers(*FirstUser.Get());
		TestFalse(TEXT("Component without valid users is removed"), IsValid(Component));
		TestTrue(TEXT("Component with a valid user is kept"), IsValid(OtherComponent));
		TestEqual(TEXT("Only the valid record remains"), Manager->GetInjectedComponentCount(), 1);
	});

	It("warns and keeps the original actor when a tracked component is injected on another actor", [this]()
	{
		UActorComponent* Component = Inject(FirstUser.Get());
		AActor* OtherActor = World->SpawnActor<AActor>();
		AddExpectedMessage(TEXT("is already injected on"), ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains, 1, false);
		Manager->InjectComponentOnActor(*OtherActor, *Component, SecondUser.Get());
		TestEqual(TEXT("Second user registered"), Manager->GetComponentUserCount(Component), 2);
		Manager->RemoveAllInjectedComponentsAndStopMonitoringActor(*OtherActor);
		TestTrue(TEXT("Removing the requested actor leaves the component"), IsValid(Component));
		TestEqual(TEXT("Record stays with the original actor"), Manager->GetInjectedComponentCount(), 1);
	});

	It("releases externally destroyed components without stale users", [this]()
	{
		UActorComponent* Component = Inject(FirstUser.Get());
		Component->DestroyComponent();
		Manager->UnregisterAllComponentUsers(*FirstUser.Get());
		TestEqual(TEXT("Dead component users removed"), Manager->GetComponentUserCount(Component), 0);
		TestEqual(TEXT("Dead component bookkeeping removed"), Manager->GetInjectedComponentCount(), 0);
	});

	It("shuts down remaining users with the owning runtime scope", [this]()
	{
		UActorComponent* Component = Inject(FirstUser.Get());
		Root->DeinitializeInstance();
		TestFalse(TEXT("Root shutdown removes outstanding injections"), IsValid(Component));
		Manager->UnregisterAllComponentUsers(*FirstUser.Get());
		TestEqual(TEXT("Late release is harmless"), Manager->GetInjectedComponentCount(), 0);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
