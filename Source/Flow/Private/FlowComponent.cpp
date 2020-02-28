#include "FlowComponent.h"
#include "FlowSubsystem.h"

#include "Engine/World.h"

FFlowActorNotify UFlowComponent::OnNotifyFromActor;

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

void UFlowComponent::NotifyGraph(const FGameplayTag Tag)
{
	OnNotifyFromActor.Broadcast(this, Tag);
}