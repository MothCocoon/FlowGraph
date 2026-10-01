// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Math/Transform.h"
#include "Types/FlowEnumUtils.h"

#include "FlowTeleportTypes.generated.h"

class AActor;
struct FRandomStream;

/** Describes how resolved destinations are assigned to resolved actors. */
UENUM(BlueprintType)
enum class EFlowTeleportDestinationAssignment : uint8
{
	/** Assign destinations in the same order as the resolved actors. */
	InOrder,

	/** Select a unique random destination for each actor. */
	RandomWithoutReplacement,

	/** Assign the single resolved destination to every actor. */
	SingleDestinationForAll,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowTeleportDestinationAssignment);

/** Result of an asynchronous teleport operation. */
UENUM(BlueprintType)
enum class EFlowTeleportOperationResult : uint8
{
	/** The teleport operation completed successfully. */
	Succeeded,

	/** The teleport operation completed unsuccessfully. */
	Failed,

	/** The teleport operation was cancelled before completion. */
	Cancelled,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowTeleportOperationResult);

UENUM(BlueprintType)
enum class EFlowTeleportExecutionResult : uint8
{
	/** The addon or default teleport implementation should be used. */
	UseDefault,

	/** The addon completed the teleport successfully. */
	Succeeded,

	/** The addon completed the teleport unsuccessfully. */
	Failed,

	/** The addon started an asynchronous teleport operation. */
	Started,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowTeleportExecutionResult);

UENUM(BlueprintType)
enum class EFlowTeleportFailureReason : uint8
{
	/** No failure occurred. */
	None,

	/** The actor rejected the requested teleport. */
	TeleportRejected,

	/** An execution addon reported a failure. */
	ExecutionAddonFailed,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowTeleportFailureReason);

/** Helpers for assigning teleport destinations. */
namespace FlowTeleport
{
	/** Assigns candidate destinations to actors according to the requested policy. */
	FLOWGAMEFRAMEWORK_API bool AssignDestinations(
		EFlowTeleportDestinationAssignment Assignment,
		int32 ActorCount,
		const TArray<FTransform>& Candidates,
		FRandomStream& RandomStream,
		TArray<int32>& OutCandidateIndices,
		FText& OutFailureReason);
}
