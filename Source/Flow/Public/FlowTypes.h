// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "GameplayTagContainer.h"

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

// Finish Policy value is read by Flow Node
// Nodes have opportunity to terminate themselves differently if Flow Graph has been aborted
// Example: Spawn node might despawn all actors if Flow Graph is aborted, not completed
UENUM(BlueprintType)
enum class EFlowFinishPolicy : uint8
{
	Keep,
	Abort
};

UENUM(BlueprintType)
enum class EFlowNetMode : uint8
{
	Any					UMETA(ToolTip = "Any networking mode."),
	Authority			UMETA(ToolTip = "Executed on the server or in the single-player (standalone)."),
	ClientOnly			UMETA(ToolTip = "Executed locally, on the single client."),
	ServerOnly			UMETA(ToolTip = "Executed on the server."),
	SinglePlayerOnly	UMETA(ToolTip = "Executed only in the single player, not available in multiplayer.")
};

UENUM(BlueprintType)
enum class EFlowTagContainerMatchType : uint8
{
	HasAny				UMETA(ToolTip = "Check if container A contains ANY of the tags in the specified container B."),
	HasAnyExact			UMETA(ToolTip = "Check if container A contains ANY of the tags in the specified container B, only allowing exact matches."),
	HasAll				UMETA(ToolTip = "Check if container A contains ALL of the tags in the specified container B."),
	HasAllExact			UMETA(ToolTip = "Check if container A contains ALL of the tags in the specified container B, only allowing exact matches")
};

namespace FlowTypes
{
	FORCEINLINE_DEBUGGABLE bool HasMatchingTags(const FGameplayTagContainer& Container, const FGameplayTagContainer& OtherContainer, const EFlowTagContainerMatchType MatchType)
	{
		switch (MatchType)
		{
			case EFlowTagContainerMatchType::HasAny:
				return Container.HasAny(OtherContainer);
			case EFlowTagContainerMatchType::HasAnyExact:
				return Container.HasAnyExact(OtherContainer);
			case EFlowTagContainerMatchType::HasAll:
				return Container.HasAll(OtherContainer);
			case EFlowTagContainerMatchType::HasAllExact:
				return Container.HasAllExact(OtherContainer);
			default:
				return false;
		}
	}
}

UENUM(BlueprintType)
enum class EFlowOnScreenMessageType : uint8
{
	Temporary,
	Permanent
};
