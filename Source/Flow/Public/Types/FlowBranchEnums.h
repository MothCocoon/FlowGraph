// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowEnumUtils.h"

UENUM(BlueprintType)
enum class EFlowPredicateCombinationRule : uint8
{
	AND UMETA(ToolTip = "Passes if ALL child predicates pass"),
	OR UMETA(ToolTip = "Passes if ANY (at least one) child predicates pass"),

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowPredicateCombinationRule);
