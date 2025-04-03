// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Actor/FlowNode_OnActorRegistered.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_OnActorRegistered)

UFlowNode_OnActorRegistered::UFlowNode_OnActorRegistered(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowNode_OnActorRegistered::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (!RegisteredActors.Contains(Actor))
	{
		RegisteredActors.Emplace(Actor, Component);
		OnEventReceived();
	}
}
