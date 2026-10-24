// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Containers/Set.h"
#include "Engine/EngineTypes.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "FlowInjectComponentsManager.generated.h"

class AActor;
class UActorComponent;

/**
 * Record of one component injected onto an actor, together with the objects currently using it
 */
USTRUCT()
struct FLOW_API FFlowInjectedComponent
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<AActor> Actor;

	UPROPERTY(Transient)
	TObjectPtr<UActorComponent> Component;

	/* Objects using the component. Weak, so a user that is garbage collected stops counting. */
	TSet<TWeakObjectPtr<UObject>, TWeakObjectPtrSetKeyFuncs<TWeakObjectPtr<UObject>>> Users;
};

/**
 * Inject components onto actors and will remove them when they are destroyed (or this is shutdown).
 */
UCLASS(MinimalAPI)
class UFlowInjectComponentsManager : public UObject
{
	GENERATED_BODY()

public:
	FLOW_API void InitializeRuntime() const;
	FLOW_API void ShutdownRuntime();

	/** Injects the component onto the actor, with this manager as its user. See the overload taking a User. */
	FLOW_API void InjectComponentOnActor(AActor& Actor, UActorComponent& ComponentInstance);

	/**
	 * Injects the component onto the actor and registers User as one of its users.
	 * A null User registers this manager instead. Injecting a component that is already tracked only adds the user;
	 * the component keeps the actor it was first injected on.
	 */
	FLOW_API void InjectComponentOnActor(AActor& Actor, UActorComponent& ComponentInstance, UObject* User);

	/** Injects each valid component onto the actor with the same User, as InjectComponentOnActor does. */
	FLOW_API void InjectComponentsOnActor(AActor& Actor, const TArray<UActorComponent*>& ComponentInstances, UObject* User = nullptr);

	/** Destroys every component injected onto the actor, whatever its users, and stops monitoring the actor. */
	FLOW_API void RemoveAllInjectedComponentsAndStopMonitoringActor(AActor& Actor);

	/** Registers User for a component this manager injected. Ignored for components this manager does not track. */
	FLOW_API void RegisterComponentUser(UObject& User, const UActorComponent& Component);

	/**
	 * Releases User from a component this manager injected. When no valid user remains, the component is destroyed
	 * before this returns, regardless of bRemoveInjectedComponentsWhenDeinitializing. Ignored if User is not registered.
	 */
	FLOW_API void UnregisterComponentUser(UObject& User, const UActorComponent& Component);

	/**
	 * Releases User from every component this manager injected, destroying those left without a valid user.
	 * Also removes components whose users were all garbage collected without unregistering, and logs a warning.
	 */
	FLOW_API void UnregisterAllComponentUsers(UObject& User);

	/** Returns the number of valid users registered for the component, or 0 if this manager does not track it. */
	FLOW_API int32 GetComponentUserCount(const UActorComponent* Component) const;

	/** Returns the number of components this manager currently tracks. */
	FLOW_API int32 GetInjectedComponentCount() const;

protected:
	FLOW_API void RegisterOnDestroyedDelegate(AActor& Actor);
	FLOW_API void UnregisterOnDestroyedDelegate(AActor& Actor);

	FLOW_API void RemoveInjectedComponents();

	UFUNCTION()
	FLOW_API void OnActorDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	FLOW_API void OnActorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlowBeforeOnActorRemoved, AActor*, SpawnedActor);

	/* Broadcast once per actor when this manager stops tracking it: after the actor's last injected component has
	 * been destroyed and the manager has stopped listening to the actor's destroy and end-of-play events.
	 * Subscribers therefore never see injected components on that actor. */
	UPROPERTY(BlueprintAssignable)
	FFlowBeforeOnActorRemoved BeforeActorRemovedDelegate;

	/* Destroy the remaining injected components when ShutdownRuntime is called. Last-user release and actor
	 * destruction or end of play always destroy the affected components, whatever this is set to. */
	UPROPERTY()
	bool bRemoveInjectedComponentsWhenDeinitializing = true;

private:
	/* Container for injected component instances */
	UPROPERTY(Transient)
	TArray<FFlowInjectedComponent> InjectedComponents;

	int32 FindComponentIndex(const UActorComponent* Component) const;
	void UnregisterComponentUser(UObject& User, const int32 ComponentIndex);
	static void PruneCollectedUsers(FFlowInjectedComponent& Entry);
	void RemoveComponent(const int32 ComponentIndex);
	void ReleaseRemovedEntries(const TConstArrayView<FFlowInjectedComponent> RemovedEntries);
	bool IsActorTracked(const AActor& Actor) const;
};
