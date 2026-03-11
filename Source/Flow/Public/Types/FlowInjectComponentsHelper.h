// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "FlowInjectComponentsHelper.generated.h"

class AActor;
class UActorComponent;

/**
 * Configuration helper struct for injecting components onto actors.
 */
USTRUCT()
struct FFlowInjectComponentsHelper
{
	GENERATED_BODY()

public:
	FLOW_API TArray<UActorComponent*> CreateComponentInstancesForActor(AActor& Actor);

	/* Static functions to create a component for injection. */
	static FLOW_API UActorComponent* TryCreateComponentInstanceForActorFromTemplate(AActor& Actor, UActorComponent& ComponentTemplate);
	static FLOW_API UActorComponent* TryCreateComponentInstanceForActorFromClass(AActor& Actor, TSubclassOf<UActorComponent> ComponentClass, const FName& InstanceBaseName);

	/* After creating using one of the above two functions, inject into the actor. */
	static FLOW_API void InjectCreatedComponent(AActor& Actor, UActorComponent& ComponentInstance);

	/* Remove & Destroy the injected component. */
	static FLOW_API void DestroyInjectedComponent(AActor& Actor, UActorComponent& ComponentInstance);

public:
	/* Component (template) to inject on the spawned actor. */
	UPROPERTY(EditAnywhere, Instanced, Category = Configuration)
	TArray<TObjectPtr<UActorComponent>> ComponentTemplates;

	/* Component (template) to inject on the spawned actor. */
	UPROPERTY(EditAnywhere, Category = Configuration)
	TArray<TSubclassOf<UActorComponent>> ComponentClasses;
};
