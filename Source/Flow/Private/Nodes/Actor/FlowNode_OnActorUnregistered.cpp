// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Actor/FlowNode_OnActorUnregistered.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_OnActorUnregistered)

UFlowNode_OnActorUnregistered::UFlowNode_OnActorUnregistered(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

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
