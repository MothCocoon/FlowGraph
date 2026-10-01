// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowFightActorsTypes.h"
#include "UObject/Interface.h"

#include "FlowFightActorsReinforcementHost.generated.h"

/** Exposes fight operations to a reinforcement addon without naming a concrete fight node. */
UINTERFACE(MinimalAPI)
class UFlowFightActorsReinforcementHost : public UInterface
{
	GENERATED_BODY()
};

class FLOWGAMEFRAMEWORK_API IFlowFightActorsReinforcementHost
{
	GENERATED_BODY()

public:
#if WITH_SERVER_CODE
	/** Request one pass; return accepted records, zero for no work, or INDEX_NONE on error. */
	virtual int32 ExecuteReinforcementPass(EFlowFightActorSpawnMethod Method, bool bShuffleCohorts) = 0;

	/** Return alive-plus-pending population divided by the initially configured population. */
	virtual float GetOverallAliveUnitPercent() const = 0;

	/** Check completion after scheduling, cancellation, defeat, or exhaustion changes. */
	virtual void ReevaluateFightCompletion() = 0;
#endif
};
