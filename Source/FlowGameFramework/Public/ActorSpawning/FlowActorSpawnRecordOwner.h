// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Interface.h"

#include "FlowActorSpawnRecordOwner.generated.h"

class AActor;
class UFlowActorSpawnRecord;

/** Reflected marker for the immediate owner of a Flow actor spawn record. */
UINTERFACE(MinimalAPI)
class UFlowActorSpawnRecordOwner : public UInterface
{
	GENERATED_BODY()
};

/** Receives ordered lifecycle callbacks from records whose outer implements this interface. */
class FLOWGAMEFRAMEWORK_API IFlowActorSpawnRecordOwner
{
	GENERATED_BODY()

public:
#if WITH_SERVER_CODE
	/** Return an actor for the default spawn transform, or null to use the identity transform. */
	virtual AActor* TryGetActorOwner() const = 0;

	/** Called before location selection and actor acquisition. */
	virtual void ProcessPreSpawnConfiguration(UFlowActorSpawnRecord& Record) = 0;

	/** Called after acquisition while a deferred actor can still be initialized before BeginPlay. */
	virtual void ProcessPreFinishSpawnActor(UFlowActorSpawnRecord& Record) = 0;

	/** Called after the actor has finished spawning but before the terminal result. */
	virtual void ProcessPostSpawnConfiguration(UFlowActorSpawnRecord& Record) = 0;

	/** Called once when an active attempt reaches success or failure, not when cancelled. */
	virtual void ProcessFinishedSpawnAttempt(UFlowActorSpawnRecord& Record) = 0;

	/** Called before the record clears an acquired actor during cleanup or external destruction. */
	virtual void OnCleanupSpawnedInstance(UFlowActorSpawnRecord& Record) = 0;
#endif
};
