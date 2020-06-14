#pragma once

#include "FlowTypes.generated.h"

#if WITH_EDITORONLY_DATA
UENUM()
enum class EFlowNodeStyle : uint8
{
	Condition,
	Default,
	InOut,
	Latent,
	Logic,
	SubGraph
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
