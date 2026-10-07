// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Actor/FlowNode_SpawnActors.h"

#include "ActorSpawning/FlowActorSpawnRecord.h"
#include "ActorSpawning/FlowActorSpawnSelector.h"
#include "ActorSpawning/FlowActorSpawningAssistant.h"
#include "ActorSpawning/FlowActorSpawningSubsystem.h"
#include "ActorSpawning/FlowDefaultActorSpawnRecord.h"
#include "Containers/Set.h"
#include "FlowSettings.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_SpawnActors)

UFlowNode_SpawnActors::UFlowNode_SpawnActors()
{
	ActorSpawnSelector.InitializeAs<FFlowActorSpawnSelector_ActorClass>();
}

void UFlowNode_SpawnActors::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	if (!HasAnyFlags(RF_ClassDefaultObject) && !IsValid(SpawnRecordTemplate))
	{
		SpawnRecordTemplate = NewObject<UFlowDefaultActorSpawnRecord>(
			this, UFlowDefaultActorSpawnRecord::StaticClass(), TEXT("SpawnRecordTemplate"), RF_Public | RF_Transactional);
	}
#endif
}

#if WITH_SERVER_CODE
AActor* UFlowNode_SpawnActors::TryGetActorOwner() const
{
	return TryGetRootFlowActorOwner();
}

void UFlowNode_SpawnActors::ProcessPreSpawnConfiguration(UFlowActorSpawnRecord& Record)
{
	HandlePreSpawnConfiguration(Record);
}

void UFlowNode_SpawnActors::ProcessPreFinishSpawnActor(UFlowActorSpawnRecord& Record)
{
	HandlePreFinishSpawnActor(Record);
}

void UFlowNode_SpawnActors::ProcessPostSpawnConfiguration(UFlowActorSpawnRecord& Record)
{
	HandlePostSpawnConfiguration(Record);
}

void UFlowNode_SpawnActors::ProcessFinishedSpawnAttempt(UFlowActorSpawnRecord& Record)
{
	HandleFinishedSpawnAttempt(Record);
}

void UFlowNode_SpawnActors::OnCleanupSpawnedInstance(UFlowActorSpawnRecord& Record)
{
	HandleCleanupSpawnedInstance(Record);
}

void UFlowNode_SpawnActors::StartSpawningPass()
{
	FFlowActorSpawningAssistant* Assistant = SpawningAssistant.GetMutablePtr();
	if (!IsValid(SpawnRecordTemplate) || !ActorSpawnSelector.GetPtr<FFlowActorSpawnSelector>() || !Assistant)
	{
		LogError(TEXT("Could not spawn actors: missing world subsystem, selector, or record template."));
		return;
	}

	const FFlowActorSpawningPassConfig Config
		{
			*this,
			ActorSpawnSelector,
			*SpawnRecordTemplate,
			ActorRecords,
			GetDefault<UFlowSettings>()->DefaultActorSpawnQueueMode,
			EFlowActorSpawningMethod::FullSpawn
		};

	const FFlowActorSpawningPassResult Result = Assistant->TryExecuteSpawningPass(Config);
	if (Result.NumQueued == INDEX_NONE)
	{
		LogError(TEXT("Could not spawn actors: missing world subsystem, selector, or record template."));
	}
	else if (Result.NumPrepared > 0 && Result.NumQueued != Result.NumPrepared)
	{
		LogError(TEXT("Failed to queue all configured actor spawn records."));
	}
}

void UFlowNode_SpawnActors::DefeatAllSpawnedActors()
{
	for (UFlowActorSpawnRecord* Record : ActorRecords)
	{
		if (IsValid(Record))
		{
			Record->CleanupRuntime();
		}
	}
}

void UFlowNode_SpawnActors::CleanupActorRecords()
{
	DefeatAllSpawnedActors();
	ActorRecords.Empty();
}

bool UFlowNode_SpawnActors::HasAnyOutstandingSpawns() const
{
	for (const UFlowActorSpawnRecord* Record : ActorRecords)
	{
		if (IsValid(Record) && Record->IsQueuedOrActivelyAttemptingtoSpawn())
		{
			return true;
		}
	}

	return false;
}
#endif

void UFlowNode_SpawnActors::UpdateNodeConfigText_Implementation()
{
#if WITH_EDITOR
	const FFlowActorSpawnSelector* Selector = ActorSpawnSelector.GetPtr<FFlowActorSpawnSelector>();
	SetNodeConfigText(FText::FromString(FString::Printf(TEXT("Spawn %d actors"), Selector ? Selector->GetNumToSpawn() : 0)));
#endif
}

#if WITH_EDITOR
EDataValidationResult UFlowNode_SpawnActors::ValidateNode()
{
	const EDataValidationResult ParentResult = Super::ValidateNode();
	const FFlowActorSpawnSelector* Selector = ActorSpawnSelector.GetPtr<FFlowActorSpawnSelector>();
	if (!IsValid(SpawnRecordTemplate)
		|| Selector == nullptr
		|| (Selector->GetNumToSpawn() > 0 && !IsValid(Selector->ChooseActorClassToSpawn(0))))
	{
		return EDataValidationResult::Invalid;
	}

	return ParentResult;
}
#endif
