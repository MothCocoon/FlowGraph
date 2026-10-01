// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Actor/FlowNode_SpawnActorsBase.h"

#include "ActorSpawning/FlowActorSpawnRecord.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_SpawnActorsBase)

const FFlowPin UFlowNode_SpawnActorsBase::INPIN_InitialSpawns(TEXT("Initial Spawning"), TEXT("Start all of the initial spawns"));
const FFlowPin UFlowNode_SpawnActorsBase::INPIN_DefeatSpawnedActors(TEXT("Defeat Spawned Actors"), TEXT("Defeat all of the spawned actors"));
const FFlowPin UFlowNode_SpawnActorsBase::OUTPIN_EachSpawnSucceeded(TEXT("Successful Spawn"), TEXT("Triggered for each successful spawn"));
const FFlowPin UFlowNode_SpawnActorsBase::OUTPIN_EachSpawnFailed(TEXT("Failed Spawn"), TEXT("Triggered for each failed spawn"));
const FFlowPin UFlowNode_SpawnActorsBase::OUTPIN_AllSpawnsCompleted(TEXT("All Spawning Complete"), TEXT("Triggered when all spawns have completed"));

UFlowNode_SpawnActorsBase::UFlowNode_SpawnActorsBase()
{
	SpawningAssistant.InitializeAs<FFlowActorSpawningAssistant>();
#if WITH_EDITOR
	Category = TEXT("Flow|Actors");
#endif

	InputPins.Reset();
	InputPins.Add(INPIN_InitialSpawns);
	InputPins.Add(INPIN_DefeatSpawnedActors);

	OutputPins.Reset();
	OutputPins.Add(OUTPIN_AllSpawnsCompleted);
	OutputPins.Add(OUTPIN_EachSpawnSucceeded);
	OutputPins.Add(OUTPIN_EachSpawnFailed);
}

void UFlowNode_SpawnActorsBase::InitializeInstance()
{
	Super::InitializeInstance();

#if WITH_SERVER_CODE
	if (FFlowActorSpawningAssistant* Assistant = SpawningAssistant.GetMutablePtr())
	{
		Assistant->Startup();
	}
#endif
}

void UFlowNode_SpawnActorsBase::ExecuteInput(const FName& PinName)
{
#if WITH_SERVER_CODE
	if (PinName == INPIN_InitialSpawns.PinName)
	{
		StartSpawningPass();
	}
	else if (PinName == INPIN_DefeatSpawnedActors.PinName)
	{
		DefeatAllSpawnedActors();
	}
#endif

	Super::ExecuteInput(PinName);
}

void UFlowNode_SpawnActorsBase::Cleanup()
{
#if WITH_SERVER_CODE
	CleanupActorRecords();
#endif

	Super::Cleanup();
}

void UFlowNode_SpawnActorsBase::DeinitializeInstance()
{
#if WITH_SERVER_CODE
	if (FFlowActorSpawningAssistant* Assistant = SpawningAssistant.GetMutablePtr())
	{
		Assistant->Shutdown();
	}
#endif
	OnSpawningAssistantShutdown();
	SpawnedActors.Empty();

	Super::DeinitializeInstance();
}

#if WITH_SERVER_CODE
void UFlowNode_SpawnActorsBase::HandlePreSpawnConfiguration(UFlowActorSpawnRecord& Record)
{
	if (FFlowActorSpawningAssistant* Assistant = SpawningAssistant.GetMutablePtr())
	{
		Assistant->ProcessPreSpawnConfiguration(Record);
	}
}

void UFlowNode_SpawnActorsBase::HandlePreFinishSpawnActor(UFlowActorSpawnRecord& Record)
{
	if (FFlowActorSpawningAssistant* Assistant = SpawningAssistant.GetMutablePtr())
	{
		Assistant->ProcessPreFinishSpawnActor(Record);
	}
}

void UFlowNode_SpawnActorsBase::HandlePostSpawnConfiguration(UFlowActorSpawnRecord& Record)
{
	if (FFlowActorSpawningAssistant* Assistant = SpawningAssistant.GetMutablePtr())
	{
		Assistant->ProcessPostSpawnConfiguration(Record);
	}
}

void UFlowNode_SpawnActorsBase::HandleFinishedSpawnAttempt(UFlowActorSpawnRecord& Record)
{
	if (FFlowActorSpawningAssistant* Assistant = SpawningAssistant.GetMutablePtr())
	{
		Assistant->ProcessFinishedSpawnAttempt(Record);
	}

	if (Record.IsSpawnSuccessful())
	{
		if (AActor* SpawnedActor = Record.GetOwnedActor(); IsValid(SpawnedActor))
		{
			SpawnedActors.Add(SpawnedActor);
		}
		TriggerOutput(OUTPIN_EachSpawnSucceeded.PinName, false);
	}
	else
	{
		check(Record.IsSpawnFailed());
		TriggerOutput(OUTPIN_EachSpawnFailed.PinName, false);
	}

	if (!HasAnyOutstandingSpawns())
	{
		TriggerOutput(OUTPIN_AllSpawnsCompleted.PinName, false);
	}
}

void UFlowNode_SpawnActorsBase::HandleCleanupSpawnedInstance(UFlowActorSpawnRecord& Record)
{
	if (FFlowActorSpawningAssistant* Assistant = SpawningAssistant.GetMutablePtr())
	{
		Assistant->OnCleanupSpawnedInstance(Record);
	}

	if (AActor* SpawnedActor = Record.GetOwnedActor())
	{
		SpawnedActors.Remove(SpawnedActor);
	}
}
#endif
