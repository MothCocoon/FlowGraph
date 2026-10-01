// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Types/FlowEnumUtils.h"
#include "FlowActorSpawnQueueMode.generated.h"

UENUM()
enum class EFlowActorSpawnQueueMode : uint8
{
	// Wait for the normal world-subsystem tick, subject to its quick-success budget.
	Staggered,

	// Start the attempt before enqueue returns; asynchronous work may complete later.
	AttemptOnEnqueue,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowActorSpawnQueueMode);
