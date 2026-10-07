// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowActorSpawningAssistant.h"
#include "Nodes/FlowNode.h"
#include "StructUtils/InstancedStruct.h"

#include "FlowNode_SpawnActorsBase.generated.h"

class AActor;
class UFlowActorSpawnRecord;

/** Shared spawn inputs, assistant callbacks, spawned-actor tracking, and completion outputs for Flow nodes. */
UCLASS(Abstract, Blueprintable, DisplayName = "Spawn Actors Base")
class FLOWGAMEFRAMEWORK_API UFlowNode_SpawnActorsBase : public UFlowNode
{
	GENERATED_BODY()

protected:

	/** The node chooses the assistant type; authors may edit its settings but cannot replace the struct. */
	UPROPERTY(EditAnywhere, DisplayName = "Actor Spawning", NoClear, Category = Configuration, meta = (DisplayPriority = 10, StructTypeConst))
	TInstancedStruct<FFlowActorSpawningAssistant> SpawningAssistant;

	/** Actors still tracked by this node, exposed through the SpawnedActors data output. */
	UPROPERTY(Transient, meta = (SourceForOutputFlowPin, FlowPinType = "Object"))
	TArray<AActor*> SpawnedActors;

public:
	UFlowNode_SpawnActorsBase();

	virtual void InitializeInstance() override;
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
	virtual void DeinitializeInstance() override;

	static const FFlowPin INPIN_InitialSpawns;
	static const FFlowPin INPIN_DefeatSpawnedActors;
	static const FFlowPin OUTPIN_EachSpawnSucceeded;
	static const FFlowPin OUTPIN_EachSpawnFailed;
	static const FFlowPin OUTPIN_AllSpawnsCompleted;

protected:
	virtual void OnSpawningAssistantShutdown() {}

#if WITH_SERVER_CODE
	virtual void StartSpawningPass() PURE_VIRTUAL(UFlowNode_SpawnActorsBase::StartSpawningPass);
	virtual void DefeatAllSpawnedActors() PURE_VIRTUAL(UFlowNode_SpawnActorsBase::DefeatAllSpawnedActors);
	virtual void CleanupActorRecords() PURE_VIRTUAL(UFlowNode_SpawnActorsBase::CleanupActorRecords);
	virtual bool HasAnyOutstandingSpawns() const PURE_VIRTUAL(UFlowNode_SpawnActorsBase::HasAnyOutstandingSpawns, return false;);

	/** Worker functions to service the IFlowActorSpawnRecordOwner interface */
	void HandlePreSpawnConfiguration(UFlowActorSpawnRecord& Record);
	void HandlePreFinishSpawnActor(UFlowActorSpawnRecord& Record);
	void HandlePostSpawnConfiguration(UFlowActorSpawnRecord& Record);
	void HandleFinishedSpawnAttempt(UFlowActorSpawnRecord& Record);
	void HandleCleanupSpawnedInstance(UFlowActorSpawnRecord& Record);
#endif
};
