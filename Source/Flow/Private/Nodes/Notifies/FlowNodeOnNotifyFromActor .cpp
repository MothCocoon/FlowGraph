#include "FlowNodeOnNotifyFromActor.h"
#include "FlowComponent.h"

UFlowNodeOnNotifyFromActor::UFlowNodeOnNotifyFromActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::Condition;
#endif

	OutputNames = { TEXT("Success") };
}

void UFlowNodeOnNotifyFromActor::ExecuteInput(const FName& PinName)
{
	if (IdentityTag.IsValid())
	{
		UFlowComponent::OnNotifyFromComponent.AddDynamic(this, &UFlowNodeOnNotifyFromActor::OnNotifyFromActor);
	}
}

void UFlowNodeOnNotifyFromActor::OnNotifyFromActor(class UFlowComponent* FlowComponent, const FGameplayTag& Tag)
{
	if (FlowComponent->IdentityTags.HasTagExact(IdentityTag) && (!NotifyTag.IsValid() || NotifyTag == Tag))
	{
		TriggerFirstOutput(true);
	}
}

void UFlowNodeOnNotifyFromActor::Cleanup()
{
	if (IdentityTag.IsValid())
	{
		UFlowComponent::OnNotifyFromComponent.RemoveAll(this);
	}
}
