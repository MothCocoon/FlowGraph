// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/World/FlowNode_NotifyActor.h"
#include "FlowComponent.h"
#include "FlowSubsystem.h"

#include "Engine/World.h"

UFlowNode_NotifyActor::UFlowNode_NotifyActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NetMode(EFlowNetMode::Authority)
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
	}
	
	if (NotifyTag_DEPRECATED.IsValid())
	{
		NotifyTags = FGameplayTagContainer(NotifyTag_DEPRECATED);
		NotifyTag_DEPRECATED = FGameplayTag();
	}
}

void UFlowNode_NotifyActor::ExecuteInput(const FName& PinName)
{
	if (const UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		for (const TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(IdentityTags, EGameplayContainerMatchType::Any))
		{
			Component->NotifyFromGraph(NotifyTags, NetMode);
		}
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNode_NotifyActor::GetNodeDescription() const
{
	return GetIdentityTagsDescription(IdentityTags) + LINE_TERMINATOR + GetNotifyTagsDescription(NotifyTags);
}
#endif
