// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Actor/FlowNode_OnNotifyFromActor.h"
#include "FlowComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_OnNotifyFromActor)

UFlowNode_OnNotifyFromActor::UFlowNode_OnNotifyFromActor()
	: NotifyMatchType(EFlowTagMatchType::HasExact)
	, bRetroactive(false)
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
			const bool NotifyMatches = FlowTypes::HasMatchingTags(Component->GetRecentlySentNotifyTags(), NotifyTags, NotifyMatchType);
			if (NotifyMatches)
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
	const bool IdentityMatches = FlowTypes::HasMatchingTags(Component->IdentityTags, IdentityTags, IdentityMatchType);
	if (IdentityMatches)
	{
		const bool NotifyMatches = NotifyTags.IsValid() ? FlowTypes::HasMatchingTag(Tag, NotifyTags, NotifyMatchType) : true;
		if (NotifyMatches)
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
