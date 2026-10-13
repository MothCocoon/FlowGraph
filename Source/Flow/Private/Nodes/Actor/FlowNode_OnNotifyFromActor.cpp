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
			const FGameplayTagContainer& RecentlySentNotifyTags = Component->GetRecentlySentNotifyTags();
			if (bExactMatch ? RecentlySentNotifyTags.HasAnyExact(NotifyTags) : RecentlySentNotifyTags.HasAny(NotifyTags))
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
			if (bExactMatch ? Tag.MatchesAnyExact(NotifyTags) : Tag.MatchesAny(NotifyTags))
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

#if WITH_EDITOR
const FFlowAgentDoc& UFlowNode_OnNotifyFromActor::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("Enable bRetroactive to also fire if the matching notify was already sent before this node started observing - useful in multiplayer, where a client-side node may start listening after the server already sent the notify."),
		/*Tags*/     { TEXT("actor"), TEXT("notify"), TEXT("event") },
		/*Articles*/ {  });
	return Doc;
}
#endif
