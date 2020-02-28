#include "FlowNodeOnNotifyFromActor.h"
#include "FlowComponent.h"

#include "Engine/World.h"

UFlowNodeOnNotifyFromActor::UFlowNodeOnNotifyFromActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutputNames = { TEXT("Success") };
}

void UFlowNodeOnNotifyFromActor::ExecuteInput(const FName& PinName)
{
	if (ActorTag.IsValid())
	{
		UFlowComponent::OnNotifyFromActor.AddUObject(this, &UFlowNodeOnNotifyFromActor::OnNotifyFromActor);
	}
}

void UFlowNodeOnNotifyFromActor::OnNotifyFromActor(UFlowComponent* FlowComponent, const FGameplayTag& Tag)
{
	if (FlowComponent->IdentityTags.HasTagExact(ActorTag) && (!NotifyTag.IsValid() || NotifyTag == Tag))
	{
		TriggerFirstOutput(true);
	}
}

void UFlowNodeOnNotifyFromActor::Cleanup()
{
	if (ActorTag.IsValid())
	{
		UFlowComponent::OnNotifyFromActor.RemoveAll(this);
	}
}

#if WITH_EDITOR
FString UFlowNodeOnNotifyFromActor::GetNodeDescription() const
{
	return ActorTag.IsValid() ? ActorTag.ToString() : FString();
}
#endif