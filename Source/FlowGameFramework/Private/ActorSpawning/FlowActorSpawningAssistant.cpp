// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "ActorSpawning/FlowActorSpawningAssistant.h"

#include "ActorSpawning/FlowActorSpawnRecord.h"
#include "ActorSpawning/FlowActorSpawnSelector.h"
#include "ActorSpawning/FlowActorSpawningSubsystem.h"
#include "Containers/Array.h"
#include "Containers/Set.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowActorSpawningAssistant)

#if WITH_SERVER_CODE
void FFlowActorSpawningAssistant::StartPass(FFlowActorSpawningPassContext& Context)
{
	if (FFlowActorSpawnSelector* Selector = Context.Config.Selector.GetMutablePtr<FFlowActorSpawnSelector>())
	{
		Selector->StartSpawningPass();
	}
}

int32 FFlowActorSpawningAssistant::GetNumToSpawn(FFlowActorSpawningPassContext& Context)
{
	const FFlowActorSpawnSelector* Selector = Context.Config.Selector.GetPtr<FFlowActorSpawnSelector>();
	return Selector ? Selector->GetNumToSpawn() : INDEX_NONE;
}

int32 FFlowActorSpawningAssistant::SelectRecordIndex(FFlowActorSpawningPassContext& Context, int32 SourceIndex,
	int32 NumToSpawn)
{
	const int32 ChosenIndex = ChooseRecordIndex(SourceIndex, NumToSpawn, Context.Config.Method, Context.Config.Records);
	return ChosenIndex == INDEX_NONE ? INDEX_NONE : FMath::Min(ChosenIndex, Context.Config.Records.Num());
}

TSubclassOf<AActor> FFlowActorSpawningAssistant::ChooseActorClass(FFlowActorSpawningPassContext& Context,
	int32 RecordIndex)
{
	const FFlowActorSpawnSelector* Selector = Context.Config.Selector.GetPtr<FFlowActorSpawnSelector>();
	return Selector ? Selector->ChooseActorClassToSpawn(RecordIndex) : nullptr;
}

UFlowActorSpawnRecord* FFlowActorSpawningAssistant::PrepareRecord(FFlowActorSpawningPassContext& Context,
	int32 RecordIndex, TSubclassOf<AActor> ActorClass)
{
	const FFlowActorSpawningPassConfig& Config = Context.Config;

	UFlowActorSpawnRecord* ExistingRecord = nullptr;
	if (Config.Records.IsValidIndex(RecordIndex))
	{
		ExistingRecord = Config.Records[RecordIndex].Get();
	}

	if (IsValid(ExistingRecord))
	{
		if (!ExistingRecord->Configure(ActorClass, RecordIndex, true))
		{
			return nullptr;
		}

		++Context.NumPrepared;
		return ExistingRecord;
	}

	UClass* SpawnClass = Config.RecordTemplate.GetClass();
	const FName RecordName = MakeUniqueObjectName(&Config.Owner, SpawnClass, Config.RecordTemplate.GetFName());

	UFlowActorSpawnRecord* Record = NewObject<UFlowActorSpawnRecord>(
		&Config.Owner, SpawnClass, RecordName, RF_NoFlags, &Config.RecordTemplate);

	if (IsValid(Record) && Record->Configure(ActorClass, RecordIndex))
	{
		if (Config.Records.IsValidIndex(RecordIndex))
		{
			Config.Records[RecordIndex] = Record;
		}
		else
		{
			Config.Records.Add(Record);
		}

		Context.NewRecords.Add(Record);
		++Context.NumPrepared;

		return Record;
	}
	return nullptr;
}

void FFlowActorSpawningAssistant::OnRejectedRecord(FFlowActorSpawningPassContext& Context,
	UFlowActorSpawnRecord& Record)
{
	if (Context.NewRecords.Contains(&Record))
	{
		for (TObjectPtr<UFlowActorSpawnRecord>& Entry : Context.Config.Records)
		{
			if (Entry.Get() == &Record)
			{
				Entry = nullptr;
				break;
			}
		}

		while (!Context.Config.Records.IsEmpty() && !IsValid(Context.Config.Records.Last()))
		{
			Context.Config.Records.Pop();
		}
	}
}

void FFlowActorSpawningAssistant::FinishPass(FFlowActorSpawningPassContext& Context)
{
	if (FFlowActorSpawnSelector* Selector = Context.Config.Selector.GetMutablePtr<FFlowActorSpawnSelector>())
	{
		Selector->FinishSpawningPass();
	}
}

FFlowActorSpawningPassResult FFlowActorSpawningAssistant::TryExecuteSpawningPass(const FFlowActorSpawningPassConfig& Config)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawningMethod, 3);
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnQueueMode, 2);

	FFlowActorSpawningPassResult Result;
	if (!FlowEnum::IsValidEnumValue(Config.QueueMode) || !FlowEnum::IsValidEnumValue(Config.Method))
	{
		return Result;
	}

	UWorld* World = Config.Owner.GetWorld();
	UFlowActorSpawningSubsystem* Subsystem = IsValid(World) ? World->GetSubsystem<UFlowActorSpawningSubsystem>() : nullptr;
	if (!IsValid(Subsystem))
	{
		return Result;
	}

	FFlowActorSpawningPassContext Context{Config};
	StartPass(Context);
	const int32 StandardGroupNum = GetNumToSpawn(Context);
	if (StandardGroupNum < 0)
	{
		FinishPass(Context);
		return Result;
	}

	// Limit the NumToSpawn to a single enemy
	const int32 NumToSpawn =
		(Config.Method == EFlowActorSpawningMethod::RefillSingleMissing)
			? FMath::Min(1, StandardGroupNum)
			: StandardGroupNum;

	Result.NumQueued = 0;
	Context.RecordsToSubmit.Reserve(NumToSpawn);
	for (int32 Index = 0; Index < StandardGroupNum; ++Index)
	{
		if (Context.RecordsToSubmit.Num() >= NumToSpawn)
		{
			break;
		}

		int32 RecordIndex = SelectRecordIndex(Context, Index, NumToSpawn);
		if (RecordIndex == INDEX_NONE)
		{
			// If we have elected not to respawn this actor record, skip it.
			continue;
		}
		while (Context.PreparedRecordIndices.Contains(RecordIndex))
		{
			++RecordIndex;
		}

		const TSubclassOf<AActor> ActorClass = ChooseActorClass(Context, RecordIndex);
		if (!IsValid(ActorClass))
		{
			continue;
		}

		UFlowActorSpawnRecord* Record = PrepareRecord(Context, RecordIndex, ActorClass);
		if (!IsValid(Record))
		{
			continue;
		}

		if (Record->GetOuter() != &Config.Owner)
		{
			OnRejectedRecord(Context, *Record);
			continue;
		}

		Context.RecordsToSubmit.Add(Record);
		Context.PreparedRecordIndices.Add(RecordIndex);
	}

	FinishPass(Context);

	if (Config.QueueMode == EFlowActorSpawnQueueMode::Staggered)
	{
		for (UFlowActorSpawnRecord* Record : Context.RecordsToSubmit)
		{
			if (IsValid(Record) && Subsystem->TryEnqueueActorSpawnRecord(*Record, Config.QueueMode))
			{
				++Result.NumQueued;
			}
			else if (IsValid(Record))
			{
				OnRejectedRecord(Context, *Record);
			}
		}
	}
	else if (!Context.RecordsToSubmit.IsEmpty())
	{
		Result.NumQueued = Subsystem->SubmitSpawnPass(Config.Owner, Context.RecordsToSubmit, Config.QueueMode);
	}

	Result.NumPrepared = Context.NumPrepared;
	return Result;
}

int32 FFlowActorSpawningAssistant::ChooseRecordIndex(int32 SourceIndex, int32 NumToSpawn,
	EFlowActorSpawningMethod Method, const TArray<TObjectPtr<UFlowActorSpawnRecord>>& Records)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawningMethod, 3);

	if (SourceIndex < 0 || NumToSpawn < 0 || !FlowEnum::IsValidEnumValue(Method))
	{
		return INDEX_NONE;
	}

	const int32 RecordCount = Records.Num();
	if (SourceIndex >= RecordCount || !IsValid(Records[SourceIndex].Get()))
	{
		return SourceIndex;
	}

	const UFlowActorSpawnRecord* Record = Records[SourceIndex].Get();
	if (Record->IsQueuedOrActivelyAttemptingtoSpawn())
	{
		return INDEX_NONE;
	}

	if (!Record->IsSpawnedActorAlive())
	{
		return SourceIndex;
	}

	if (Method != EFlowActorSpawningMethod::FullSpawn)
	{
		return INDEX_NONE;
	}

	for (int32 Index = NumToSpawn; Index < RecordCount; ++Index)
	{
		const UFlowActorSpawnRecord* Candidate = Records[Index].Get();
		if (!IsValid(Candidate)
			|| (!Candidate->IsQueuedOrActivelyAttemptingtoSpawn() && !Candidate->IsSpawnedActorAlive()))
		{
			return Index;
		}
	}

	return RecordCount;
}
#endif
