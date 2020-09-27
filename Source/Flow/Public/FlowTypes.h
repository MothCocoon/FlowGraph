#pragma once

#include "FlowTypes.generated.h"

#if WITH_EDITORONLY_DATA
UENUM(BlueprintType)
enum class EFlowNodeStyle : uint8
{
	Condition,
	Default,
	InOut UMETA(Hidden),
	Latent,
	Logic,
	SubGraph UMETA(Hidden)
};
#endif

#if !UE_BUILD_SHIPPING
enum class EFlowActivationState : uint8
{
	NeverActivated,
	Active,
	WasActive
};
#endif
