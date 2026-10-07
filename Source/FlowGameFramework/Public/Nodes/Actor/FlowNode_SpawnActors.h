// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowActorSpawnRecordOwner.h"
#include "Nodes/Actor/FlowNode_SpawnActorsBase.h"
#include "StructUtils/InstancedStruct.h"

#include "FlowNode_SpawnActors.generated.h"

// NOTE (gtaylor) The SpawnActors Node (SAN) implementation for FlowGraph plugin is only
// marginally usable on its own. If you combine it with the AIFlowGraph UAIFlowActorSpawnRecordEQS
// it becomes more interesting.  And for many projects you might want to also make your own
// subclass of it (or of SANBase) to hook into your project's actor lifetime systems (pooling, etc.)
//
// There is a shared spawning architecture and spawning system with Fight Actors Node (FAN)
// through SANBase. SAN differs from FAN in that it assumes only one 'cohort' of actors is being spawned,
// so it embeds the cohort data directly in the SAN node.  You could extend this to also accept cohorts or
// make your own SAN altogether that organizes the actor data to spawn differently.
//
// The Spawning architecture goes through the actor spawning subsystem. This allows for fire-and-forget
// async actor spawning that progresses over multiple game frames.  It is configured by default to spawn
// sequentially, so that each EQS query can take into account the location of the previously spawned actor(s)
// in their group.  But it can be configured to spawn immediately if so desired.

class UFlowActorSpawnRecord;

/** Spawns actors selected by a Flow selector using an authored spawn-record template. */
UCLASS(Blueprintable, DisplayName = "Spawn Actors")
class FLOWGAMEFRAMEWORK_API UFlowNode_SpawnActors : public UFlowNode_SpawnActorsBase, public IFlowActorSpawnRecordOwner
{
	GENERATED_BODY()

protected:

	/** Selects actor classes and the number of actors for each spawn pass. */
	UPROPERTY(EditAnywhere, Category = Configuration, DisplayName = "What to Spawn", NoClear, meta = (ExcludeBaseStruct, BaseStruct = "/Script/FlowGameFramework.FlowActorSpawnSelector", DisplayPriority = 2))
	FInstancedStruct ActorSpawnSelector;

	/** Authored record duplicated for each new actor; concrete record types choose the spawn location and acquisition. */
	UPROPERTY(EditAnywhere, Instanced, NoClear, Category = Configuration, DisplayName = "Where to Spawn", meta = (DisplayPriority = 2))
	TObjectPtr<UFlowActorSpawnRecord> SpawnRecordTemplate;

	/** Runtime records retained for outstanding attempts and cleanup. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UFlowActorSpawnRecord>> ActorRecords;

public:
	UFlowNode_SpawnActors();
	virtual void PostInitProperties() override;

#if WITH_SERVER_CODE
	virtual AActor* TryGetActorOwner() const override;
	virtual void ProcessPreSpawnConfiguration(UFlowActorSpawnRecord& Record) override;
	virtual void ProcessPreFinishSpawnActor(UFlowActorSpawnRecord& Record) override;
	virtual void ProcessPostSpawnConfiguration(UFlowActorSpawnRecord& Record) override;
	virtual void ProcessFinishedSpawnAttempt(UFlowActorSpawnRecord& Record) override;
	virtual void OnCleanupSpawnedInstance(UFlowActorSpawnRecord& Record) override;
#endif

protected:
#if WITH_SERVER_CODE
	virtual void StartSpawningPass() override;
	virtual void DefeatAllSpawnedActors() override;
	virtual void CleanupActorRecords() override;
	virtual bool HasAnyOutstandingSpawns() const override;
#endif

	virtual void UpdateNodeConfigText_Implementation() override;
#if WITH_EDITOR
	virtual EDataValidationResult ValidateNode() override;
#endif
};
