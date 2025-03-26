// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowInjectComponentsHelper.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "FlowLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowInjectComponentsHelper)

TArray<UActorComponent*> FFlowInjectComponentsHelper::CreateComponentInstancesForActor(AActor& Actor)
{
	TArray<UActorComponent*> ComponentInstances;

	if (ComponentTemplates.IsEmpty() && ComponentClasses.IsEmpty())
	{
		return ComponentInstances;
	}

	// If the desired component does not already exist, add it to the ActorOwner
	for (UActorComponent* ComponentTemplate : ComponentTemplates)
	{
		if (!IsValid(ComponentTemplate))
		{
			UE_LOG(LogFlow, Warning, TEXT("Cannot inject a null component!"));

			continue;
		}

		if (UActorComponent* ComponentInstance = TryCreateComponentInstanceForActorFromTemplate(Actor, *ComponentTemplate))
		{
			ComponentInstances.Add(ComponentInstance);
		}
	}

	for (TSubclassOf<UActorComponent> ComponentClass : ComponentClasses)
	{
		if (!IsValid(ComponentClass))
		{
			UE_LOG(LogFlow, Warning, TEXT("Cannot inject a null component class!"));

			continue;
		}

		const FName InstanceBaseName = ComponentClass->GetFName();
		if (UActorComponent* ComponentInstance = TryCreateComponentInstanceForActorFromClass(Actor, ComponentClass, InstanceBaseName))
		{
			ComponentInstances.Add(ComponentInstance);
		}
	}

	return ComponentInstances;
}

UActorComponent* FFlowInjectComponentsHelper::TryCreateComponentInstanceForActorFromTemplate(AActor& Actor, UActorComponent& ComponentTemplate)
{
	// Following pattern from UGameFrameworkComponentManager::CreateComponentOnInstance()
	UClass* ComponentClass = ComponentTemplate.GetClass();
	if (!ComponentClass->GetDefaultObject<UActorComponent>()->GetIsReplicated() || Actor.GetLocalRole() == ROLE_Authority)
	{
		const EObjectFlags InstanceFlags = ComponentTemplate.GetFlags() | RF_Transient;

		UActorComponent* ComponentInstance = NewObject<UActorComponent>(&Actor, ComponentTemplate.GetClass(), ComponentTemplate.GetFName(), InstanceFlags, &ComponentTemplate);

		return ComponentInstance;
	}

	return nullptr;
}

UActorComponent* FFlowInjectComponentsHelper::TryCreateComponentInstanceForActorFromClass(AActor& Actor, TSubclassOf<UActorComponent> ComponentClass, const FName& InstanceBaseName)
{
	// Following pattern from UGameFrameworkComponentManager::CreateComponentOnInstance()
	if (ComponentClass && (!ComponentClass->GetDefaultObject<UActorComponent>()->GetIsReplicated() || Actor.GetLocalRole() == ROLE_Authority))
	{
		const FName UniqueName = MakeUniqueObjectName(&Actor, ComponentClass, InstanceBaseName);
		UActorComponent* ComponentInstance = NewObject<UActorComponent>(&Actor, ComponentClass, UniqueName);

		return ComponentInstance;
	}

	return nullptr;
}

void FFlowInjectComponentsHelper::InjectCreatedComponent(AActor& Actor, UActorComponent& ComponentInstance)
{
	// Following pattern from UGameFrameworkComponentManager::CreateComponentOnInstance()
	if (USceneComponent* SceneComponentInstance = Cast<USceneComponent>(&ComponentInstance))
	{
		SceneComponentInstance->SetupAttachment(Actor.GetRootComponent());
	}

	ComponentInstance.RegisterComponent();
}

void FFlowInjectComponentsHelper::DestroyInjectedComponent(AActor& Actor, UActorComponent& ComponentInstance)
{
	// Following pattern from UGameFrameworkComponentManager::DestroyInstancedComponent()
	ComponentInstance.DestroyComponent();
	ComponentInstance.SetFlags(RF_Transient);
}
