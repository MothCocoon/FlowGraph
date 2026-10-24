// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Types/FlowActorOwnerComponentRef.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Misc/RuntimeErrors.h"
#include "FlowLogChannels.h"

UActorComponent* FFlowActorOwnerComponentRef::TryResolveComponent(const AActor& InActor, bool bWarnIfFailed)
{
	if (!IsResolved() && IsConfigured())
	{
		ResolvedComponent = TryResolveComponentByName(InActor, ComponentName);

		if (bWarnIfFailed && !IsValid(ResolvedComponent))
		{
			UE_LOG(LogFlow, Warning, TEXT("Could not resolve component named %s on actor %s"), *ComponentName.ToString(), *InActor.GetName());
		}
	}

	return ResolvedComponent;
}

void FFlowActorOwnerComponentRef::SetResolvedComponentDirect(UActorComponent& Component)
{
	ComponentName = Component.GetFName();

	ResolvedComponent = &Component;
}

bool FFlowActorOwnerComponentRef::IsResolved() const
{
	return IsValid(ResolvedComponent);
}

UActorComponent* FFlowActorOwnerComponentRef::TryResolveComponentByName(const AActor& InActor, const FName& InComponentName)
{
	constexpr bool bIncludeFromChildActors = false;

	UActorComponent* FoundComponent = nullptr;

	// Search for the component (by name) on the given actor
	InActor.ForEachComponent(
		bIncludeFromChildActors,
		[&FoundComponent, &InComponentName](UActorComponent* Component)
		{
			FString CleanedName = Component->GetName();
			CleanedName.RemoveFromEnd(TEXT("_C"));

			if ((InComponentName == Component->GetFName() || InComponentName == FName(CleanedName)) &&
				ensureAsRuntimeWarning(FoundComponent == nullptr))
			{
				FoundComponent = Component;
			}
		});

	return FoundComponent;
}

