// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn_FightActorsCohortBase.h"

#include "ActorSpawning/FlowActorSpawnRecord.h"
#include "GameFramework/Actor.h"
#include "Nodes/Actor/FlowNode_FightActorsBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn_FightActorsCohortBase)

void UFlowNodeAddOn_FightActorsCohortBase::Cleanup()
{
#if WITH_SERVER_CODE
	CleanupActorRecords();
#endif

	Super::Cleanup();
}

#if WITH_SERVER_CODE
int32 UFlowNodeAddOn_FightActorsCohortBase::ExecuteSpawningPass(
	EFlowFightActorSpawnMethod Method, FFlowActorSpawningAssistant& Assistant, bool bIsInitialPass)
{
	return INDEX_NONE;
}

void UFlowNodeAddOn_FightActorsCohortBase::AppendActorCounts(FFlowFightActorCounts& InOutCounts) const
{
	for (const UFlowActorSpawnRecord* Record : ActorRecords)
	{
		if (!IsValid(Record))
		{
			continue;
		}

		if (Record->IsSpawnedActorAlive())
		{
			++InOutCounts.AliveCount;
		}
		else if (Record->IsQueuedOrActivelyAttemptingtoSpawn())
		{
			++InOutCounts.PendingSpawnCount;
		}

		++InOutCounts.TotalCount;
	}
	InOutCounts.InitialSpawnCount += GetInitialActorCount();
}

void UFlowNodeAddOn_FightActorsCohortBase::DefeatSpawnedActors()
{
	const TArray<TObjectPtr<UFlowActorSpawnRecord>> RecordsToDefeat = ActorRecords;
	for (UFlowActorSpawnRecord* Record : RecordsToDefeat)
	{
		if (!IsValid(Record))
		{
			continue;
		}

		if (AActor* Actor = Record->GetOwnedActor(); IsValid(Actor))
		{
			Actor->Destroy();
		}
		else if (Record->IsQueuedOrActivelyAttemptingtoSpawn())
		{
			Record->CleanupRuntime();
		}
	}
}

void UFlowNodeAddOn_FightActorsCohortBase::CleanupActorRecords()
{
	const TArray<TObjectPtr<UFlowActorSpawnRecord>> RecordsToCleanup = ActorRecords;
	for (UFlowActorSpawnRecord* Record : RecordsToCleanup)
	{
		if (IsValid(Record))
		{
			Record->CleanupRuntime();
		}
	}

	ActorRecords.Empty();
}

int32 UFlowNodeAddOn_FightActorsCohortBase::GetInitialActorCount() const
{
	return 0;
}

AActor* UFlowNodeAddOn_FightActorsCohortBase::TryGetActorOwner() const
{
	return TryGetRootFlowActorOwner();
}

void UFlowNodeAddOn_FightActorsCohortBase::ProcessPreSpawnConfiguration(UFlowActorSpawnRecord& Record)
{
	if (UFlowNode_FightActorsBase* Fight = Cast<UFlowNode_FightActorsBase>(GetFlowNode()))
	{
		Fight->NotifyCohortPreSpawn(Record);
	}
}

void UFlowNodeAddOn_FightActorsCohortBase::ProcessPreFinishSpawnActor(UFlowActorSpawnRecord& Record)
{
	if (UFlowNode_FightActorsBase* Fight = Cast<UFlowNode_FightActorsBase>(GetFlowNode()))
	{
		Fight->NotifyCohortPreFinishSpawn(Record);
	}
}

void UFlowNodeAddOn_FightActorsCohortBase::ProcessPostSpawnConfiguration(UFlowActorSpawnRecord& Record)
{
	if (UFlowNode_FightActorsBase* Fight = Cast<UFlowNode_FightActorsBase>(GetFlowNode()))
	{
		Fight->NotifyCohortPostSpawn(Record);
	}
}

void UFlowNodeAddOn_FightActorsCohortBase::ProcessFinishedSpawnAttempt(UFlowActorSpawnRecord& Record)
{
	if (UFlowNode_FightActorsBase* Fight = Cast<UFlowNode_FightActorsBase>(GetFlowNode()))
	{
		Fight->NotifyCohortSpawnAttemptFinished(Record);
	}
}

void UFlowNodeAddOn_FightActorsCohortBase::OnCleanupSpawnedInstance(UFlowActorSpawnRecord& Record)
{
	if (UFlowNode_FightActorsBase* Fight = Cast<UFlowNode_FightActorsBase>(GetFlowNode()))
	{
		Fight->NotifyCohortActorCleanup(Record);
	}
}
#endif

#if WITH_EDITOR
FText UFlowNodeAddOn_FightActorsCohortBase::K2_GetNodeTitle_Implementation() const
{
	if (!CustomCohortTitle.IsEmpty())
	{
		return CustomCohortTitle;
	}

	return Super::K2_GetNodeTitle_Implementation();
}
#endif
