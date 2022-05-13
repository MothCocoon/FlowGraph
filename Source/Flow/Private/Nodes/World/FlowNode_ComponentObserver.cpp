// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/World/FlowNode_ComponentObserver.h"
#include "FlowSubsystem.h"

UFlowNode_ComponentObserver::UFlowNode_ComponentObserver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, IdentityMatchType(EFlowTagContainerMatchType::HasAnyExact)
	, SuccessLimit(1)
	, SuccessCount(0)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::Condition;
	Category = TEXT("World");
#endif

	InputPins = {FFlowPin(TEXT("Start")), FFlowPin(TEXT("Stop"))};
	OutputPins = {FFlowPin(TEXT("Success")), FFlowPin(TEXT("Completed")), FFlowPin(TEXT("Stopped"))};
}

void UFlowNode_ComponentObserver::PostLoad()
{
	Super::PostLoad();

	if (IdentityTag_DEPRECATED.IsValid())
	{
		IdentityTags = FGameplayTagContainer(IdentityTag_DEPRECATED);
	}
}

void UFlowNode_ComponentObserver::ExecuteInput(const FName& PinName)
{
	if (IdentityTags.IsValid())
	{
		if (PinName == TEXT("Start"))
		{
			StartObserving();
		}
		else if (PinName == TEXT("Stop"))
		{
			TriggerOutput(TEXT("Stopped"), true);
		}
	}
	else
	{
		LogError(MissingIdentityTag);
	}
}

void UFlowNode_ComponentObserver::OnLoad_Implementation()
{
	if (IdentityTags.IsValid())
	{
		StartObserving();
	}
}

void UFlowNode_ComponentObserver::StartObserving()
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		for (const TWeakObjectPtr<UFlowComponent>& FoundComponent : FlowSubsystem->GetComponents<UFlowComponent>(IdentityTags, EGameplayContainerMatchType::Any))
		{
			ObserveActor(FoundComponent->GetOwner(), FoundComponent);
		}

		// clear old bindings before binding again, which might happen while loading a SaveGame
		StopObserving();

		FlowSubsystem->OnComponentRegistered.AddDynamic(this, &UFlowNode_ComponentObserver::OnComponentRegistered);
		FlowSubsystem->OnComponentTagAdded.AddDynamic(this, &UFlowNode_ComponentObserver::OnComponentTagAdded);
		FlowSubsystem->OnComponentTagRemoved.AddDynamic(this, &UFlowNode_ComponentObserver::OnComponentTagRemoved);
		FlowSubsystem->OnComponentUnregistered.AddDynamic(this, &UFlowNode_ComponentObserver::OnComponentUnregistered);
	}
}

void UFlowNode_ComponentObserver::StopObserving()
{
	if (UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		FlowSubsystem->OnComponentRegistered.RemoveAll(this);
		FlowSubsystem->OnComponentUnregistered.RemoveAll(this);
		FlowSubsystem->OnComponentTagAdded.RemoveAll(this);
		FlowSubsystem->OnComponentTagRemoved.RemoveAll(this);
	}
}

void UFlowNode_ComponentObserver::OnComponentRegistered(UFlowComponent* Component)
{
	if (!RegisteredActors.Contains(Component->GetOwner()) && FlowTypes::HasMatchingTags(Component->IdentityTags, IdentityTags, IdentityMatchType) == true)
	{
		ObserveActor(Component->GetOwner(), Component);
	}
}

void UFlowNode_ComponentObserver::OnComponentTagAdded(UFlowComponent* Component, const FGameplayTagContainer& AddedTags)
{
	if (!RegisteredActors.Contains(Component->GetOwner()) && FlowTypes::HasMatchingTags(Component->IdentityTags, IdentityTags, IdentityMatchType) == true)
	{
		ObserveActor(Component->GetOwner(), Component);
	}
}

void UFlowNode_ComponentObserver::OnComponentTagRemoved(UFlowComponent* Component, const FGameplayTagContainer& RemovedTags)
{
	if (RegisteredActors.Contains(Component->GetOwner()) && FlowTypes::HasMatchingTags(Component->IdentityTags, IdentityTags, IdentityMatchType) == false)
	{
		RegisteredActors.Remove(Component->GetOwner());
		ForgetActor(Component->GetOwner(), Component);
	}
}

void UFlowNode_ComponentObserver::OnComponentUnregistered(UFlowComponent* Component)
{
	if (RegisteredActors.Contains(Component->GetOwner()))
	{
		RegisteredActors.Remove(Component->GetOwner());
		ForgetActor(Component->GetOwner(), Component);
	}
}

void UFlowNode_ComponentObserver::OnEventReceived()
{
	TriggerFirstOutput(false);

	SuccessCount++;
	if (SuccessLimit > 0 && SuccessCount == SuccessLimit)
	{
		TriggerOutput(TEXT("Completed"), true);
	}
}

void UFlowNode_ComponentObserver::Cleanup()
{
	StopObserving();

	if (RegisteredActors.Num() > 0)
	{
		for (const TPair<TWeakObjectPtr<AActor>, TWeakObjectPtr<UFlowComponent>>& RegisteredActor : RegisteredActors)
		{
			ForgetActor(RegisteredActor.Key, RegisteredActor.Value);
		}
	}
	RegisteredActors.Empty();

	SuccessCount = 0;
}

#if WITH_EDITOR
FString UFlowNode_ComponentObserver::GetNodeDescription() const
{
	return GetIdentityTagsDescription(IdentityTags);
}

FString UFlowNode_ComponentObserver::GetStatusString() const
{
	if (ActivationState == EFlowNodeState::Active && RegisteredActors.Num() == 0)
	{
		return NoActorsFound;
	}

	return FString();
}
#endif
