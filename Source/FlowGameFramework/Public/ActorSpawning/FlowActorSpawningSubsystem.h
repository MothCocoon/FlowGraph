// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/ArrayView.h"
#include "Subsystems/WorldSubsystem.h"
#include "Types/FlowActorSpawnQueueMode.h"

#include "FlowActorSpawningSubsystem.generated.h"

class UFlowActorSpawnRecord;

/** One GC-visible queued attempt with its configuration generation at submission time. */
USTRUCT()
struct FLOWGAMEFRAMEWORK_API FFlowActorSpawnQueueEntry
{
	GENERATED_BODY()

public:
	/** Record retained for GC while this attempt is pending, flushing, or parked. */
	UPROPERTY(Transient)
	TObjectPtr<UFlowActorSpawnRecord> Record;

	/** Configuration at enqueue time; stale work must not run a reused record. */
	UPROPERTY(Transient)
	uint32 ConfigurationGeneration = 0;

	FFlowActorSpawnQueueEntry() = default;
	FFlowActorSpawnQueueEntry(UFlowActorSpawnRecord& InRecord);
};

// NOTE (gtaylor) This is the actor spawning subsystem used by the Spawn Actors and Fight Actors nodes
// 
// It is setup to be a fire-and-forget async actor spawning system that processes a queue of spawn requests
// over multiple game frames.  It is configured by default to spawn sequentially, so that each EQS query 
// can take into account the location of the previously spawned actor(s) in their group.  But it can be 
// configured to spawn immediately if so desired. A single spawn request may also override the spawn policy 
// separately from the system setting.
// 
// If you don't need immediate spawning, spawning over multiple frames is gentler on the UE systems, 
// amortizing the spawn-actor costs over multiple frames and preventing a big frame-spike at the start of 
// an encounter.  So it's generally good to preserve if you can wait for them to spawn over time.

/** Queues actor spawn attempts for authoritative worlds and staggers synchronous successes across ticks. */
UCLASS()
class FLOWGAMEFRAMEWORK_API UFlowActorSpawningSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

protected:
	/** GC-visible records awaiting the historical staggered scheduler. */
	UPROPERTY(Transient)
	TArray<FFlowActorSpawnQueueEntry> PendingSpawnRecords;

	/** GC-visible ready attempts processed on enqueue or the next tick if the flush reaches its bound. */
	UPROPERTY(Transient)
	TArray<FFlowActorSpawnQueueEntry> ImmediateSpawnRecords;

	/** GC-visible asynchronous immediate attempts, released after their callback resolves or cancels them. */
	UPROPERTY(Transient)
	TArray<FFlowActorSpawnQueueEntry> ParkedImmediateSpawnRecords;

	/** Single staggered attempt, retained through its async wait. */
	UPROPERTY(Transient)
	TObjectPtr<UFlowActorSpawnRecord> ActiveSpawnRecord;

	/** GC-visible record currently executing and able to call back into cancellation. */
	UPROPERTY(Transient)
	TObjectPtr<UFlowActorSpawnRecord> ProcessingImmediateRecord;

	/** Quick staggered successes permitted per tick; fast failures do not consume the budget. */
	UPROPERTY(EditAnywhere, Category = "Actor Spawning", meta = (ClampMin = "1"))
	int32 MaxSpawnSuccessesPerTick = 2;

	/** Maximum immediate attempts per flush before remaining work continues on the next tick. */
	UPROPERTY(EditAnywhere, Category = "Actor Spawning", meta = (ClampMin = "1"))
	int32 MaxSpawnAttemptsPerFlush = 64;

	/** Owner currently being cancelled; used to reject reentrant enqueue for that owner. */
	const UObject* CancellingOwner = nullptr;

	/** Set during Deinitialize so cancellation callbacks cannot enqueue work or start another tick. */
	bool bIsDeinitializing = false;

	/** Prevent enqueue callbacks from processing a pass before all records have been submitted. */
	bool bIsSubmittingPass = false;

	/** Prevent recursive immediate processing while allowing nested enqueues to join the worklist. */
	bool bIsProcessingImmediate = false;

public:
	// UWorldSubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// --

	// FTickableObjectBase
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	// --

#if WITH_SERVER_CODE
	/** Submit one owner-retained record using FlowSettings::DefaultActorSpawnQueueMode. */
	bool TryEnqueueActorSpawnRecord(UFlowActorSpawnRecord& Record);

	/** Override the FlowSettings mode for this record only; AttemptOnEnqueue may invoke callbacks before returning. */
	bool TryEnqueueActorSpawnRecord(UFlowActorSpawnRecord& Record, EFlowActorSpawnQueueMode ModeOverride);

	/** Submit a complete owner-retained pass using FlowSettings::DefaultActorSpawnQueueMode; return records accepted. */
	int32 SubmitSpawnPass(const UObject& Owner, TConstArrayView<UFlowActorSpawnRecord*> Records);

	/** Override the FlowSettings mode for this pass only; queue all accepted records before invoking callbacks. */
	int32 SubmitSpawnPass(
		const UObject& Owner,
		TConstArrayView<UFlowActorSpawnRecord*> Records,
		EFlowActorSpawnQueueMode ModeOverride);

	/** Whether this owner's pending, parked, or asynchronous attempts still need a result. */
	bool HasOutstandingSpawns(const UObject* Owner) const;

	/** Cancel an owner's queued and active attempts before its records or world context are torn down. */
	void CancelSpawnsForOwner(const UObject* Owner);

	/** Set the number of immediately successful staggered attempts allowed per tick, clamped to at least one. */
	void SetMaxSpawnSuccessesPerTick(int32 InMaxSuccesses);

	/** Set the maximum number of records attempted in one immediate flush, clamped to at least one. */
	void SetMaxSpawnAttemptsPerFlush(int32 InMaxAttempts);

protected:
	/** Validate and enqueue without starting work; used while staging a complete pass. */
	bool QueueRecord(UFlowActorSpawnRecord& Record, EFlowActorSpawnQueueMode Mode);

	/** Attempt ready immediate work up to the configured bound; defer overflow to a later tick. */
	void FlushImmediateSpawnRecords();

	/** Cancel outstanding attempts without touching successfully completed actors. */
	void CancelOutstandingSpawnRecords();

	/** Remove one owner's queue entries before invoking their cleanup callbacks. */
	void ExtractRecordsForOwner(
		TArray<FFlowActorSpawnQueueEntry>& Records,
		const UObject* Owner,
		TArray<FFlowActorSpawnQueueEntry>& OutRecords);

	/** Drop completed or cancelled immediate work while retaining active async attempts for GC. */
	void PruneImmediateSpawnRecords();

	/** Drain staggered FIFO work until the success budget is spent or an asynchronous attempt starts. */
	void ProcessPendingSpawnRecords();
#endif

};
