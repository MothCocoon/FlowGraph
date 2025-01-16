// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Actor/FlowNode_ComponentObserver.h"
#include "FlowSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_ComponentObserver)

UFlowNode_ComponentObserver::UFlowNode_ComponentObserver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, IdentityMatchType(EFlowTagContainerMatchType::HasAnyExact)
	, SuccessLimit(1)
	, SuccessCount(0)
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::Condition;
	Category = TEXT("Actor");
#endif

	InputPins = {FFlowPin(TEXT("Start")), FFlowPin(TEXT("Stop"))};
	OutputPins = {FFlowPin(TEXT("Success")), FFlowPin(TEXT("Completed")), FFlowPin(TEXT("Stopped"))};
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
		// translate Flow name into engine types
		const EGameplayContainerMatchType ContainerMatchType = (IdentityMatchType == EFlowTagContainerMatchType::HasAny || IdentityMatchType == EFlowTagContainerMatchType::HasAnyExact) ? EGameplayContainerMatchType::Any : EGameplayContainerMatchType::All;
		const bool bExactMatch = (IdentityMatchType == EFlowTagContainerMatchType::HasAnyExact || IdentityMatchType == EFlowTagContainerMatchType::HasAllExact);

		// collect already registered components
		for (const TWeakObjectPtr<UFlowComponent>& FoundComponent : FlowSubsystem->GetComponents<UFlowComponent>(IdentityTags, ContainerMatchType, bExactMatch))
		{
			ObserveActor(FoundComponent->GetOwner(), FoundComponent);
			
			// node might finish work immediately as the effect of ObserveActor()
			// we should terminate iteration in this case
			if (GetActivationState() != EFlowNodeState::Active)
			{
				return;
			}
		}
		
		FlowSubsystem->OnComponentRegistered.AddUniqueDynamic(this, &UFlowNode_ComponentObserver::OnComponentRegistered);
		FlowSubsystem->OnComponentTagAdded.AddUniqueDynamic(this, &UFlowNode_ComponentObserver::OnComponentTagAdded);
		FlowSubsystem->OnComponentTagRemoved.AddUniqueDynamic(this, &UFlowNode_ComponentObserver::OnComponentTagRemoved);
		FlowSubsystem->OnComponentUnregistered.AddUniqueDynamic(this, &UFlowNode_ComponentObserver::OnComponentUnregistered);
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

EDataValidationResult UFlowNode_ComponentObserver::ValidateNode()
{
	if (IdentityTags.IsEmpty())
	{
		ValidationLog.Error<UFlowNode>(*UFlowNode::MissingIdentityTag, this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
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
