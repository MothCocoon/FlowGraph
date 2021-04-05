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

void UFlowNode_OnNotifyFromActor::PostLoad()
{
	Super::PostLoad();

	if (NotifyTag_DEPRECATED.IsValid())
	{
		NotifyTags = FGameplayTagContainer(NotifyTag_DEPRECATED);
	}
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
			if (SelfWeakPtr.IsValid() && FlowComponent->IdentityTags.HasAnyExact(SelfWeakPtr.Get()->IdentityTags)
				&& (!SelfWeakPtr.Get()->NotifyTags.IsValid() || SelfWeakPtr.Get()->NotifyTags.HasTagExact(Tag)))
			{
				SelfWeakPtr->OnEventReceived();
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
	return GetIdentityDescription(IdentityTags) + LINE_TERMINATOR + GetNotifyDescription(NotifyTags);
}
#endif
