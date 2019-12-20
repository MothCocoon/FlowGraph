#pragma once

#include "FlowAssetTypes.generated.h"

UENUM()
enum class EFlowNodeStyle : uint8
{
	Condition,
	Default,
	InOut,
	Latent,
	Logic,
	SubFlow
};
