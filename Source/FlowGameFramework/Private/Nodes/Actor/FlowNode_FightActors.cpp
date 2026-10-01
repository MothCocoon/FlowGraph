// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Actor/FlowNode_FightActors.h"

#include "ActorSpawning/FlowActorSpawnRecord.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_FightActors)

UFlowNode_FightActors::UFlowNode_FightActors()
{
#if WITH_EDITOR
	Category = TEXT("Flow|Actors");
#endif
}

#if WITH_SERVER_CODE
bool UFlowNode_FightActors::ShouldCheckCompletionAfterPass(bool bQueuedAnyRecords) const
{
	return !bQueuedAnyRecords;
}

void UFlowNode_FightActors::OnBeforeCohortActorCleanup(UFlowActorSpawnRecord& Record)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorState, 4);
	AActor* Actor = Record.GetOwnedActor();
	if (Actor 
		&& Actor->IsActorBeingDestroyed() 
		&& !bCleaningUpFight
		&& FightState == EFlowFightActorState::Executing)
	{
		NotifyActorDefeated(*Actor);
	}
}
#endif
