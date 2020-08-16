#include "Nodes/World/FlowNode_OnNotifyFromActor.h"
#include "FlowComponent.h"

UFlowNode_OnNotifyFromActor::UFlowNode_OnNotifyFromActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::Condition;
#endif

	OutputNames = { TEXT("Success") };
}

void UFlowNode_OnNotifyFromActor::ExecuteInput(const FName& PinName)
{
	if (IdentityTag.IsValid())
	{
		UFlowComponent::OnNotifyFromComponent.AddDynamic(this, &UFlowNode_OnNotifyFromActor::OnNotifyFromActor);
	}
}

void UFlowNode_OnNotifyFromActor::OnNotifyFromActor(class UFlowComponent* FlowComponent, const FGameplayTag& Tag)
{
	if (FlowComponent->IdentityTags.HasTagExact(IdentityTag) && (!NotifyTag.IsValid() || NotifyTag == Tag))
	{
		TriggerFirstOutput(true);
	}
}

void UFlowNode_OnNotifyFromActor::Cleanup()
{
	if (IdentityTag.IsValid())
	{
		UFlowComponent::OnNotifyFromComponent.RemoveAll(this);
	}
}
