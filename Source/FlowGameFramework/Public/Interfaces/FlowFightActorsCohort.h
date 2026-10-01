// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowFightActorsTypes.h"
#include "UObject/Interface.h"

#include "FlowFightActorsCohort.generated.h"

class AActor;
class UFlowNodeAddOn;
struct FFlowActorSpawningAssistant;

/** Identifies a record-owning cohort that can participate in a Fight Actors pass. */
UINTERFACE(MinimalAPI)
class UFlowFightActorsCohort : public UInterface
{
	GENERATED_BODY()
};

class FLOWGAMEFRAMEWORK_API IFlowFightActorsCohort
{
	GENERATED_BODY()

public:
#if WITH_SERVER_CODE
	/** Return accepted record count, zero for valid no-work, or INDEX_NONE for invalid configuration. */
	virtual int32 ExecuteSpawningPass(EFlowFightActorSpawnMethod Method,
		FFlowActorSpawningAssistant& Assistant, bool bIsInitialPass) = 0;

	/** Add this cohort's alive, pending, initial, and total record counts. */
	virtual void AppendActorCounts(FFlowFightActorCounts& InOutCounts) const = 0;

	/** Defeat members using this cohort's actor-lifetime policy. */
	virtual void DefeatSpawnedActors() = 0;

	/** Cancel outstanding records and release runtime ownership. */
	virtual void CleanupActorRecords() = 0;
#endif
};
