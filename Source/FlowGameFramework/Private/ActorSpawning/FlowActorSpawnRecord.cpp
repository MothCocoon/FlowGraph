// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "ActorSpawning/FlowActorSpawnRecord.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include "ActorSpawning/FlowActorSpawnRecordOwner.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowActorSpawnRecord)

UWorld* UFlowActorSpawnRecord::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject) || GetOuter() == nullptr)
	{
		return nullptr;
	}

	return GetOuter()->GetWorld();
}

#if WITH_SERVER_CODE
bool UFlowActorSpawnRecord::Configure(TSubclassOf<AActor> InActorClass, int32 InRecordIndex, bool bIsRespawn)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client || !IsValid(InActorClass))
	{
		return false;
	}

	if (bIsRespawn)
	{
		SetRecordState(EFlowActorSpawnRecordState::CleanupForRespawn);
	}
	else if (RecordState != EFlowActorSpawnRecordState::Invalid)
	{
		return false;
	}

	ConfiguredActorClass = InActorClass;
	RecordIndex = InRecordIndex;
	++ConfigurationGeneration;
	SetRecordState(EFlowActorSpawnRecordState::Configured);

	return true;
}

void UFlowActorSpawnRecord::CleanupRuntime()
{
	SetRecordState(EFlowActorSpawnRecordState::Invalid);

	ConfiguredActorClass = nullptr;
	RecordIndex = INDEX_NONE;
}

bool UFlowActorSpawnRecord::TryQueueRecordForSpawning()
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);
	const UWorld* World = GetWorld();
	if (RecordState != EFlowActorSpawnRecordState::Configured ||
		!IsValid(World) ||
		World->GetNetMode() == NM_Client ||
		!IsValid(ConfiguredActorClass))
	{
		return false;
	}

	SetRecordState(EFlowActorSpawnRecordState::QueuedForSpawn);
	return true;
}

bool UFlowActorSpawnRecord::TryStartSpawningProcess()
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);
	const UWorld* World = GetWorld();
	if (RecordState != EFlowActorSpawnRecordState::QueuedForSpawn ||
		!IsValid(World) ||
		World->GetNetMode() == NM_Client)
	{
		return false;
	}

	SetRecordState(EFlowActorSpawnRecordState::PreSpawnConfiguration);
	UpdateRecordState();
	return true;
}

bool UFlowActorSpawnRecord::IsActivelyAttemptingToSpawn() const
{
	return EFlowActorSpawnRecordState_Classifiers::IsActiveSpawningState(RecordState);
}

bool UFlowActorSpawnRecord::IsSpawnedActorAlive() const
{
	// "Basic" version of the Is Alive question, which could be expanded in subclasses.
	return IsValid(GetOwnedActor());
}

void UFlowActorSpawnRecord::CompleteSpawnLocation(bool bSucceeded)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);
	if (RecordState != EFlowActorSpawnRecordState::WaitingForSpawnLocation)
	{
		return;
	}

	const EFlowActorSpawnRecordState NextState =
		(bSucceeded)
			? EFlowActorSpawnRecordState::TrySpawnActor
			: EFlowActorSpawnRecordState::FailedSpawn;

	SetRecordState(NextState);
	UpdateRecordState();
}

void UFlowActorSpawnRecord::SetRecordState(EFlowActorSpawnRecordState NextState)
{
	if (RecordState == NextState)
	{
		return;
	}

	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);
	const EFlowActorSpawnRecordState PreviousState = RecordState;
	RecordState = NextState;

	OnStateChanged(PreviousState, NextState);

	if (RecordState != NextState)
	{
		// An owner callback may synchronously cancel or reconfigure this record; do not continue the old edge.
		return;
	}

	if (NextState == EFlowActorSpawnRecordState::FailedSpawn
		|| NextState == EFlowActorSpawnRecordState::CleanupForRespawn
		|| NextState == EFlowActorSpawnRecordState::Invalid)
	{
		ReleaseOwnedActor(true);

		if (RecordState != NextState)
		{
			// Cleanup callbacks may start another pass; never report the previous attempt as finished.
			return;
		}
	}

	IFlowActorSpawnRecordOwner* Owner = GetRecordOwner();
	if (!Owner)
	{
		return;
	}

	if (NextState == EFlowActorSpawnRecordState::PreSpawnConfiguration)
	{
		Owner->ProcessPreSpawnConfiguration(*this);
	}
	else if (NextState == EFlowActorSpawnRecordState::FinishSpawnActor)
	{
		Owner->ProcessPreFinishSpawnActor(*this);
	}
	else if (NextState == EFlowActorSpawnRecordState::PostSpawnConfigureActor)
	{
		Owner->ProcessPostSpawnConfiguration(*this);
	}
	else if (EFlowActorSpawnRecordState_Classifiers::IsActiveSpawningState(PreviousState)
			 && (EFlowActorSpawnRecordState_Classifiers::IsSuccessfulSpawnState(NextState)
				 || EFlowActorSpawnRecordState_Classifiers::IsFailedSpawnState(NextState)))
	{
		Owner->ProcessFinishedSpawnAttempt(*this);
	}
}

void UFlowActorSpawnRecord::UpdateRecordState()
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);

	// Settle consecutive immediate transitions in one call, stopping at an async wait or terminal state.
	EFlowActorSpawnRecordState PreviousState = EFlowActorSpawnRecordState::Invalid;
	int32 TransitionCount = 0;
	while (RecordState != PreviousState && FlowEnum::IsValidEnumValue(RecordState))
	{
		check(++TransitionCount <= 10);

		PreviousState = RecordState;

		if (!IsActivelyAttemptingToSpawn())
		{
			break;
		}

		EFlowActorSpawnRecordState NextState = RecordState;
		if (AdvanceCurrentState(NextState) && RecordState == PreviousState)
		{
			SetRecordState(NextState);
		}
	}
}

bool UFlowActorSpawnRecord::AdvanceCurrentState(EFlowActorSpawnRecordState& OutNextState)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);
	FLOW_ASSERT_ENUM_VALUE(EFlowActorSpawnRecordState::WaitingForSpawnLocation, 3);
	FLOW_ASSERT_ENUM_VALUE(EFlowActorSpawnRecordState::TrySpawnActor, 4);
	FLOW_ASSERT_ENUM_VALUE(EFlowActorSpawnRecordState::SuccessfulSpawn, 7);

	switch (RecordState)
	{
	case EFlowActorSpawnRecordState::PreSpawnConfiguration:
	case EFlowActorSpawnRecordState::WaitingForSpawnLocation:
	case EFlowActorSpawnRecordState::PostSpawnConfigureActor:
		{
			OutNextState = static_cast<EFlowActorSpawnRecordState>(FlowEnum::ToInt(RecordState) + 1);
			return true;
		}

	case EFlowActorSpawnRecordState::TrySpawnActor:
		{
			ResolvedSpawnTransform = GetDesiredSpawnTransform();
			if (AcquireActor())
			{
				OutNextState = EFlowActorSpawnRecordState::FinishSpawnActor;
			}
			else
			{
				OutNextState = EFlowActorSpawnRecordState::FailedSpawn;
			}
			return true;
		}

	case EFlowActorSpawnRecordState::FinishSpawnActor:
		{
			if (FinishAcquiredActor())
			{
				OutNextState = EFlowActorSpawnRecordState::PostSpawnConfigureActor;
			}
			else
			{
				OutNextState = EFlowActorSpawnRecordState::FailedSpawn;
			}
			return true;
		}

	default:
		return false;
	}
}

FTransform UFlowActorSpawnRecord::GetDesiredSpawnTransform() const
{
	if (const IFlowActorSpawnRecordOwner* Owner = GetRecordOwner())
	{
		if (const AActor* ActorOwner = Owner->TryGetActorOwner(); IsValid(ActorOwner))
		{
			return ActorOwner->GetActorTransform();
		}
	}

	return FTransform::Identity;
}

IFlowActorSpawnRecordOwner* UFlowActorSpawnRecord::GetRecordOwner() const
{
	UObject* Outer = GetOuter();
	if (!IsValid(Outer))
	{
		return nullptr;
	}

	return Cast<IFlowActorSpawnRecordOwner>(Outer);
}
#endif
