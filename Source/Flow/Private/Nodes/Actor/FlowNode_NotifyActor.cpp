// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Actor/FlowNode_NotifyActor.h"

#include "FlowComponent.h"
#include "FlowSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_NotifyActor)

UFlowNode_NotifyActor::UFlowNode_NotifyActor()
{
#if WITH_EDITOR
	Category = TEXT("Actor");
#endif
}

void UFlowNode_NotifyActor::ExecuteInput(const FName& PinName)
{
	if (const UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		for (const TWeakObjectPtr<UFlowComponent>& Component : FlowSubsystem->GetComponents<UFlowComponent>(IdentityTags, MatchType, bExactMatch))
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

EDataValidationResult UFlowNode_NotifyActor::ValidateNode()
{
	if (IdentityTags.IsEmpty())
	{
		ValidationLog.Error<UFlowNode>(*UFlowNode::MissingIdentityTag, this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

const FFlowAgentDoc& UFlowNode_NotifyActor::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("Set bExactMatch carefully - disabling it can become expensive, since the search cost then scales with the total number of registered Gameplay Tags rather than just this node's tags."),
		/*Tags*/     { TEXT("actor"), TEXT("notify"), TEXT("event") },
		/*Articles*/ {  });
	return Doc;
}
#endif
