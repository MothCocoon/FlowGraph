// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/Array.h"
#include "Containers/Set.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"
#include "Types/FlowActorSpawnQueueMode.h"
#include "Types/FlowEnumUtils.h"

#include "FlowActorSpawningAssistant.generated.h"

class AActor;
class UObject;
class UFlowActorSpawnRecord;
struct FInstancedStruct;

// Method to take when spawning with spawn actors pass
UENUM(BlueprintType)
enum class EFlowActorSpawningMethod : uint8
{
	// Fully (re)spawn all enemies in the group
	FullSpawn,

	// (Re)spawn dead/missing actors in a group
	RefillMissing,

	// (Re)spawn single dead/missing actor
	RefillSingleMissing,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowActorSpawningMethod);

#if WITH_SERVER_CODE
/** Non-owning inputs for one synchronous spawn pass. The caller supplies the owner, selector, template,
 * records, queue mode, and method; only the initial-spawn hint has a default. */
struct FFlowActorSpawningPassConfig
{
	UObject& Owner;
	FInstancedStruct& Selector;
	UFlowActorSpawnRecord& RecordTemplate;
	TArray<TObjectPtr<UFlowActorSpawnRecord>>& Records;
	EFlowActorSpawnQueueMode QueueMode;
	EFlowActorSpawningMethod Method;
	bool bIsInitialSpawnForSpawnGroup = false;
};

/** Stack-local pass state. The caller supplies Config; collections start empty and NumPrepared starts at zero. */
struct FFlowActorSpawningPassContext
{
	const FFlowActorSpawningPassConfig& Config;
	TArray<UFlowActorSpawnRecord*> RecordsToSubmit;
	TSet<int32> PreparedRecordIndices;
	TSet<UFlowActorSpawnRecord*> NewRecords;
	int32 NumPrepared = 0;
};

/** NumPrepared counts configured records; NumQueued is INDEX_NONE if the pass could not start. */
struct FFlowActorSpawningPassResult
{
	int32 NumQueued = INDEX_NONE;
	int32 NumPrepared = 0;
};
#endif

/** Shared pass scheduling and lifecycle callbacks for Flow actor spawning. Derived structs can add project-specific behavior. */
USTRUCT(BlueprintType, meta = (Hidden))
struct FLOWGAMEFRAMEWORK_API FFlowActorSpawningAssistant
{
	GENERATED_BODY()

public:
	virtual ~FFlowActorSpawningAssistant() = default;

#if WITH_SERVER_CODE
	/** Called when the owning instance initializes, before it schedules spawning passes. */
	virtual void Startup() {}

	/** Called when the owning instance deinitializes; release assistant-owned runtime state. */
	virtual void Shutdown() {}

	/** Called when a record enters pre-spawn configuration, before acquiring an actor. */
	virtual void ProcessPreSpawnConfiguration(UFlowActorSpawnRecord&) {}

	/** Called after acquiring an actor, before finishing its spawn. */
	virtual void ProcessPreFinishSpawnActor(UFlowActorSpawnRecord&) {}

	/** Called after finishing an actor's spawn, before the attempt reports completion. */
	virtual void ProcessPostSpawnConfiguration(UFlowActorSpawnRecord&) {}

	/** Called when a spawn attempt settles as a success or failure. */
	virtual void ProcessFinishedSpawnAttempt(UFlowActorSpawnRecord&) {}

	/** Called before a record clears an acquired actor during cleanup or external destruction. */
	virtual void OnCleanupSpawnedInstance(UFlowActorSpawnRecord&) {}

	/** Start and finish a selector pass, prepare each record, and submit it to the queue.
	 * Returns INDEX_NONE in NumQueued if the pass cannot start. */
	FFlowActorSpawningPassResult TryExecuteSpawningPass(const FFlowActorSpawningPassConfig& Config);

	/** Find the record to reuse or append for a spawn candidate; INDEX_NONE skips a queued or non-refillable record. */
	static int32 ChooseRecordIndex(
		int32 SourceIndex,
		int32 NumToSpawn,
		EFlowActorSpawningMethod Method,
		const TArray<TObjectPtr<UFlowActorSpawnRecord>>& Records);

protected:
	/** Begin selector bookkeeping once the pass owner and scheduler have been validated. */
	virtual void StartPass(FFlowActorSpawningPassContext& Context);

	/** Return the selector's spawn count; a negative value aborts the pass after FinishPass. */
	virtual int32 GetNumToSpawn(FFlowActorSpawningPassContext& Context);

	/** Find an available record index for this candidate, or INDEX_NONE to skip it. */
	virtual int32 SelectRecordIndex(FFlowActorSpawningPassContext& Context, int32 SourceIndex, int32 NumToSpawn);

	/** Choose an actor class for the selected index; a null class skips the candidate. */
	virtual TSubclassOf<AActor> ChooseActorClass(FFlowActorSpawningPassContext& Context, int32 RecordIndex);

	/** Configure or create a record and register new records before queue submission; null skips the candidate. */
	virtual UFlowActorSpawnRecord* PrepareRecord(FFlowActorSpawningPassContext& Context, int32 RecordIndex,
		TSubclassOf<AActor> ActorClass);

	/** Handle a record rejected for incorrect ownership or queue admission. */
	virtual void OnRejectedRecord(FFlowActorSpawningPassContext& Context, UFlowActorSpawnRecord& Record);

	/** Finish selector bookkeeping before the prepared records enter the queue. */
	virtual void FinishPass(FFlowActorSpawningPassContext& Context);
#endif
};
