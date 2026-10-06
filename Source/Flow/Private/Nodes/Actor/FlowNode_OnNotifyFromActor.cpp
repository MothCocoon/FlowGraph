// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Actor/FlowNode_OnNotifyFromActor.h"

#include "FlowComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_OnNotifyFromActor)

UFlowNode_OnNotifyFromActor::UFlowNode_OnNotifyFromActor()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::Condition;
#endif
}

void UFlowNode_OnNotifyFromActor::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (!RegisteredActors.Contains(Actor))
	{
		RegisteredActors.Emplace(Actor, Component);
		Component->OnNotifyFromComponent.AddUObject(this, &UFlowNode_OnNotifyFromActor::OnNotifyFromComponent);

		if (bRetroactive)
		{
			if (FlowTypes::HasMatchingTags(Component->GetRecentlySentNotifyTags(), NotifyTags, NotifyMatchType))
			{
				OnEventReceived();
			}
		}
	}
}

void UFlowNode_OnNotifyFromActor::ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	Component->OnNotifyFromComponent.RemoveAll(this);
}

void UFlowNode_OnNotifyFromActor::OnNotifyFromComponent(UFlowComponent* Component, const FGameplayTag& Tag)
{
	if (FlowTypes::HasMatchingTags(Component->IdentityTags, IdentityTags, IdentityMatchType)) // identity matches?
	{
		if (NotifyTags.IsValid())
		{
			if (FlowTypes::HasMatchingTag(Tag, NotifyTags, NotifyMatchType))
			{
				OnEventReceived();
			}
		}
		else
		{
			OnEventReceived();
		}
	}
}

#if WITH_EDITOR
FString UFlowNode_OnNotifyFromActor::GetNodeDescription() const
{
	return GetIdentityTagsDescription(IdentityTags) + LINE_TERMINATOR + GetNotifyTagsDescription(NotifyTags);
}
#endif
