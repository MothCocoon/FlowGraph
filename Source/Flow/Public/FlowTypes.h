// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "GameplayTagContainer.h"
#include "Types/FlowEnumUtils.h"

#include "FlowTypes.generated.h"

#if WITH_EDITORONLY_DATA
UENUM(BlueprintType)
enum class EFlowNodeStyle : uint8
{
	// Deprecated EFlowNodeStyle enum (use NodeDisplayStyle tag instead)
	Condition,
	Default,
	InOut UMETA(Hidden),
	Latent,
	Logic,
	SubGraph UMETA(Hidden),
	Custom,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowNodeStyle)
#endif

UENUM(BlueprintType)
enum class EFlowNodeState : uint8
{
	NeverActivated,
	Active,
	Completed,
	Aborted,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowNodeState)

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
enum class EFlowSignalMode : uint8
{
	Enabled		UMETA(ToolTip = "Default state, node is fully executed."),
	Disabled	UMETA(ToolTip = "No logic executed, any Input Pin activation is ignored. Node instantly enters a deactivated state."),
	PassThrough UMETA(ToolTip = "Internal node logic not executed. All connected outputs are triggered, node finishes its work.")
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

UENUM(BlueprintType)
enum class EFlowAddOnAcceptResult : uint8
{
	// Note that these enum values are ordered by priority, where greater numerical values are higher priority
	// (see CombineFlowAddOnAcceptResult)

	// No result from the current operation
	Undetermined,

	// Accept, if all other conditions are met
	TentativeAccept,

	// Reject the AddOn outright, regardless if previously TentativelyAccept-ed
	Reject,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = Undetermined UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowAddOnAcceptResult);

FORCEINLINE_DEBUGGABLE EFlowAddOnAcceptResult CombineFlowAddOnAcceptResult(EFlowAddOnAcceptResult Result0, EFlowAddOnAcceptResult Result1)
{
	const FlowEnum::safe_underlying_type<EFlowAddOnAcceptResult>::type Result0AsInt = FlowEnum::ToInt(Result0);
	const FlowEnum::safe_underlying_type<EFlowAddOnAcceptResult>::type Result1AsInt = FlowEnum::ToInt(Result1);

	// Prioritize the higher numerical value enum value
	return static_cast<EFlowAddOnAcceptResult>(FMath::Max(Result0AsInt, Result1AsInt));
}

UENUM()
enum class EFlowForEachAddOnFunctionReturnValue : int8
{
	// Continue iterating the ForEach loop
	Continue,

	// Break out of the ForEach loop, with a "Success" result (whatever that means to the TFunction)
	BreakWithSuccess,

	// Break out of the ForEach loop, with a "Failure" return (whatever that means to the TFunction)
	BreakWithFailure,

	Max UMETA(Hidden),
	Invalid = -1 UMETA(Hidden),
	Min = 0 UMETA(Hidden),

	ContinueForEachFirst = Continue UMETA(Hidden),
	ContinueForEachLast = Continue UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowForEachAddOnFunctionReturnValue);

namespace EFlowForEachAddOnFunctionReturnValue_Classifiers
{
	FORCEINLINE bool ShouldContinueForEach(EFlowForEachAddOnFunctionReturnValue Result) { return FLOW_IS_ENUM_IN_SUBRANGE(Result, EFlowForEachAddOnFunctionReturnValue::ContinueForEach); }
}
