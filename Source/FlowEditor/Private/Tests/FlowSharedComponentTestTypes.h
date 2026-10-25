// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Types/FlowInjectComponentsManager.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "FlowSharedComponentTestTypes.generated.h"

UCLASS()
class UFlowSharedComponentTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	int32 RegistrationCount = 0;
	int32 DestructionCount = 0;

protected:
	virtual void OnRegister() override
	{
		Super::OnRegister();
		++RegistrationCount;
	}

public:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override
	{
		++DestructionCount;
		Super::OnComponentDestroyed(bDestroyingHierarchy);
	}
};

UCLASS()
class UFlowSharedComponentTestObserver : public UObject
{
	GENERATED_BODY()

public:
	int32 RemovalCount = 0;
	TWeakObjectPtr<AActor> LastRemovedActor;
	TWeakObjectPtr<UActorComponent> WatchedComponent;
	bool bWatchedComponentAliveAtRemoval = false;

	UFUNCTION()
	void OnActorRemoved(AActor* Actor)
	{
		++RemovalCount;
		LastRemovedActor = Actor;
		bWatchedComponentAliveAtRemoval = IsValid(WatchedComponent.Get(/*bEvenIfPendingKill=*/true));
	}
};

/** Removes another actor's injections from inside the manager's actor-removed notification. */
UCLASS()
class UFlowSharedComponentReentrantObserver : public UObject
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UFlowInjectComponentsManager> Manager;
	TWeakObjectPtr<AActor> TriggerActor;
	TWeakObjectPtr<AActor> OtherActor;

	UFUNCTION()
	void OnActorRemoved(AActor* RemovedActor)
	{
		UFlowInjectComponentsManager* ManagerPtr = Manager.Get();
		AActor* OtherActorPtr = OtherActor.Get();
		if (RemovedActor == TriggerActor.Get() && ManagerPtr && OtherActorPtr)
		{
			ManagerPtr->RemoveAllInjectedComponentsAndStopMonitoringActor(*OtherActorPtr);
		}
	}
};

/** Exposes AActor::EndPlay, which RouteEndPlay skips for actors in a world not initialized for play. */
UCLASS()
class AFlowSharedComponentTestActor : public AActor
{
	GENERATED_BODY()

public:
	void EndPlayForTest(const EEndPlayReason::Type EndPlayReason)
	{
		EndPlay(EndPlayReason);
	}
};
