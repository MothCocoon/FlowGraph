// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/World/FlowNode_OnActorRegistered.h"

UFlowNode_OnActorRegistered::UFlowNode_OnActorRegistered(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowNode_OnActorRegistered::ExecuteInput(const FName& PinName)
{
	Super::ExecuteInput(PinName);
}

void UFlowNode_OnActorRegistered::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (!RegisteredActors.Contains(Actor))
	{
		RegisteredActors.Emplace(Actor, Component);
		OnEventReceived();
	}
}
