// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Actor/FlowNode_OnActorUnregistered.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_OnActorUnregistered)

void UFlowNode_OnActorUnregistered::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (!RegisteredActors.Contains(Actor))
	{
		RegisteredActors.Emplace(Actor, Component);
	}
}

void UFlowNode_OnActorUnregistered::ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (ActivationState == EFlowNodeState::Active)
	{
		OnEventReceived();
	}
}

#if WITH_EDITOR
const FFlowAgentDoc& UFlowNode_OnActorUnregistered::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("Use to react to an actor being destroyed or otherwise going out of scope. Pairs with On Actor Registered for the corresponding appearance event."),
		/*Tags*/     { TEXT("actor"), TEXT("event"), TEXT("unbind") },
		/*Articles*/ {  });
	return Doc;
}
#endif
