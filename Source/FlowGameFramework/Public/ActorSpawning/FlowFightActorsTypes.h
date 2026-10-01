// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowActorSpawningAssistant.h"
#include "Math/Interval.h"
#include "Types/FlowEnumUtils.h"

#include "FlowFightActorsTypes.generated.h"

/** A pass across all cohorts, or the first cohort/slot that can perform work. */
UENUM(BlueprintType)
enum class EFlowFightActorSpawnMethod : uint8
{
	/** Spawn each cohort's full configured count, including actors already alive. */
	FullSpawnAllCohorts,

	/** Fully spawn only the first cohort that can queue at least one actor. */
	FullSpawnSingleCohort,

	/** Replace missing actors in every cohort. */
	RefillSpawnAllCohorts,

	/** Refill only the first cohort with a missing actor. */
	RefillSpawnSingleCohort,

	/** Replace one missing actor in the first eligible cohort. */
	RefillSpawnSingleActor,

	/** Skip spawning without treating the pass as an error. */
	NoSpawning,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),

	AnySpawnFirst = FullSpawnAllCohorts UMETA(Hidden),
	AnySpawnLast = RefillSpawnSingleActor UMETA(Hidden),

	FullSpawnFirst = FullSpawnAllCohorts UMETA(Hidden),
	FullSpawnLast = FullSpawnSingleCohort UMETA(Hidden),

	RefillSpawnFirst = RefillSpawnAllCohorts UMETA(Hidden),
	RefillSpawnLast = RefillSpawnSingleActor UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowFightActorSpawnMethod);

namespace EFlowFightActorSpawnMethod_Classifiers
{
	FORCEINLINE bool IsAnySpawnMethod(EFlowFightActorSpawnMethod Method)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowFightActorSpawnMethod, 6);
		return FLOW_IS_ENUM_IN_SUBRANGE(Method, EFlowFightActorSpawnMethod::AnySpawn);
	}

	FORCEINLINE bool IsFullSpawnMethod(EFlowFightActorSpawnMethod Method)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowFightActorSpawnMethod, 6);
		return FLOW_IS_ENUM_IN_SUBRANGE(Method, EFlowFightActorSpawnMethod::FullSpawn);
	}

	FORCEINLINE bool IsRefillSpawnMethod(EFlowFightActorSpawnMethod Method)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowFightActorSpawnMethod, 6);
		return FLOW_IS_ENUM_IN_SUBRANGE(Method, EFlowFightActorSpawnMethod::RefillSpawn);
	}

	FORCEINLINE bool IsAllCohortsMethod(EFlowFightActorSpawnMethod Method)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowFightActorSpawnMethod, 6);
		return Method == EFlowFightActorSpawnMethod::FullSpawnAllCohorts
			|| Method == EFlowFightActorSpawnMethod::RefillSpawnAllCohorts;
	}

	FORCEINLINE bool IsSingleCohortMethod(EFlowFightActorSpawnMethod Method)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowFightActorSpawnMethod, 6);
		return Method == EFlowFightActorSpawnMethod::FullSpawnSingleCohort
			|| Method == EFlowFightActorSpawnMethod::RefillSpawnSingleCohort;
	}

	FORCEINLINE bool IsSingleActorMethod(EFlowFightActorSpawnMethod Method)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowFightActorSpawnMethod, 6);
		return Method == EFlowFightActorSpawnMethod::RefillSpawnSingleActor;
	}

	FORCEINLINE bool IsNoSpawnMethod(EFlowFightActorSpawnMethod Method)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowFightActorSpawnMethod, 6);
		return Method == EFlowFightActorSpawnMethod::NoSpawning;
	}

	/** Maps fight-wide selection to the existing per-cohort Flow spawning method. */
	FORCEINLINE EFlowActorSpawningMethod GetActorSpawningMethodFromFightActorSpawnMethod(EFlowFightActorSpawnMethod Method)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowFightActorSpawnMethod, 6);
		FLOW_ASSERT_ENUM_MAX(EFlowActorSpawningMethod, 3);
		switch (Method)
		{
		case EFlowFightActorSpawnMethod::FullSpawnAllCohorts:
		case EFlowFightActorSpawnMethod::FullSpawnSingleCohort:
			return EFlowActorSpawningMethod::FullSpawn;
		case EFlowFightActorSpawnMethod::RefillSpawnAllCohorts:
		case EFlowFightActorSpawnMethod::RefillSpawnSingleCohort:
			return EFlowActorSpawningMethod::RefillMissing;
		case EFlowFightActorSpawnMethod::RefillSpawnSingleActor:
			return EFlowActorSpawningMethod::RefillSingleMissing;
		default:
			return EFlowActorSpawningMethod::Invalid;
		}
	}
}

/** Determines when a fight emits its terminal output. */
UENUM(BlueprintType)
enum class EFlowFightActorCompletionRule : uint8
{
	/** Finish once all cohorts are defeated and no reinforcement is scheduled or in progress. */
	AllActorsDefeatedWithNoReinforcementsScheduled,

	/** Finish only after all cohorts are defeated and a finite reinforcement addon is exhausted. */
	AllActorsDefeatedAndReinforcementsExhausted,

	/** Finish when the explicit defeat-and-disable input fires. */
	WhenDefeatAndDisableAutoReinforceIsTriggered,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowFightActorCompletionRule);

/** A fight's activation-local state, independent of Flow node activation. */
UENUM()
enum class EFlowFightActorState : int8
{
	/** Active fight waiting for spawn, defeat, timer, or input events. */
	Executing,

	/** Evaluate outstanding cohorts and reinforcement before finishing. */
	CheckCompletion,

	/** Request an eligible automatic reinforcement, then return to Executing. */
	RequestReinforcement,

	/** Terminal state that emits the all-defeated output. */
	Complete,

	Max UMETA(Hidden),
	Invalid = -1 UMETA(Hidden),
	Min = 0 UMETA(Hidden),

	CanCheckCompletionFirst = Executing UMETA(Hidden),
	CanCheckCompletionLast = Executing UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowFightActorState);

/** Selects the source of automatic reinforcement requests. */
UENUM(BlueprintType)
enum class EFlowFightAutoReinforcementTrigger : uint8
{
	/** Check the alive-plus-pending threshold after a fight actor is defeated. */
	WhenAnyActorDefeated,

	/** Request reinforcements on the configured automatic timer cadence. */
	WhenTimerExpires,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowFightAutoReinforcementTrigger);

/** Runtime states of a reinforcement addon. */
UENUM()
enum class EFlowFightReinforcementState : int8
{
	/** No delayed pass is pending; automatic cadence may be armed. */
	Idle,

	/** A requested pass is waiting for its start-delay timer. */
	Scheduled,

	/** The start delay elapsed and the host is starting a pass. */
	StartReinforcement,

	/** Cancel a scheduled or starting pass before returning to Idle. */
	AbortReinforcement,

	/** The configured number of productive rounds has completed. */
	Exhausted,

	Max UMETA(Hidden),
	Invalid = -1 UMETA(Hidden),
	Min = 0 UMETA(Hidden),

	EligibleForNewReinforcementFirst = Idle UMETA(Hidden),
	EligibleForNewReinforcementLast = Idle UMETA(Hidden),

	ScheduledOrInProgressFirst = Scheduled UMETA(Hidden),
	ScheduledOrInProgressLast = AbortReinforcement UMETA(Hidden),

	AbortableFirst = Scheduled UMETA(Hidden),
	AbortableLast = StartReinforcement UMETA(Hidden),

	StartDelayTimerFirst = Scheduled UMETA(Hidden),
	StartDelayTimerLast = Scheduled UMETA(Hidden),

	AutoReinforcementTimerFirst = Idle UMETA(Hidden),
	AutoReinforcementTimerLast = Idle UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowFightReinforcementState);

/** Aggregate counts from the fight's record-owning cohorts. */
struct FLOWGAMEFRAMEWORK_API FFlowFightActorCounts
{
	/** Members with an acquired actor that is still alive by their record's policy. */
	int32 AliveCount = 0;

	/** Records queued for or actively attempting to spawn. */
	int32 PendingSpawnCount = 0;

	/** Predicate-eligible authored count used as the reinforcement percentage denominator. */
	int32 InitialSpawnCount = 0;

	/** Records retained by the cohorts, whether alive, pending, or defeated. */
	int32 TotalCount = 0;

	int32 GetAliveAndPendingCount() const { return AliveCount + PendingSpawnCount; }

	/** An empty initial population reports 1.0, matching the existing fight threshold convention. */
	float CalculateAliveAndPendingVsInitialCountUnitPercent() const;
};

/** Authorable timing and pass policy for Flow fights. */
USTRUCT(BlueprintType)
struct FLOWGAMEFRAMEWORK_API FFlowFightReinforcementParameters
{
	GENERATED_BODY()

	/** Random delay after a request, before starting the reinforcement pass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration, meta = (ClampMin = 0, UIMin = 0))
	FFloatInterval StartDelayTimeRange = FFloatInterval(0.0f, 0.0f);

	/** Random delay before the next automatic timer-driven request. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration, meta = (ClampMin = 0, UIMin = 0,
		EditCondition = "AutoReinforcementTrigger == EFlowFightAutoReinforcementTrigger::WhenTimerExpires", EditConditionHides))
	FFloatInterval AutoReinforcementTimeRange = FFloatInterval(0.0f, 0.0f);

	/** Number of passes that queue actors; zero permits unlimited productive rounds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration, meta = (UIMin = 0, UIMax = 250))
	uint8 MaxReinforcementRounds = 0;

	/** Which cohorts or missing actor slots a reinforcement pass should spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration)
	EFlowFightActorSpawnMethod ReinforceMethod = EFlowFightActorSpawnMethod::FullSpawnAllCohorts;

	/** Choose defeat-threshold or timer-driven automatic requests. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration)
	EFlowFightAutoReinforcementTrigger AutoReinforcementTrigger =
		EFlowFightAutoReinforcementTrigger::WhenAnyActorDefeated;

	/** Request a pass when rounded alive-plus-pending percentage is at or below this threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration, meta = (ClampMin = 0, ClampMax = 100,
		EditCondition = "AutoReinforcementTrigger == EFlowFightAutoReinforcementTrigger::WhenAnyActorDefeated", EditConditionHides))
	uint8 AutoReinforcementPercentAlive = 0;

	/** Shuffle the cohort order for each subsequent reinforcement pass, not for the initial pass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration)
	bool bShuffleCohortSpawnOrderEachReinforcementRound = false;

	/** Start with automatic requests enabled; manual requests remain available when disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration)
	bool bEnableAutoReinforcement = false;
};
