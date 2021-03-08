#include "Nodes/World/FlowNode_OnNotifyFromActor.h"
#include "FlowComponent.h"

UFlowNode_OnNotifyFromActor::UFlowNode_OnNotifyFromActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Notifies");
	NodeStyle = EFlowNodeStyle::Condition;
#endif
}

void UFlowNode_OnNotifyFromActor::ExecuteInput(const FName& PinName)
{
	Super::ExecuteInput(PinName);
}

void UFlowNode_OnNotifyFromActor::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (!RegisteredActors.Contains(Actor))
	{
		RegisteredActors.Emplace(Actor, Component);

		TWeakObjectPtr<UFlowNode_OnNotifyFromActor> SelfWeakPtr(this);
		Component->OnNotifyFromComponent.AddWeakLambda(this, [SelfWeakPtr](UFlowComponent* FlowComponent, const FGameplayTag& Tag)
		{
			if (SelfWeakPtr.IsValid() && FlowComponent->IdentityTags.HasTagExact(SelfWeakPtr.Get()->IdentityTag)
				&& (!SelfWeakPtr.Get()->NotifyTag.IsValid() || SelfWeakPtr.Get()->NotifyTag == Tag))
			{
				SelfWeakPtr->TriggerFirstOutput(true);
			}
		});
	}
}

void UFlowNode_OnNotifyFromActor::ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	Component->OnNotifyFromComponent.RemoveAll(this);
}

#if WITH_EDITOR
FString UFlowNode_OnNotifyFromActor::GetNodeDescription() const
{
	const FString IdentityString = IdentityTag.IsValid() ? IdentityTag.ToString() : MissingIdentityTag;
	const FString NotifyString = NotifyTag.IsValid() ? NotifyTag.ToString() : TEXT("---");

	return IdentityString + LINE_TERMINATOR + NotifyString;
}
#endif
