// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "ActorSpawning/Tests/FlowActorSpawningTestTypes.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "StructUtils/InstancedStruct.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/UObjectGlobals.h"

#include "ActorSpawning/FlowActorSpawningAssistant.h"
#include "ActorSpawning/FlowActorSpawningSubsystem.h"
#include "FlowSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowActorSpawningTestTypes)

#if WITH_DEV_AUTOMATION_TESTS && WITH_SERVER_CODE && WITH_EDITOR

namespace FlowActorSpawningTests
{
	struct FFixture
	{
		UWorld* World = nullptr;
		UGameInstance* GameInstance = nullptr;
		AFlowActorSpawningTestOwner* Owner = nullptr;
		UFlowActorSpawningSubsystem* Scheduler = nullptr;

		FFixture()
		{
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::PIE);
			World = UWorld::CreateWorld(EWorldType::PIE, false);
			GameInstance = NewObject<UGameInstance>();
			GameInstance->AddToRoot();
			World->SetGameInstance(GameInstance);
			Context.SetCurrentWorld(World);
			const FURL URL(TEXT("Template_Default"));
			World->SetGameMode(URL);
			World->InitializeActorsForPlay(URL);
			World->BeginPlay();
			Owner = World->SpawnActor<AFlowActorSpawningTestOwner>();
			Scheduler = World->GetSubsystem<UFlowActorSpawningSubsystem>();
		}

		~FFixture()
		{
			GameInstance->RemoveFromRoot();
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		template <typename TRecord = UFlowDefaultActorSpawnRecord>
		TRecord* CreateRecord(int32 Index)
		{
			TRecord* Record = NewObject<TRecord>(Owner);
			Owner->Records.Add(Record);
			check(Record->Configure(AActor::StaticClass(), Index));
			return Record;
		}

	};

	struct FFinishPassProbe : FFlowActorSpawningAssistant
	{
		bool bQueuedDuringFinish = false;

		virtual void FinishPass(FFlowActorSpawningPassContext& Context) override
		{
			bQueuedDuringFinish = Context.RecordsToSubmit.ContainsByPredicate(
				[](const UFlowActorSpawnRecord* Record) { return Record->IsQueuedToSpawn(); });
			FFlowActorSpawningAssistant::FinishPass(Context);
		}
	};
}

BEGIN_DEFINE_SPEC(
	FFlowActorSpawningSpec,
	"Flow.GameFramework.ActorSpawning",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext)
END_DEFINE_SPEC(FFlowActorSpawningSpec)

void FFlowActorSpawningSpec::Define()
{
	using namespace FlowActorSpawningTests;

	It("queues a vanilla deferred actor without running callbacks until tick", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		UFlowDefaultActorSpawnRecord* Record = Fixture.CreateRecord(0);
		TestTrue(TEXT("Queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Record, EFlowActorSpawnQueueMode::Staggered));
		TestTrue(TEXT("No synchronous callbacks"), Fixture.Owner->Events.IsEmpty());
		TestTrue(TEXT("Outstanding before tick"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Successful actor"), Record->IsSpawnSuccessful() && IsValid(Record->GetOwnedActor()));
		TestTrue(TEXT("Actor exists before finish"), Fixture.Owner->bSawActorBeforeFinish);
		TestTrue(TEXT("Actor begins play before post-spawn"), Fixture.Owner->bSawActorAfterFinish);
		TestTrue(TEXT("Callback order"), Fixture.Owner->Events == TArray<FName>{
			TEXT("PreSpawn"), TEXT("PreFinish"), TEXT("PostSpawn"), TEXT("Finished")});
		TestFalse(TEXT("No outstanding attempts"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));

		Record->CleanupRuntime();
		Record->CleanupRuntime();
		TestNull(TEXT("Owned actor released"), Record->GetOwnedActor());
		TestEqual(TEXT("Cleanup once"), Fixture.Owner->CleanupCount, 1);
		TestTrue(TEXT("Owner saw actor during cleanup"), Fixture.Owner->bSawActorDuringCleanup);
	});

	It("limits quick successes to two per tick without charging fast failures", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		Fixture.Scheduler->SetMaxSpawnSuccessesPerTick(2);
		auto* First = Fixture.CreateRecord(0);
		auto* Failed = Fixture.CreateRecord<UFlowFailingActorSpawnTestRecord>(1);
		auto* Second = Fixture.CreateRecord(2);
		auto* Third = Fixture.CreateRecord(3);
		TestTrue(TEXT("First queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*First, EFlowActorSpawnQueueMode::Staggered));
		TestTrue(TEXT("Failure queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Failed, EFlowActorSpawnQueueMode::Staggered));
		TestTrue(TEXT("Second queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Second, EFlowActorSpawnQueueMode::Staggered));
		TestTrue(TEXT("Third queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Third, EFlowActorSpawnQueueMode::Staggered));
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("First succeeded"), First->IsSpawnSuccessful());
		TestTrue(TEXT("Fast failure processed"), Failed->IsSpawnFailed());
		TestTrue(TEXT("Second succeeded"), Second->IsSpawnSuccessful());
		TestTrue(TEXT("Third waits"), Third->IsQueuedToSpawn());
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Third succeeded next tick"), Third->IsSpawnSuccessful());
	});

	It("rejects an invalid queue mode without transitioning the record", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		UFlowDefaultActorSpawnRecord* Record = Fixture.CreateRecord(0);
		TestFalse(TEXT("Invalid mode rejected"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Record, EFlowActorSpawnQueueMode::Invalid));
		TestFalse(TEXT("Record was not queued"), Record->IsQueuedToSpawn());
		TestTrue(TEXT("Configured record can still queue normally"),
			Fixture.Scheduler->TryEnqueueActorSpawnRecord(*Record));
	});

	It("applies a configured success budget", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		Fixture.Scheduler->SetMaxSpawnSuccessesPerTick(1);
		auto* First = Fixture.CreateRecord(0);
		auto* Second = Fixture.CreateRecord(1);
		TestTrue(TEXT("First queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*First, EFlowActorSpawnQueueMode::Staggered));
		TestTrue(TEXT("Second queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Second, EFlowActorSpawnQueueMode::Staggered));
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("First succeeds"), First->IsSpawnSuccessful());
		TestTrue(TEXT("Second stays queued"), Second->IsQueuedToSpawn());
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Second succeeds next tick"), Second->IsSpawnSuccessful());
	});

	It("loads staggered and flush budgets from Flow settings", [this]
	{
		UFlowSettings* Settings = GetMutableDefault<UFlowSettings>();
		TGuardValue<EFlowActorSpawnQueueMode> ModeGuard(
			Settings->DefaultActorSpawnQueueMode, EFlowActorSpawnQueueMode::Staggered);
		TGuardValue<int32> SuccessBudget(Settings->MaxActorSpawnSuccessesPerTick, 1);
		TGuardValue<int32> FlushBudget(Settings->MaxActorSpawnAttemptsPerFlush, 1);
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		auto* First = Fixture.CreateRecord(0);
		auto* Second = Fixture.CreateRecord(1);
		TestTrue(TEXT("Default mode queues first record"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(*First));
		TestTrue(TEXT("Default mode queues second record"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(*Second));
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Configured tick budget allows one success"), First->IsSpawnSuccessful());
		TestTrue(TEXT("Second remains queued"), Second->IsQueuedToSpawn());

		auto* Third = Fixture.CreateRecord(2);
		auto* Fourth = Fixture.CreateRecord(3);
		TArray<UFlowActorSpawnRecord*> Batch{Third, Fourth};
		TestEqual(TEXT("Immediate batch accepted"), Fixture.Scheduler->SubmitSpawnPass(
			*Fixture.Owner, Batch, EFlowActorSpawnQueueMode::AttemptOnEnqueue), 2);
		TestTrue(TEXT("Configured flush budget allows one success"), Third->IsSpawnSuccessful());
		TestTrue(TEXT("Next immediate record remains queued"), Fourth->IsQueuedToSpawn());
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Immediate overflow continues next tick"), Fourth->IsSpawnSuccessful());
		TestTrue(TEXT("Staggered record completes next tick"), Second->IsSpawnSuccessful());
	});

	It("honors the Flow setting when a single record is enqueued", [this]
	{
		UFlowSettings* Settings = GetMutableDefault<UFlowSettings>();
		TGuardValue<EFlowActorSpawnQueueMode> ModeGuard(
			Settings->DefaultActorSpawnQueueMode, EFlowActorSpawnQueueMode::AttemptOnEnqueue);
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		auto* Record = Fixture.CreateRecord(0);
		TestTrue(TEXT("Record accepted"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(*Record));
		TestTrue(TEXT("Spawn succeeds before enqueue returns"), Record->IsSpawnSuccessful());
	});

	It("keeps an explicit staggered record deferred when the Flow default is immediate", [this]
	{
		UFlowSettings* Settings = GetMutableDefault<UFlowSettings>();
		TGuardValue<EFlowActorSpawnQueueMode> ModeGuard(
			Settings->DefaultActorSpawnQueueMode, EFlowActorSpawnQueueMode::AttemptOnEnqueue);
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		auto* Record = Fixture.CreateRecord(0);
		TestTrue(TEXT("Record accepted"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Record, EFlowActorSpawnQueueMode::Staggered));
		TestTrue(TEXT("Explicit staggered attempt remains queued"), Record->IsQueuedToSpawn());
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Attempt succeeds on tick"), Record->IsSpawnSuccessful());
	});

	It("skips a pending record cancelled before its tick", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		UFlowDefaultActorSpawnRecord* Record = Fixture.CreateRecord(0);
		TestTrue(TEXT("Queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Record, EFlowActorSpawnQueueMode::Staggered));
		Record->CleanupRuntime();
		Fixture.Scheduler->Tick(0.0f);
		TestNull(TEXT("No actor acquired"), Record->GetOwnedActor());
		TestEqual(TEXT("No finished attempt"), Fixture.Owner->FinishedCount, 0);
		TestFalse(TEXT("No outstanding work"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));
	});

	It("blocks later work while a location request is active", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		auto* Waiting = Fixture.CreateRecord<UFlowWaitingActorSpawnTestRecord>(0);
		auto* Follower = Fixture.CreateRecord(1);
		TestTrue(TEXT("Waiting queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Waiting, EFlowActorSpawnQueueMode::Staggered));
		TestTrue(TEXT("Follower queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Follower, EFlowActorSpawnQueueMode::Staggered));
		Fixture.Scheduler->Tick(0.0f);
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Waiting remains active"), Waiting->IsActivelyAttemptingToSpawn());
		TestTrue(TEXT("Follower remains pending"), Follower->IsQueuedToSpawn());
		TestTrue(TEXT("Outstanding async attempt"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));
		Waiting->CompleteLocation(false);
		TestTrue(TEXT("Location failure"), Waiting->IsSpawnFailed());
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Follower runs after failure"), Follower->IsSpawnSuccessful());
	});

	It("cancels an acquired deferred actor when pre-finish work cleans up", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		Fixture.Owner->bCancelBeforeFinish = true;
		UFlowDefaultActorSpawnRecord* Record = Fixture.CreateRecord(0);
		TestTrue(TEXT("Queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Record, EFlowActorSpawnQueueMode::Staggered));
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Pre-finish saw deferred actor"), Fixture.Owner->bSawActorBeforeFinish);
		TestNull(TEXT("Deferred actor released"), Record->GetOwnedActor());
		TestEqual(TEXT("Exactly one cleanup callback"), Fixture.Owner->CleanupCount, 1);
		TestEqual(TEXT("No successful attempt reported"), Fixture.Owner->FinishedCount, 0);
		TestFalse(TEXT("Cancelled work is not outstanding"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));
	});

	It("queues a complete immediate pass before callbacks can observe outstanding work", [this]
	{
		UFlowSettings* Settings = GetMutableDefault<UFlowSettings>();
		TGuardValue<EFlowActorSpawnQueueMode> ModeGuard(
			Settings->DefaultActorSpawnQueueMode, EFlowActorSpawnQueueMode::AttemptOnEnqueue);
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		auto* First = Fixture.CreateRecord(0);
		auto* Second = Fixture.CreateRecord(1);
		bool bSecondWasOutstanding = false;
		Fixture.Owner->FinishedCallback = [&](UFlowActorSpawnRecord& Record)
		{
			if (&Record == First)
			{
				bSecondWasOutstanding = Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner);
			}
		};

		TArray<UFlowActorSpawnRecord*> Records{First, Second};
		TestEqual(TEXT("Both records accepted"), Fixture.Scheduler->SubmitSpawnPass(*Fixture.Owner, Records), 2);
		TestTrue(TEXT("Second record visible to first callback"), bSecondWasOutstanding);
		TestTrue(TEXT("Both succeeded before submission returns"),
			First->IsSpawnSuccessful() && Second->IsSpawnSuccessful());
		TestFalse(TEXT("Pass has no outstanding work"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));
	});

	It("keeps a staggered batch deferred until its normal tick", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		Fixture.Scheduler->SetMaxSpawnSuccessesPerTick(2);
		auto* First = Fixture.CreateRecord(0);
		auto* Second = Fixture.CreateRecord(1);
		TArray<UFlowActorSpawnRecord*> Records{First, Second};
		TestEqual(TEXT("Both records accepted"), Fixture.Scheduler->SubmitSpawnPass(
			*Fixture.Owner, Records, EFlowActorSpawnQueueMode::Staggered), 2);
		TestTrue(TEXT("No callbacks during submission"), Fixture.Owner->Events.IsEmpty());
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Both finish under normal tick budget"),
			First->IsSpawnSuccessful() && Second->IsSpawnSuccessful());
	});

	It("does not submit another owner's record in an immediate batch", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		auto* OtherOwner = Fixture.World->SpawnActor<AFlowActorSpawningTestOwner>();
		TestNotNull(TEXT("Other owner"), OtherOwner);
		if (!OtherOwner)
		{
			return;
		}

		auto* Owned = Fixture.CreateRecord(0);
		auto* Foreign = NewObject<UFlowDefaultActorSpawnRecord>(OtherOwner);
		OtherOwner->Records.Add(Foreign);
		TestTrue(TEXT("Foreign configured"), Foreign->Configure(AActor::StaticClass(), 0));
		TArray<UFlowActorSpawnRecord*> Records{Owned, Foreign};
		TestEqual(TEXT("Only owned record accepted"), Fixture.Scheduler->SubmitSpawnPass(
			*Fixture.Owner, Records, EFlowActorSpawnQueueMode::AttemptOnEnqueue), 1);
		TestTrue(TEXT("Owned record started"), Owned->IsSpawnSuccessful());
		TestFalse(TEXT("Foreign record not queued"), Foreign->IsQueuedToSpawn());
	});

	It("parks async attempts and continues immediate overflow on the next tick", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		Fixture.Scheduler->SetMaxSpawnAttemptsPerFlush(2);
		auto* Waiting = Fixture.CreateRecord<UFlowWaitingActorSpawnTestRecord>(0);
		auto* Ready = Fixture.CreateRecord(1);
		auto* Failed = Fixture.CreateRecord<UFlowFailingActorSpawnTestRecord>(2);
		TArray<UFlowActorSpawnRecord*> Records{Waiting, Ready, Failed};
		TestEqual(TEXT("Three records accepted"), Fixture.Scheduler->SubmitSpawnPass(
			*Fixture.Owner, Records, EFlowActorSpawnQueueMode::AttemptOnEnqueue), 3);
		TestTrue(TEXT("Async attempt parked"), Waiting->IsActivelyAttemptingToSpawn());
		TestTrue(TEXT("Follower succeeded without waiting"), Ready->IsSpawnSuccessful());
		TestTrue(TEXT("Failure retained at the bound"), Failed->IsQueuedToSpawn());
		TestTrue(TEXT("Outstanding includes parked work"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Failure continued without manual flush"), Failed->IsSpawnFailed());
		Waiting->CompleteLocation(true);
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Async attempt resumed"), Waiting->IsSpawnSuccessful());
		TestFalse(TEXT("No outstanding work"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));
	});

	It("never executes a stale queue entry after a record is reconfigured", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		Fixture.Scheduler->SetMaxSpawnAttemptsPerFlush(1);
		auto* First = Fixture.CreateRecord(0);
		auto* Reused = Fixture.CreateRecord(1);
		TArray<UFlowActorSpawnRecord*> Records{First, Reused};
		TestEqual(TEXT("Initial pass accepted"), Fixture.Scheduler->SubmitSpawnPass(
			*Fixture.Owner, Records, EFlowActorSpawnQueueMode::AttemptOnEnqueue), 2);
		TestTrue(TEXT("Older attempt remains queued"), Reused->IsQueuedToSpawn());
		TestTrue(TEXT("Record reused"), Reused->Configure(AActor::StaticClass(), 1, true));
		TestTrue(TEXT("New attempt accepted"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Reused, EFlowActorSpawnQueueMode::AttemptOnEnqueue));
		TestTrue(TEXT("New generation succeeds"), Reused->IsSpawnSuccessful());
		TestEqual(TEXT("One result per configured record"), Fixture.Owner->FinishedCount, 2);
	});

	It("defers nested immediate processing until the current callback returns", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		auto* First = Fixture.CreateRecord(0);
		auto* Cancelled = Fixture.CreateRecord(1);
		UFlowDefaultActorSpawnRecord* NewRecord = nullptr;
		bool bNestedAccepted = false;
		bool bStartedInsideCallback = false;
		Fixture.Owner->FinishedCallback = [&](UFlowActorSpawnRecord& Record)
		{
			if (NewRecord)
			{
				return;
			}
			Record.CleanupRuntime();
			Cancelled->CleanupRuntime();
			NewRecord = Fixture.CreateRecord(2);
			bNestedAccepted = Fixture.Scheduler->TryEnqueueActorSpawnRecord(
				*NewRecord, EFlowActorSpawnQueueMode::AttemptOnEnqueue);
			bStartedInsideCallback = NewRecord->IsSpawnSuccessful();
		};

		TArray<UFlowActorSpawnRecord*> Records{First, Cancelled};
		TestEqual(TEXT("Initial pass accepted"), Fixture.Scheduler->SubmitSpawnPass(
			*Fixture.Owner, Records, EFlowActorSpawnQueueMode::AttemptOnEnqueue), 2);
		TestTrue(TEXT("Nested enqueue accepted"), bNestedAccepted);
		TestFalse(TEXT("Nested attempt does not recursively execute"), bStartedInsideCallback);
		TestFalse(TEXT("Cleaned record no longer queued"), Cancelled->IsQueuedToSpawn());
		TestTrue(TEXT("New attempt processed after callback"), NewRecord && NewRecord->IsSpawnSuccessful());
	});

	It("continues reentrant immediate work on the next tick after reaching the flush bound", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		Fixture.Scheduler->SetMaxSpawnAttemptsPerFlush(1);
		auto* First = Fixture.CreateRecord(0);
		UFlowDefaultActorSpawnRecord* NewRecord = nullptr;
		Fixture.Owner->FinishedCallback = [&](UFlowActorSpawnRecord&)
		{
			if (!NewRecord)
			{
				NewRecord = Fixture.CreateRecord(1);
				TestTrue(TEXT("Reentrant record accepted"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
					*NewRecord, EFlowActorSpawnQueueMode::AttemptOnEnqueue));
			}
		};

		TestTrue(TEXT("First accepted"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*First, EFlowActorSpawnQueueMode::AttemptOnEnqueue));
		TestNotNull(TEXT("Callback queued another record"), NewRecord);
		if (NewRecord)
		{
			TestTrue(TEXT("Overflow remains queued"), NewRecord->IsQueuedToSpawn());
			Fixture.Scheduler->Tick(0.0f);
			TestTrue(TEXT("Overflow continues without another enqueue"), NewRecord->IsSpawnSuccessful());
		}
	});

	It("skips a reused record's stale entry during an immediate pass", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		auto* First = Fixture.CreateRecord(0);
		auto* Reused = Fixture.CreateRecord(1);
		bool bRequeued = false;
		Fixture.Owner->FinishedCallback = [&](UFlowActorSpawnRecord& Record)
		{
			if (bRequeued)
			{
				return;
			}
			TestTrue(TEXT("Record reused during callback"), Reused->Configure(AActor::StaticClass(), 1, true));
			bRequeued = Fixture.Scheduler->TryEnqueueActorSpawnRecord(
				*Reused, EFlowActorSpawnQueueMode::AttemptOnEnqueue);
		};

		TArray<UFlowActorSpawnRecord*> Records{First, Reused};
		TestEqual(TEXT("Initial pass accepted"), Fixture.Scheduler->SubmitSpawnPass(
			*Fixture.Owner, Records, EFlowActorSpawnQueueMode::AttemptOnEnqueue), 2);
		TestTrue(TEXT("Reused record requeued"), bRequeued);
		TestTrue(TEXT("New generation succeeds"), Reused->IsSpawnSuccessful());
		TestEqual(TEXT("Stale generation did not finish"), Fixture.Owner->FinishedCount, 2);
	});

	It("cancels only one owner's parked and queued attempts", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		auto* OtherOwner = Fixture.World->SpawnActor<AFlowActorSpawningTestOwner>();
		TestNotNull(TEXT("Other owner"), OtherOwner);
		if (!OtherOwner)
		{
			return;
		}

		Fixture.Scheduler->SetMaxSpawnAttemptsPerFlush(1);
		auto* Waiting = Fixture.CreateRecord<UFlowWaitingActorSpawnTestRecord>(0);
		auto* Queued = Fixture.CreateRecord(1);
		auto* Other = NewObject<UFlowDefaultActorSpawnRecord>(OtherOwner);
		OtherOwner->Records.Add(Other);
		TestTrue(TEXT("Other configured"), Other->Configure(AActor::StaticClass(), 0));
		TArray<UFlowActorSpawnRecord*> Records{Waiting, Queued};
		TestEqual(TEXT("Owner pass accepted"), Fixture.Scheduler->SubmitSpawnPass(
			*Fixture.Owner, Records, EFlowActorSpawnQueueMode::AttemptOnEnqueue), 2);
		TestTrue(TEXT("Other owner queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Other, EFlowActorSpawnQueueMode::Staggered));
		TestTrue(TEXT("Parked attempt outstanding"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));
		Fixture.Scheduler->CancelSpawnsForOwner(Fixture.Owner);
		TestFalse(TEXT("Owner has no outstanding work"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));
		TestFalse(TEXT("Parked attempt cancelled"), Waiting->IsActivelyAttemptingToSpawn());
		TestFalse(TEXT("Pending attempt cancelled"), Queued->IsQueuedToSpawn());
		TestEqual(TEXT("No terminal callback for cancellation"), Fixture.Owner->FinishedCount, 0);
		TestTrue(TEXT("Other owner unaffected"), Other->IsQueuedToSpawn());
		Fixture.Scheduler->Tick(0.0f);
		TestTrue(TEXT("Other owner succeeds on tick"), Other->IsSpawnSuccessful());
	});

	It("counts queued batch work during callbacks and cancels it on owner cleanup", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		auto* First = Fixture.CreateRecord(0);
		auto* Next = Fixture.CreateRecord(1);
		bool bOutstandingDuringCallback = false;
		Fixture.Owner->FinishedCallback = [&](UFlowActorSpawnRecord& Record)
		{
			bOutstandingDuringCallback = Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner);
			Fixture.Scheduler->CancelSpawnsForOwner(Fixture.Owner);
		};

		TArray<UFlowActorSpawnRecord*> Records{First, Next};
		TestEqual(TEXT("Both records accepted"), Fixture.Scheduler->SubmitSpawnPass(
			*Fixture.Owner, Records, EFlowActorSpawnQueueMode::AttemptOnEnqueue), 2);
		TestTrue(TEXT("Follower counted during callback"), bOutstandingDuringCallback);
		TestFalse(TEXT("Follower cancelled"), Next->IsQueuedToSpawn());
		TestFalse(TEXT("No outstanding work after owner cleanup"), Fixture.Scheduler->HasOutstandingSpawns(Fixture.Owner));
	});

	It("clears ownership when the spawned actor is destroyed externally", [this]
	{
		FFixture Fixture;
		TestNotNull(TEXT("Standalone Flow scheduler"), Fixture.Scheduler);
		if (!Fixture.Scheduler)
		{
			return;
		}

		UFlowDefaultActorSpawnRecord* Record = Fixture.CreateRecord(0);
		TestTrue(TEXT("Queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Record, EFlowActorSpawnQueueMode::Staggered));
		Fixture.Scheduler->Tick(0.0f);
		AActor* Actor = Record->GetOwnedActor();
		TestTrue(TEXT("Actor spawned"), IsValid(Actor));
		if (!IsValid(Actor))
		{
			return;
		}

		Actor->Destroy();
		TestNull(TEXT("Externally destroyed actor released"), Record->GetOwnedActor());
		Record->CleanupRuntime();
		TestEqual(TEXT("One cleanup callback"), Fixture.Owner->CleanupCount, 1);
	});

	It("submits a complete standalone Spawn Actors pass before immediate callbacks", [this]
	{
		UFlowSettings* Settings = GetMutableDefault<UFlowSettings>();
		TGuardValue<EFlowActorSpawnQueueMode> ModeGuard(
			Settings->DefaultActorSpawnQueueMode, EFlowActorSpawnQueueMode::AttemptOnEnqueue);
		FFixture Fixture;
		UFlowSpawnActorsTestNode* Node = NewObject<UFlowSpawnActorsTestNode>(Fixture.Owner);
		Node->SetTestOwner(Fixture.Owner);
		Node->ExecuteInput(UFlowNode_SpawnActorsBase::INPIN_InitialSpawns.PinName);

		TestEqual(TEXT("All three records retained"), Node->GetRecordCount(), 3);
		TestEqual(TEXT("All three actors retained"), Node->GetSpawnedCount(), 3);
		TestTrue(TEXT("Per-record outputs precede the pass completion"), Node->Outputs == TArray<FName>{
			UFlowNode_SpawnActorsBase::OUTPIN_EachSpawnSucceeded.PinName,
			UFlowNode_SpawnActorsBase::OUTPIN_EachSpawnSucceeded.PinName,
			UFlowNode_SpawnActorsBase::OUTPIN_EachSpawnSucceeded.PinName,
			UFlowNode_SpawnActorsBase::OUTPIN_AllSpawnsCompleted.PinName});
		TestFalse(TEXT("No pending records"), Fixture.Scheduler->HasOutstandingSpawns(Node));
	});

	It("hides the Spawn Actors test node from authoring", [this]
	{
		TestTrue(TEXT("Test node is not placeable in the Flow palette"),
			UFlowSpawnActorsTestNode::StaticClass()->HasAnyClassFlags(CLASS_NotPlaceable));
		TestTrue(TEXT("Test node is excluded from the Flow catalog"),
			UFlowSpawnActorsTestNode::StaticClass()->HasMetaData(TEXT("ExcludeFromFlowCatalog")));
	});

	It("finishes selection before queueing a staggered pass", [this]
	{
		FFixture Fixture;
		FInstancedStruct Selector;
		Selector.InitializeAs<FFlowActorSpawnTestSelector>().Count = 2;
		UFlowDefaultActorSpawnRecord* Template = NewObject<UFlowDefaultActorSpawnRecord>(Fixture.Owner);
		const FFlowActorSpawningPassConfig Config{*Fixture.Owner, Selector, *Template, Fixture.Owner->Records,
			EFlowActorSpawnQueueMode::Staggered, EFlowActorSpawningMethod::FullSpawn};
		FFinishPassProbe Assistant;
		const FFlowActorSpawningPassResult Result = Assistant.TryExecuteSpawningPass(Config);

		TestFalse(TEXT("Records remain unqueued through selector finish"), Assistant.bQueuedDuringFinish);
		TestEqual(TEXT("Both records queued after selector finish"), Result.NumQueued, 2);
	});

	It("does not reuse a record prepared earlier in the same pass", [this]
	{
		FFixture Fixture;
		UFlowDefaultActorSpawnRecord* First = Fixture.CreateRecord(0);
		UFlowDefaultActorSpawnRecord* Second = Fixture.CreateRecord(1);
		TestTrue(TEXT("First queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*First, EFlowActorSpawnQueueMode::Staggered));
		TestTrue(TEXT("Second queued"), Fixture.Scheduler->TryEnqueueActorSpawnRecord(
			*Second, EFlowActorSpawnQueueMode::Staggered));
		Fixture.Scheduler->Tick(0.0f);
		if (!First->IsSpawnSuccessful() || !Second->IsSpawnSuccessful())
		{
			AddError(TEXT("Existing actors must be alive before the replacement pass"));
			return;
		}

		FInstancedStruct Selector;
		Selector.InitializeAs<FFlowActorSpawnTestSelector>().Count = 2;
		UFlowDefaultActorSpawnRecord* Template = NewObject<UFlowDefaultActorSpawnRecord>(Fixture.Owner);
		const FFlowActorSpawningPassConfig Config{*Fixture.Owner, Selector, *Template, Fixture.Owner->Records,
			EFlowActorSpawnQueueMode::Staggered, EFlowActorSpawningMethod::FullSpawn};
		FFlowActorSpawningAssistant Assistant;
		const FFlowActorSpawningPassResult Result = Assistant.TryExecuteSpawningPass(Config);

		TestEqual(TEXT("Both replacements queued"), Result.NumQueued, 2);
		TestEqual(TEXT("Distinct replacement records registered"), Fixture.Owner->Records.Num(), 4);
		if (Fixture.Owner->Records.Num() == 4)
		{
			TestTrue(TEXT("Distinct replacement record identities"),
				Fixture.Owner->Records[2] != Fixture.Owner->Records[3]);
		}
	});

	It("reuses a missing record without reusing live or prepared records", [this]
	{
		TGuardValue<EFlowActorSpawnQueueMode> ModeGuard(
			GetMutableDefault<UFlowSettings>()->DefaultActorSpawnQueueMode, EFlowActorSpawnQueueMode::Staggered);
		FFixture Fixture;
		UFlowSpawnActorsTestNode* Node = NewObject<UFlowSpawnActorsTestNode>(Fixture.Owner);
		Node->SetTestOwner(Fixture.Owner);
		Node->ExecuteInput(UFlowNode_SpawnActorsBase::INPIN_InitialSpawns.PinName);
		Fixture.Scheduler->Tick(0.0f);
		Fixture.Scheduler->Tick(0.0f);
		TestEqual(TEXT("First pass has three records"), Node->GetRecordCount(), 3);
		if (Node->GetRecordCount() != 3)
		{
			return;
		}

		UFlowActorSpawnRecord* FirstRecord = Node->GetRecord(0);
		FirstRecord->CleanupRuntime();
		Node->ExecuteInput(UFlowNode_SpawnActorsBase::INPIN_InitialSpawns.PinName);
		TestEqual(TEXT("One missing record reused and two live actors retained"), Node->GetRecordCount(), 5);
		TestTrue(TEXT("Missing record retains its identity"), Node->GetRecord(0) == FirstRecord);
		if (Node->GetRecordCount() == 5)
		{
			TestTrue(TEXT("New records are distinct"), Node->GetRecord(3) != Node->GetRecord(4));
		}
	});

	It("reuses missing records in immediate mode without sharing a new record between slots", [this]
	{
		TGuardValue<EFlowActorSpawnQueueMode> ModeGuard(
			GetMutableDefault<UFlowSettings>()->DefaultActorSpawnQueueMode, EFlowActorSpawnQueueMode::AttemptOnEnqueue);
		FFixture Fixture;
		UFlowSpawnActorsTestNode* Node = NewObject<UFlowSpawnActorsTestNode>(Fixture.Owner);
		Node->SetTestOwner(Fixture.Owner);
		Node->ExecuteInput(UFlowNode_SpawnActorsBase::INPIN_InitialSpawns.PinName);
		TestEqual(TEXT("Immediate first pass registers three records"), Node->GetRecordCount(), 3);
		TestEqual(TEXT("Immediate first pass spawns three actors"), Node->GetSpawnedCount(), 3);
		if (Node->GetRecordCount() != 3)
		{
			return;
		}

		UFlowActorSpawnRecord* FirstRecord = Node->GetRecord(0);
		FirstRecord->CleanupRuntime();
		Node->ExecuteInput(UFlowNode_SpawnActorsBase::INPIN_InitialSpawns.PinName);
		TestEqual(TEXT("Second pass reuses one record and adds two"), Node->GetRecordCount(), 5);
		TestTrue(TEXT("Reused record retains its identity"), Node->GetRecord(0) == FirstRecord);
		TestEqual(TEXT("Five actors remain tracked"), Node->GetSpawnedCount(), 5);
		TestFalse(TEXT("Immediate second pass leaves no outstanding spawns"), Fixture.Scheduler->HasOutstandingSpawns(Node));
	});

	It("keeps a standalone Spawn Actors pass staggered when the project default is staggered", [this]
	{
		UFlowSettings* Settings = GetMutableDefault<UFlowSettings>();
		TGuardValue<EFlowActorSpawnQueueMode> ModeGuard(
			Settings->DefaultActorSpawnQueueMode, EFlowActorSpawnQueueMode::Staggered);
		FFixture Fixture;
		UFlowSpawnActorsTestNode* Node = NewObject<UFlowSpawnActorsTestNode>(Fixture.Owner);
		Node->SetTestOwner(Fixture.Owner);
		Node->ExecuteInput(UFlowNode_SpawnActorsBase::INPIN_InitialSpawns.PinName);

		TestEqual(TEXT("Three records queued"), Node->GetRecordCount(), 3);
		TestTrue(TEXT("No outputs during submission"), Node->Outputs.IsEmpty());
		Fixture.Scheduler->Tick(0.0f);
		TestEqual(TEXT("Two successful outputs in first tick"), Node->Outputs.Num(), 2);
		Fixture.Scheduler->Tick(0.0f);
		TestEqual(TEXT("Last success and pass completion"), Node->Outputs.Num(), 4);
	});
}

#endif
