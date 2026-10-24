// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Types/FlowInjectComponentsManager.h"

#include "Types/FlowInjectComponentsHelper.h"
#include "Components/ActorComponent.h"
#include "Containers/ArrayView.h"
#include "GameFramework/Actor.h"
#include "FlowLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowInjectComponentsManager)

void UFlowInjectComponentsManager::InitializeRuntime() const
{
	check(InjectedComponents.IsEmpty());
}

void UFlowInjectComponentsManager::ShutdownRuntime()
{
	if (bRemoveInjectedComponentsWhenDeinitializing)
	{
		RemoveInjectedComponents();
	}
	for (const FFlowInjectedComponent& Entry : InjectedComponents)
	{
		if (AActor* Actor = Entry.Actor.Get(); IsValid(Actor))
		{
			UnregisterOnDestroyedDelegate(*Actor);
		}
	}
	InjectedComponents.Empty();
}

void UFlowInjectComponentsManager::InjectComponentOnActor(AActor& Actor, UActorComponent& ComponentInstance)
{
	InjectComponentOnActor(Actor, ComponentInstance, nullptr);
}

void UFlowInjectComponentsManager::InjectComponentOnActor(AActor& Actor, UActorComponent& ComponentInstance, UObject* User)
{
	UObject* ComponentUser = User != nullptr ? User : this;

	if (!IsValid(ComponentUser))
	{
		UE_LOG(LogFlow, Error, TEXT("UFlowInjectComponentsManager::InjectComponentOnActor - ComponentUser is not valid"));
		return;
	}

	const int32 ComponentIndex = FindComponentIndex(&ComponentInstance);
	if (ComponentIndex != INDEX_NONE)
	{
		const AActor* TrackedActor = InjectedComponents[ComponentIndex].Actor;
		if (TrackedActor != &Actor)
		{
			UE_LOG(LogFlow, Warning,
			       TEXT("UFlowInjectComponentsManager::InjectComponentOnActor - Component %s is already injected on %s; ignoring requested actor %s"),
			       *GetNameSafe(&ComponentInstance), *GetNameSafe(TrackedActor), *GetNameSafe(&Actor));
		}

		InjectedComponents[ComponentIndex].Users.Add(ComponentUser);
		return;
	}

	// If we will be responsible for removing them later,
	// we need to keep track of the spawned components
	FFlowInjectedComponent& Entry = InjectedComponents.AddDefaulted_GetRef();
	Entry.Actor = &Actor;
	Entry.Component = &ComponentInstance;
	Entry.Users.Add(ComponentUser);
	RegisterOnDestroyedDelegate(Actor);
	FFlowInjectComponentsHelper::InjectCreatedComponent(Actor, ComponentInstance);
}

void UFlowInjectComponentsManager::InjectComponentsOnActor(
	AActor& Actor, const TArray<UActorComponent*>& ComponentInstances, UObject* User)
{
	for (UActorComponent* ComponentInstance : ComponentInstances)
	{
		if (IsValid(ComponentInstance))
		{
			InjectComponentOnActor(Actor, *ComponentInstance, User);
		}
	}
}

void UFlowInjectComponentsManager::RemoveAllInjectedComponentsAndStopMonitoringActor(AActor& Actor)
{
	// Single pass: move the actor's entries out of the array, then release them
	TArray<FFlowInjectedComponent> ActorEntries;
	for (int32 ComponentIndex = InjectedComponents.Num() - 1; ComponentIndex >= 0; --ComponentIndex)
	{
		if (InjectedComponents[ComponentIndex].Actor == &Actor)
		{
			ActorEntries.Add(MoveTemp(InjectedComponents[ComponentIndex]));
			InjectedComponents.RemoveAtSwap(ComponentIndex, EAllowShrinking::No);
		}
	}

	ReleaseRemovedEntries(ActorEntries);
}

void UFlowInjectComponentsManager::RegisterComponentUser(UObject& User, const UActorComponent& Component)
{
	const int32 ComponentIndex = FindComponentIndex(&Component);
	if (ComponentIndex != INDEX_NONE)
	{
		InjectedComponents[ComponentIndex].Users.Add(&User);
	}
}

void UFlowInjectComponentsManager::UnregisterComponentUser(UObject& User, const UActorComponent& Component)
{
	const int32 ComponentIndex = FindComponentIndex(&Component);
	if (ComponentIndex != INDEX_NONE)
	{
		UnregisterComponentUser(User, ComponentIndex);
	}
}

void UFlowInjectComponentsManager::UnregisterAllComponentUsers(UObject& User)
{
	// Single pass: move the entries left without a valid user out of the array, then release them.
	// This also sweeps entries whose users were all garbage collected without unregistering.
	TArray<FFlowInjectedComponent> UnusedEntries;
	for (int32 ComponentIndex = InjectedComponents.Num() - 1; ComponentIndex >= 0; --ComponentIndex)
	{
		FFlowInjectedComponent& Entry = InjectedComponents[ComponentIndex];
		Entry.Users.Remove(&User);
		PruneCollectedUsers(Entry);
		if (Entry.Users.IsEmpty())
		{
			UnusedEntries.Add(MoveTemp(Entry));
			InjectedComponents.RemoveAtSwap(ComponentIndex, EAllowShrinking::No);
		}
	}

	ReleaseRemovedEntries(UnusedEntries);
}

int32 UFlowInjectComponentsManager::GetComponentUserCount(const UActorComponent* Component) const
{
	const int32 ComponentIndex = FindComponentIndex(Component);
	if (ComponentIndex == INDEX_NONE)
	{
		return 0;
	}

	int32 ValidUserCount = 0;
	for (const TWeakObjectPtr<UObject>& ComponentUser : InjectedComponents[ComponentIndex].Users)
	{
		ValidUserCount += ComponentUser.IsValid() ? 1 : 0;
	}
	return ValidUserCount;
}

int32 UFlowInjectComponentsManager::GetInjectedComponentCount() const
{
	return InjectedComponents.Num();
}

void UFlowInjectComponentsManager::RegisterOnDestroyedDelegate(AActor& Actor)
{
	Actor.OnDestroyed.AddUniqueDynamic(this, &UFlowInjectComponentsManager::OnActorDestroyed);
	Actor.OnEndPlay.AddUniqueDynamic(this, &UFlowInjectComponentsManager::OnActorEndPlay);
}

void UFlowInjectComponentsManager::UnregisterOnDestroyedDelegate(AActor& Actor)
{
	Actor.OnDestroyed.RemoveDynamic(this, &UFlowInjectComponentsManager::OnActorDestroyed);
	Actor.OnEndPlay.RemoveDynamic(this, &UFlowInjectComponentsManager::OnActorEndPlay);
}

void UFlowInjectComponentsManager::RemoveInjectedComponents()
{
	TArray<FFlowInjectedComponent> RemovedEntries = MoveTemp(InjectedComponents);
	InjectedComponents.Reset();
	ReleaseRemovedEntries(RemovedEntries);
}

void UFlowInjectComponentsManager::OnActorDestroyed(AActor* DestroyedActor)
{
	if (IsValid(DestroyedActor))
	{
		RemoveAllInjectedComponentsAndStopMonitoringActor(*DestroyedActor);
	}
}

void UFlowInjectComponentsManager::OnActorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(Actor))
	{
		RemoveAllInjectedComponentsAndStopMonitoringActor(*Actor);
	}
}

int32 UFlowInjectComponentsManager::FindComponentIndex(const UActorComponent* Component) const
{
	return InjectedComponents.IndexOfByPredicate([Component](const FFlowInjectedComponent& Entry)
	{
		return Entry.Component == Component;
	});
}

void UFlowInjectComponentsManager::UnregisterComponentUser(UObject& User, const int32 ComponentIndex)
{
	FFlowInjectedComponent& Entry = InjectedComponents[ComponentIndex];
	if (Entry.Users.Remove(&User) == 0)
	{
		return;
	}

	PruneCollectedUsers(Entry);
	if (Entry.Users.IsEmpty())
	{
		RemoveComponent(ComponentIndex);
	}
}

void UFlowInjectComponentsManager::PruneCollectedUsers(FFlowInjectedComponent& Entry)
{
	// Users that were garbage collected without releasing must not keep the component alive
	int32 CollectedUserCount = 0;
	for (auto UserIt = Entry.Users.CreateIterator(); UserIt; ++UserIt)
	{
		if (!UserIt->IsValid())
		{
			UserIt.RemoveCurrent();
			++CollectedUserCount;
		}
	}

	if (CollectedUserCount > 0)
	{
		UE_LOG(LogFlow, Warning,
		       TEXT("UFlowInjectComponentsManager - %d user(s) of component %s on %s were garbage collected without unregistering. ")
		       TEXT("Call UnregisterComponentUser or UnregisterAllComponentUsers before releasing a user; otherwise the component stays injected until another release reaches this manager."),
		       CollectedUserCount, *GetNameSafe(Entry.Component), *GetNameSafe(Entry.Actor));
	}
}

void UFlowInjectComponentsManager::RemoveComponent(const int32 ComponentIndex)
{
	const FFlowInjectedComponent Entry = MoveTemp(InjectedComponents[ComponentIndex]);
	InjectedComponents.RemoveAtSwap(ComponentIndex, EAllowShrinking::No);
	ReleaseRemovedEntries(TConstArrayView<FFlowInjectedComponent>(&Entry, 1));
}

void UFlowInjectComponentsManager::ReleaseRemovedEntries(const TConstArrayView<FFlowInjectedComponent> RemovedEntries)
{
	// The entries are already out of InjectedComponents, so code run by destruction or by the notification below
	// may change the tracked records freely.

	// Actors left without any injected record; collected before any component destruction runs other code
	TArray<AActor*, TInlineAllocator<4>> UntrackedActors;
	for (const FFlowInjectedComponent& Entry : RemovedEntries)
	{
		AActor* Actor = Entry.Actor.Get();
		if (IsValid(Actor) && !IsActorTracked(*Actor))
		{
			UntrackedActors.AddUnique(Actor);
		}
	}

	for (const FFlowInjectedComponent& Entry : RemovedEntries)
	{
		AActor* Actor = Entry.Actor.Get();
		UActorComponent* Component = Entry.Component.Get();
		if (IsValid(Actor) && IsValid(Component))
		{
			FFlowInjectComponentsHelper::DestroyInjectedComponent(*Actor, *Component);
		}
	}

	// Notify once per actor, after all of its injected components are gone,
	// unless something injected onto the actor again while the components were destroyed
	for (AActor* Actor : UntrackedActors)
	{
		if (IsValid(Actor) && !IsActorTracked(*Actor))
		{
			UnregisterOnDestroyedDelegate(*Actor);
			BeforeActorRemovedDelegate.Broadcast(Actor);
		}
	}
}

bool UFlowInjectComponentsManager::IsActorTracked(const AActor& Actor) const
{
	return InjectedComponents.ContainsByPredicate([&Actor](const FFlowInjectedComponent& Entry)
	{
		return Entry.Actor == &Actor;
	});
}
