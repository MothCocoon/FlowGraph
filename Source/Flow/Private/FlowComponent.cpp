#include "FlowComponent.h"
#include "FlowSubsystem.h"

#include "Engine/World.h"

FFlowComponentNotify UFlowComponent::OnNotifyFromComponent;

UFlowComponent::UFlowComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		FlowSubsystem->RegisterComponent(this);
	}
}

void UFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		FlowSubsystem->UnregisterComponent(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UFlowComponent::NotifyGraph(const FGameplayTag NotifyTag)
{
	OnNotifyFromComponent.Broadcast(this, NotifyTag);
}

void UFlowComponent::NotifyFromGraph(const FGameplayTag NotifyTag)
{
	ReceiveNotify.Broadcast(nullptr, NotifyTag);
}

void UFlowComponent::NotifyActor(const FGameplayTag ActorTag, const FGameplayTag NotifyTag)
{
	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		for (TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(ActorTag))
		{
			Component->ReceiveNotify.Broadcast(this, NotifyTag);
		}
	}
}
