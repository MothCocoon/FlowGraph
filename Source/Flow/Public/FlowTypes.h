#pragma once

#include "FlowSave.h"
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

UENUM(BlueprintType)
enum class EFlowNodeState : uint8
{
	NeverActivated,
	Active,
	Completed,
	Aborted
};

UENUM(BlueprintType)
enum class EFlowFinishPolicy : uint8
{
	Keep,
	Abort
};

UENUM(BlueprintType)
enum class EFlowNetMode : uint8
{
	Any					UMETA(ToolTip = "Any networking mode"),
	Authority			UMETA(ToolTip = "Executed on the server or in the single-player (standalone)."),
	ClientOnly			UMETA(ToolTip = "Executed locally, on the single client."),
	ServerOnly			UMETA(ToolTip = "Executed on the server"),
	SinglePlayerOnly	UMETA(ToolTip = "Executed only in the single player, not available in multiplayer.")
};

UENUM(BlueprintType)
enum class EFlowOnScreenMessageType : uint8
{
	Temporary,
	Permanent
};
