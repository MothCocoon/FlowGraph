// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Object.h"

#include "FlowInjectComponentsManager.generated.h"

class UActorComponent;
class UFlowNodeBase;

// Container for injected component instances
USTRUCT()
struct FLOW_API FFlowComponentInstances
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UActorComponent>> Components;
};

// Inject components onto actors and will remove them when they are destroyed (or this is shutdown)
UCLASS(MinimalAPI)
class UFlowInjectComponentsManager : public UObject
{
	GENERATED_BODY()

public:

	FLOW_API void InitializeRuntime();
	FLOW_API void ShutdownRuntime();
	
	FLOW_API FORCEINLINE void InjectComponentOnActor(AActor& Actor, UActorComponent& ComponentInstance) { AddAndRegisterComponent(Actor, ComponentInstance); }
	FLOW_API void InjectComponentsOnActor(AActor& Actor, const TArray<UActorComponent*>& ComponentInstances);

	FLOW_API void RemoveAllInjectedComponentsAndStopMonitoringActor(AActor& Actor);

protected:

	FLOW_API void AddAndRegisterComponent(AActor& Actor, UActorComponent& ComponentInstance);
	FLOW_API void RemoveAndUnregisterComponent(AActor& Actor, UActorComponent& ComponentInstance);

	FLOW_API void RegisterOnDestroyedDelegate(AActor& Actor);
	FLOW_API void UnregisterOnDestroyedDelegate(AActor& Actor);

	FLOW_API void RemoveInjectedComponents();

	UFUNCTION()
	FLOW_API void OnActorDestroyed(AActor* DestroyedActor);

public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlowBeforeOnActorRemoved, AActor*, SpawnedActor);

	UPROPERTY(BlueprintAssignable)
	FFlowBeforeOnActorRemoved BeforeActorRemovedDelegate;

	// Remove the Injected Components from the Actors when Deinitialized
	UPROPERTY()
	bool bRemoveInjectedComponentsWhenDeinitializing = true;

	// Map of spawned components (if we are cleaning up)
	UPROPERTY(Transient)
	TMap<TObjectPtr<AActor>, FFlowComponentInstances> ActorToComponentsMap;
};
