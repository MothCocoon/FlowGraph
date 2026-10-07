// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "ActorSpawning/FlowFightActorsTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowFightActorsTypes)

float FFlowFightActorCounts::CalculateAliveAndPendingVsInitialCountUnitPercent() const
{
	check(InitialSpawnCount >= 0);
	if (InitialSpawnCount == 0)
	{
		return 1.0f;
	}

	check(AliveCount >= 0);
	check(PendingSpawnCount >= 0);
	return static_cast<float>(GetAliveAndPendingCount()) / static_cast<float>(InitialSpawnCount);
}
