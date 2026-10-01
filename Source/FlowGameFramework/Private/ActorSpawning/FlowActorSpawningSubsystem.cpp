// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "ActorSpawning/FlowActorSpawningSubsystem.h"

#include "Engine/World.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/UObjectGlobals.h"

#include "ActorSpawning/FlowActorSpawnRecord.h"
#include "FlowSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowActorSpawningSubsystem)

FFlowActorSpawnQueueEntry::FFlowActorSpawnQueueEntry(UFlowActorSpawnRecord& InRecord)
	: Record(&InRecord)
	, ConfigurationGeneration(InRecord.GetConfigurationGeneration())
{
}

bool UFlowActorSpawningSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = IsValid(Outer) ? Outer->GetWorld() : nullptr;
	if (!IsValid(World))
	{
		return false;
	}

	if (World->GetNetMode() == NM_Client)
	{
		return false;
	}

	return Super::ShouldCreateSubsystem(Outer);
}

void UFlowActorSpawningSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UFlowSettings* Settings = GetDefault<UFlowSettings>();
	MaxSpawnSuccessesPerTick = FMath::Max(1, Settings->MaxActorSpawnSuccessesPerTick);
	MaxSpawnAttemptsPerFlush = FMath::Max(1, Settings->MaxActorSpawnAttemptsPerFlush);
}

void UFlowActorSpawningSubsystem::Deinitialize()
{
	check(!bIsDeinitializing);
	TGuardValue<bool> IsDeinitializingGuard(bIsDeinitializing, true);

#if WITH_SERVER_CODE
	CancelOutstandingSpawnRecords();
#endif

	Super::Deinitialize();
}

void UFlowActorSpawningSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if WITH_SERVER_CODE
	PruneImmediateSpawnRecords();
	FlushImmediateSpawnRecords();
	ProcessPendingSpawnRecords();
#endif
}

bool UFlowActorSpawningSubsystem::IsTickable() const
{
	if (bIsDeinitializing || !IsInitialized())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client)
	{
		return false;
	}

	return !PendingSpawnRecords.IsEmpty()
		|| !ImmediateSpawnRecords.IsEmpty()
		|| !ParkedImmediateSpawnRecords.IsEmpty();
}

TStatId UFlowActorSpawningSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFlowActorSpawningSubsystem, STATGROUP_Tickables);
}

#if WITH_SERVER_CODE
bool UFlowActorSpawningSubsystem::TryEnqueueActorSpawnRecord(UFlowActorSpawnRecord& Record)
{
	return TryEnqueueActorSpawnRecord(Record, GetDefault<UFlowSettings>()->DefaultActorSpawnQueueMode);
}

bool UFlowActorSpawningSubsystem::TryEnqueueActorSpawnRecord(
	UFlowActorSpawnRecord& Record, EFlowActorSpawnQueueMode ModeOverride)
{
	if (!QueueRecord(Record, ModeOverride))
	{
		return false;
	}

	if (ModeOverride == EFlowActorSpawnQueueMode::AttemptOnEnqueue)
	{
		FlushImmediateSpawnRecords();
	}

	return true;
}

int32 UFlowActorSpawningSubsystem::SubmitSpawnPass(
	const UObject& Owner, TConstArrayView<UFlowActorSpawnRecord*> Records)
{
	return SubmitSpawnPass(Owner, Records, GetDefault<UFlowSettings>()->DefaultActorSpawnQueueMode);
}

int32 UFlowActorSpawningSubsystem::SubmitSpawnPass(
	const UObject& Owner, TConstArrayView<UFlowActorSpawnRecord*> Records, EFlowActorSpawnQueueMode ModeOverride)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnQueueMode, 2);
	if (!IsValid(&Owner) || bIsDeinitializing || !FlowEnum::IsValidEnumValue(ModeOverride))
	{
		return 0;
	}

	int32 NumQueued = 0;
	{
		TGuardValue<bool> SubmissionGuard(bIsSubmittingPass, true);
		for (UFlowActorSpawnRecord* Record : Records)
		{
			if (IsValid(Record) && Record->GetOuter() == &Owner && QueueRecord(*Record, ModeOverride))
			{
				++NumQueued;
			}
		}
	}

	if (ModeOverride == EFlowActorSpawnQueueMode::AttemptOnEnqueue)
	{
		FlushImmediateSpawnRecords();
	}

	return NumQueued;
}

bool UFlowActorSpawningSubsystem::QueueRecord(UFlowActorSpawnRecord& Record, EFlowActorSpawnQueueMode Mode)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnQueueMode, 2);
	if (bIsDeinitializing || Record.GetOuter() == CancellingOwner
		|| Record.GetWorld() != GetWorld() || !FlowEnum::IsValidEnumValue(Mode)
		|| !Record.TryQueueRecordForSpawning())
	{
		return false;
	}

	FFlowActorSpawnQueueEntry Entry(Record);
	if (Mode == EFlowActorSpawnQueueMode::AttemptOnEnqueue)
	{
		ImmediateSpawnRecords.Add(Entry);
	}
	else
	{
		PendingSpawnRecords.Add(Entry);
	}

	return true;
}

void UFlowActorSpawningSubsystem::FlushImmediateSpawnRecords()
{
	if (bIsDeinitializing || bIsSubmittingPass || bIsProcessingImmediate)
	{
		return;
	}

	TGuardValue<bool> ProcessingGuard(bIsProcessingImmediate, true);
	const int32 WorkBound = FMath::Max(1, MaxSpawnAttemptsPerFlush);
	int32 Attempts = 0;

	while (!bIsDeinitializing && !ImmediateSpawnRecords.IsEmpty() && Attempts < WorkBound)
	{
		const FFlowActorSpawnQueueEntry Entry = ImmediateSpawnRecords[0];
		ImmediateSpawnRecords.RemoveAt(0);
		UFlowActorSpawnRecord* Record = Entry.Record.Get();
		if (!IsValid(Record)
			|| Record->GetConfigurationGeneration() != Entry.ConfigurationGeneration
			|| !Record->IsQueuedToSpawn())
		{
			continue;
		}

		++Attempts;
		TGuardValue<TObjectPtr<UFlowActorSpawnRecord>> RecordGuard(ProcessingImmediateRecord, Record);
		if (Record->TryStartSpawningProcess()
			&& IsValid(Record)
			&& Record->GetConfigurationGeneration() == Entry.ConfigurationGeneration
			&& Record->IsActivelyAttemptingToSpawn())
		{
			ParkedImmediateSpawnRecords.Add(Entry);
		}
	}
}

bool UFlowActorSpawningSubsystem::HasOutstandingSpawns(const UObject* Owner) const
{
	if (!IsValid(Owner))
	{
		return false;
	}

	for (const UFlowActorSpawnRecord* Active : {ActiveSpawnRecord.Get(), ProcessingImmediateRecord.Get()})
	{
		if (IsValid(Active) && Active->GetOuter() == Owner && Active->IsQueuedOrActivelyAttemptingtoSpawn())
		{
			return true;
		}
	}

	const auto HasPendingOwnerWork = [Owner](const TArray<FFlowActorSpawnQueueEntry>& Entries)
	{
		for (const FFlowActorSpawnQueueEntry& Entry : Entries)
		{
			const UFlowActorSpawnRecord* Record = Entry.Record.Get();
			if (IsValid(Record) && Record->GetOuter() == Owner
				&& Record->GetConfigurationGeneration() == Entry.ConfigurationGeneration
				&& Record->IsQueuedOrActivelyAttemptingtoSpawn())
			{
				return true;
			}
		}
		return false;
	};

	return HasPendingOwnerWork(PendingSpawnRecords)
		|| HasPendingOwnerWork(ImmediateSpawnRecords)
		|| HasPendingOwnerWork(ParkedImmediateSpawnRecords);
}

void UFlowActorSpawningSubsystem::ExtractRecordsForOwner(
	TArray<FFlowActorSpawnQueueEntry>& Records,
	const UObject* Owner,
	TArray<FFlowActorSpawnQueueEntry>& OutRecords)
{
	for (int32 Index = 0; Index < Records.Num();)
	{
		const FFlowActorSpawnQueueEntry& Entry = Records[Index];
		if (!IsValid(Entry.Record) || Entry.Record->GetOuter() != Owner)
		{
			++Index;
			continue;
		}

		OutRecords.Add(Entry);
		Records.RemoveAt(Index);
	}
}

void UFlowActorSpawningSubsystem::CancelSpawnsForOwner(const UObject* Owner)
{
	if (Owner == nullptr || CancellingOwner == Owner)
	{
		return;
	}

	TGuardValue<const UObject*> Guard(CancellingOwner, Owner);
	UFlowActorSpawnRecord* Current = ProcessingImmediateRecord.Get();
	if (IsValid(Current) && Current->GetOuter() == Owner && Current->IsQueuedOrActivelyAttemptingtoSpawn())
	{
		Current->CleanupRuntime();
	}

	TArray<FFlowActorSpawnQueueEntry> ToCancel;
	ExtractRecordsForOwner(PendingSpawnRecords, Owner, ToCancel);
	ExtractRecordsForOwner(ImmediateSpawnRecords, Owner, ToCancel);
	ExtractRecordsForOwner(ParkedImmediateSpawnRecords, Owner, ToCancel);
	UFlowActorSpawnRecord* Active = ActiveSpawnRecord.Get();
	if (IsValid(Active) && Active->GetOuter() == Owner)
	{
		ActiveSpawnRecord = nullptr;
		if (Active->IsQueuedOrActivelyAttemptingtoSpawn())
		{
			ToCancel.Emplace(*Active);
		}
	}

	for (const FFlowActorSpawnQueueEntry& Entry : ToCancel)
	{
		UFlowActorSpawnRecord* Record = Entry.Record.Get();
		if (IsValid(Record) && Record->GetConfigurationGeneration() == Entry.ConfigurationGeneration
			&& Record->IsQueuedOrActivelyAttemptingtoSpawn())
		{
			Record->CleanupRuntime();
		}
	}
}

void UFlowActorSpawningSubsystem::SetMaxSpawnSuccessesPerTick(int32 InMaxSuccesses)
{
	MaxSpawnSuccessesPerTick = FMath::Max(1, InMaxSuccesses);
}

void UFlowActorSpawningSubsystem::SetMaxSpawnAttemptsPerFlush(int32 InMaxAttempts)
{
	MaxSpawnAttemptsPerFlush = FMath::Max(1, InMaxAttempts);
}

void UFlowActorSpawningSubsystem::CancelOutstandingSpawnRecords()
{
	UFlowActorSpawnRecord* Current = ProcessingImmediateRecord.Get();
	if (IsValid(Current) && Current->IsQueuedOrActivelyAttemptingtoSpawn())
	{
		Current->CleanupRuntime();
	}

	UFlowActorSpawnRecord* Active = ActiveSpawnRecord.Get();
	ActiveSpawnRecord = nullptr;

	// Move pending work out before callbacks, so a callback cannot mutate the arrays being visited.
	TArray<FFlowActorSpawnQueueEntry> RecordsToCancel = MoveTemp(PendingSpawnRecords);
	RecordsToCancel.Append(ImmediateSpawnRecords);
	RecordsToCancel.Append(ParkedImmediateSpawnRecords);
	ImmediateSpawnRecords.Reset();
	ParkedImmediateSpawnRecords.Reset();

	for (const FFlowActorSpawnQueueEntry& Entry : RecordsToCancel)
	{
		UFlowActorSpawnRecord* Record = Entry.Record.Get();
		if (IsValid(Record) && Record != Active
			&& Record->GetConfigurationGeneration() == Entry.ConfigurationGeneration
			&& Record->IsQueuedOrActivelyAttemptingtoSpawn())
		{
			Record->CleanupRuntime();
		}
	}

	// Successful actors belong to their owners; cancel only work still queued or active.
	if (IsValid(Active) && Active->IsQueuedOrActivelyAttemptingtoSpawn())
	{
		Active->CleanupRuntime();
	}
}

void UFlowActorSpawningSubsystem::PruneImmediateSpawnRecords()
{
	ImmediateSpawnRecords.RemoveAll([](const FFlowActorSpawnQueueEntry& Entry)
	{
		const UFlowActorSpawnRecord* Record = Entry.Record.Get();
		return !IsValid(Record) || Record->GetConfigurationGeneration() != Entry.ConfigurationGeneration
			|| !Record->IsQueuedToSpawn();
	});

	ParkedImmediateSpawnRecords.RemoveAll([](const FFlowActorSpawnQueueEntry& Entry)
	{
		const UFlowActorSpawnRecord* Record = Entry.Record.Get();
		return !IsValid(Record) || Record->GetConfigurationGeneration() != Entry.ConfigurationGeneration
			|| !Record->IsActivelyAttemptingToSpawn();
	});
}

void UFlowActorSpawningSubsystem::ProcessPendingSpawnRecords()
{
	// An asynchronous staggered location request retains its turn until completion or cancellation.
	if (IsValid(ActiveSpawnRecord) && ActiveSpawnRecord->IsActivelyAttemptingToSpawn())
	{
		return;
	}

	// Clear completed work before starting the next staggered record.
	ActiveSpawnRecord = nullptr;
	const int32 SuccessBudget = FMath::Max(1, MaxSpawnSuccessesPerTick);
	int32 ImmediateSuccessCount = 0;

	// Drain FIFO work until the success budget is spent or a new asynchronous attempt begins.
	while (!PendingSpawnRecords.IsEmpty())
	{
		const FFlowActorSpawnQueueEntry Entry = PendingSpawnRecords[0];
		PendingSpawnRecords.RemoveAt(0);
		ActiveSpawnRecord = Entry.Record;

		if (!IsValid(ActiveSpawnRecord))
		{
			ActiveSpawnRecord = nullptr;
			continue;
		}

		if (ActiveSpawnRecord->GetConfigurationGeneration() != Entry.ConfigurationGeneration)
		{
			ActiveSpawnRecord = nullptr;
			continue;
		}

		if (!ActiveSpawnRecord->TryStartSpawningProcess())
		{
			ActiveSpawnRecord = nullptr;
			continue;
		}

		if (!IsValid(ActiveSpawnRecord) || ActiveSpawnRecord->GetConfigurationGeneration() != Entry.ConfigurationGeneration)
		{
			ActiveSpawnRecord = nullptr;
			continue;
		}

		if (ActiveSpawnRecord->IsSpawnSuccessful())
		{
			++ImmediateSuccessCount;

			if (ImmediateSuccessCount >= SuccessBudget)
			{
				break;
			}
		}
		else if (!ActiveSpawnRecord->IsSpawnFailed())
		{
			break;
		}
	}
}
#endif
