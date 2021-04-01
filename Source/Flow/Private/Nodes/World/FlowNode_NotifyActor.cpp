#include "Nodes/World/FlowNode_NotifyActor.h"
#include "FlowComponent.h"
#include "FlowSubsystem.h"

#include "Engine/World.h"

UFlowNode_NotifyActor::UFlowNode_NotifyActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Notifies");
#endif
}

void UFlowNode_NotifyActor::PostLoad()
{
	Super::PostLoad();

	if (IdentityTag_DEPRECATED.IsValid())
	{
		IdentityTags = FGameplayTagContainer(IdentityTag_DEPRECATED);
		IdentityTag_DEPRECATED = FGameplayTag();
	}
	
	if (NotifyTag_DEPRECATED.IsValid())
	{
		NotifyTags = FGameplayTagContainer(NotifyTag_DEPRECATED);
		NotifyTag_DEPRECATED = FGameplayTag();
	}
}

void UFlowNode_NotifyActor::ExecuteInput(const FName& PinName)
{
	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		for (TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(IdentityTags, EGameplayContainerMatchType::Any))
		{
			Component->NotifyFromGraph(NotifyTags);
		}
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNode_NotifyActor::GetNodeDescription() const
{
	return GetIdentityDescription(IdentityTags) + LINE_TERMINATOR + GetNotifyDescription(NotifyTags);
}
#endif
