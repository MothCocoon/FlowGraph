// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Actor/FlowNode_OnActorRegistered.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_OnActorRegistered)

void UFlowNode_OnActorRegistered::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (!RegisteredActors.Contains(Actor))
	{
		RegisteredActors.Emplace(Actor, Component);
		OnEventReceived();
	}
}

#if WITH_EDITOR
const FFlowAgentDoc& UFlowNode_OnActorRegistered::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("Use to react to an actor spawning or becoming relevant, rather than polling for its existence. Pairs with On Actor Unregistered for the corresponding disappearance event."),
		/*Tags*/     { TEXT("actor"), TEXT("event"), TEXT("bind") },
		/*Articles*/ {  });
	return Doc;
}
#endif
