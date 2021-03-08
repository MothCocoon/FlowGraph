#include "Nodes/World/FlowNode_ComponentObserver.h"
#include "FlowSubsystem.h"

UFlowNode_ComponentObserver::UFlowNode_ComponentObserver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::Condition;
	Category = TEXT("World");
#endif

	InputNames = {TEXT("Start"), TEXT("Stop")};
	OutputNames = {TEXT("Success"), TEXT("Stopped")};
}

void UFlowNode_ComponentObserver::ExecuteInput(const FName& PinName)
{
	if (IdentityTag.IsValid())
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

void UFlowNode_ComponentObserver::StartObserving()
{
	for (const TWeakObjectPtr<UFlowComponent>& FoundComponent : GetFlowSubsystem()->GetComponents<UFlowComponent>(IdentityTag))
	{
		ObserveActor(FoundComponent->GetOwner(), FoundComponent);
	}

	GetFlowSubsystem()->OnComponentRegistered.AddUObject(this, &UFlowNode_ComponentObserver::OnComponentRegistered);
	GetFlowSubsystem()->OnComponentUnregistered.AddUObject(this, &UFlowNode_ComponentObserver::OnComponentUnregistered);
}

void UFlowNode_ComponentObserver::StopObserving()
{
	GetFlowSubsystem()->OnComponentRegistered.RemoveAll(this);
	GetFlowSubsystem()->OnComponentUnregistered.RemoveAll(this);
}

void UFlowNode_ComponentObserver::OnComponentRegistered(UFlowComponent* Component)
{
	if (Component->IdentityTags.HasTagExact(IdentityTag))
	{
		ObserveActor(Component->GetOwner(), Component);
	}
}

void UFlowNode_ComponentObserver::OnComponentUnregistered(UFlowComponent* Component)
{
	if (RegisteredActors.Contains(Component->GetOwner()) && Component->IdentityTags.HasTagExact(IdentityTag))
	{
		RegisteredActors.Remove(Component->GetOwner());
		ForgetActor(Component->GetOwner(), Component);
	}
}

void UFlowNode_ComponentObserver::Cleanup()
{
	StopObserving();

	for (const TPair<TWeakObjectPtr<AActor>, TWeakObjectPtr<UFlowComponent>>& RegisteredActor : RegisteredActors)
	{
		ForgetActor(RegisteredActor.Key, RegisteredActor.Value);
	}
	RegisteredActors.Empty();
}

#if WITH_EDITOR
FString UFlowNode_ComponentObserver::GetNodeDescription() const
{
	return IdentityTag.IsValid() ? IdentityTag.ToString() : TEXT("Missing Identity Tag!");
}

FString UFlowNode_ComponentObserver::GetStatusString() const
{
	if (ActivationState == EFlowActivationState::Active && RegisteredActors.Num() == 0)
	{
		return NoActorsFound;
	}

	return FString();
}
#endif
