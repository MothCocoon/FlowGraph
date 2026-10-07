// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Math/Transform.h"
#include "Templates/SubclassOf.h"
#include "Types/FlowEnumUtils.h"
#include "UObject/Object.h"

#include "FlowActorSpawnRecord.generated.h"

class AActor;
class IFlowActorSpawnRecordOwner;
class UWorld;

UENUM()
enum class EFlowActorSpawnRecordState : uint8
{
	// Configured for a pass but not yet submitted to a scheduler.
	Configured,

	// Enqueued and waiting for the scheduler to start this attempt.
	QueuedForSpawn,

	// Notify the owner before choosing a location or acquiring an actor.
	PreSpawnConfiguration,

	// Resolve a location synchronously or wait for an asynchronous request.
	WaitingForSpawnLocation,

	// Acquire a deferred actor, or reuse an actor in a derived record.
	TrySpawnActor,

	// Notify the owner while the acquired actor is available but not yet finished.
	FinishSpawnActor,

	// Notify the owner after the actor has finished spawning.
	PostSpawnConfigureActor,

	// Terminal success for this spawn attempt.
	SuccessfulSpawn,

	// Terminal failure for this spawn attempt.
	FailedSpawn,

	// Release the previous instance before reconfiguring this record for reuse.
	CleanupForRespawn,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),

	// Configured attempts may be enqueued, but must not execute on enqueue.
	QueueableForSpawnFirst = Configured UMETA(Hidden),
	QueueableForSpawnLast = Configured UMETA(Hidden),

	// Pre-spawn setup through acquisition may hold an asynchronous location helper.
	LocationPreparationFirst = PreSpawnConfiguration UMETA(Hidden),
	LocationPreparationLast = TrySpawnActor UMETA(Hidden),

	// Pending work has not started its spawn attempt.
	QueuedForSpawningFirst = QueuedForSpawn UMETA(Hidden),
	QueuedForSpawningLast = QueuedForSpawn UMETA(Hidden),

	// Started attempts include location waits and pre/post-finish callbacks.
	ActiveSpawningFirst = PreSpawnConfiguration UMETA(Hidden),
	ActiveSpawningLast = PostSpawnConfigureActor UMETA(Hidden),

	// Successful attempts have completed acquisition, finish, and post-spawn configuration.
	SuccessfulSpawnFirst = SuccessfulSpawn UMETA(Hidden),
	SuccessfulSpawnLast = SuccessfulSpawn UMETA(Hidden),

	// Failed attempts have no successful spawn result.
	FailedSpawnFirst = FailedSpawn UMETA(Hidden),
	FailedSpawnLast = FailedSpawn UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowActorSpawnRecordState);
FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);

namespace EFlowActorSpawnRecordState_Classifiers
{
	FORCEINLINE bool IsQueueableForSpawnState(EFlowActorSpawnRecordState State) { return FLOW_IS_ENUM_IN_SUBRANGE(State, EFlowActorSpawnRecordState::QueueableForSpawn); }
	FORCEINLINE bool IsLocationPreparationState(EFlowActorSpawnRecordState State) { return FLOW_IS_ENUM_IN_SUBRANGE(State, EFlowActorSpawnRecordState::LocationPreparation); }
	FORCEINLINE bool IsQueuedForSpawningState(EFlowActorSpawnRecordState State) { return FLOW_IS_ENUM_IN_SUBRANGE(State, EFlowActorSpawnRecordState::QueuedForSpawning); }
	FORCEINLINE bool IsActiveSpawningState(EFlowActorSpawnRecordState State) { return FLOW_IS_ENUM_IN_SUBRANGE(State, EFlowActorSpawnRecordState::ActiveSpawning); }
	FORCEINLINE bool IsSuccessfulSpawnState(EFlowActorSpawnRecordState State) { return FLOW_IS_ENUM_IN_SUBRANGE(State, EFlowActorSpawnRecordState::SuccessfulSpawn); }
	FORCEINLINE bool IsFailedSpawnState(EFlowActorSpawnRecordState State) { return FLOW_IS_ENUM_IN_SUBRANGE(State, EFlowActorSpawnRecordState::FailedSpawn); }

	FORCEINLINE bool IsRelevantStateForGetStatusString(EFlowActorSpawnRecordState State)
		{ return IsQueuedForSpawningState(State) || IsActiveSpawningState(State) || IsFailedSpawnState(State); }
}

/**
 * A world-bound spawn attempt with overridable location and actor-acquisition steps.
 * Subclasses can carry authored location settings or replace actor acquisition and release
 * while sharing the configure, queue, callback, and terminal-state lifecycle.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class FLOWGAMEFRAMEWORK_API UFlowActorSpawnRecord : public UObject
{
	GENERATED_BODY()

protected:
	/** Actor class selected for the current attempt. */
	UPROPERTY(Transient)
	TSubclassOf<AActor> ConfiguredActorClass;

	/** Transform resolved for the current attempt before actor acquisition. */
	UPROPERTY(Transient)
	FTransform ResolvedSpawnTransform;

	/** Index assigned by the owner for this record in the current pass. */
	UPROPERTY(Transient)
	int32 RecordIndex = INDEX_NONE;

	/** Current lifecycle state; no runtime state is serialized into a template. */
	UPROPERTY(Transient)
	EFlowActorSpawnRecordState RecordState = EFlowActorSpawnRecordState::Invalid;

	/** Distinguishes a queued attempt from a later reuse of the same record. */
	uint32 ConfigurationGeneration = 0;

public:
	// -- UObject
	virtual UWorld* GetWorld() const override;
	uint32 GetConfigurationGeneration() const { return ConfigurationGeneration; }

#if WITH_SERVER_CODE
	/** Configure a new attempt; pass bIsRespawn to release an existing instance first. */
	bool Configure(TSubclassOf<AActor> InActorClass, int32 InRecordIndex, bool bIsRespawn = false);

	/** Cancel outstanding work and release any instance still owned by this record. */
	virtual void CleanupRuntime();

	/** Transition from configured to queued without executing the attempt. */
	virtual bool TryQueueRecordForSpawning();

	/** Begin and settle any immediately available work for a queued attempt. */
	virtual bool TryStartSpawningProcess();

	virtual bool IsSpawnSuccessful() const { return RecordState == EFlowActorSpawnRecordState::SuccessfulSpawn; }
	virtual bool IsSpawnFailed() const { return RecordState == EFlowActorSpawnRecordState::FailedSpawn; }
	virtual bool IsQueuedToSpawn() const { return RecordState == EFlowActorSpawnRecordState::QueuedForSpawn; }
	virtual bool IsActivelyAttemptingToSpawn() const;
	bool IsQueuedOrActivelyAttemptingtoSpawn() const { return IsQueuedToSpawn() || IsActivelyAttemptingToSpawn(); }

	/** The actor held by the concrete record, if one has been acquired. */
	virtual AActor* GetOwnedActor() const { return nullptr; }
	virtual bool IsSpawnedActorAlive() const;
	TSubclassOf<AActor> GetConfiguredActorClass() const { return ConfiguredActorClass; }
	int32 GetRecordIndex() const { return RecordIndex; }

protected:
	/** Advance one state, or return false while asynchronous work is outstanding. */
	virtual bool AdvanceCurrentState(EFlowActorSpawnRecordState& OutNextState);
	virtual void OnStateChanged(EFlowActorSpawnRecordState PreviousState, EFlowActorSpawnRecordState NextState) { }
	virtual FTransform GetDesiredSpawnTransform() const;
	virtual bool AcquireActor() { return false; }
	virtual bool FinishAcquiredActor() { return false; }
	virtual void ReleaseOwnedActor(bool bDestroyActor) { }

	/** Resume a waiting location request, including its terminal failure path. */
	void CompleteSpawnLocation(bool bSucceeded);
	void SetRecordState(EFlowActorSpawnRecordState NextState);
	void UpdateRecordState();
	EFlowActorSpawnRecordState GetRecordState() const { return RecordState; }
	IFlowActorSpawnRecordOwner* GetRecordOwner() const;
#endif
};
