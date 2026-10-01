// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn_ReinforcementBase.h"

#include "Engine/World.h"
#include "Interfaces/FlowFightActorsReinforcementHost.h"
#include "Nodes/FlowNode.h"
#include "TimerManager.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn_ReinforcementBase)

namespace
{
	float GetRandomDelay(const FFloatInterval& Range)
	{
		if (Range.Max > 0.0f)
		{
			return FMath::FRandRange(Range.Min, Range.Max);
		}

		return 0.0f;
	}
}

const FName UFlowNodeAddOn_ReinforcementBase::INPIN_RequestReinforce(TEXT("Request Reinforce"));
const FName UFlowNodeAddOn_ReinforcementBase::INPIN_EnableAutoReinforce(TEXT("Enable Auto-Reinforce"));
const FName UFlowNodeAddOn_ReinforcementBase::INPIN_DisableAutoReinforce(TEXT("Disable Auto-Reinforce"));

UFlowNodeAddOn_ReinforcementBase::UFlowNodeAddOn_ReinforcementBase()
{
	InputPins.Add(FFlowPin(INPIN_RequestReinforce));
	InputPins.Add(FFlowPin(INPIN_EnableAutoReinforce));
	InputPins.Add(FFlowPin(INPIN_DisableAutoReinforce));
}

void UFlowNodeAddOn_ReinforcementBase::OnActivate()
{
	Super::OnActivate();

#if WITH_SERVER_CODE
	bAutoReinforcementEnabled = Params.bEnableAutoReinforcement;
	SetReinforcementState(EFlowFightReinforcementState::Idle);
#endif
}

void UFlowNodeAddOn_ReinforcementBase::ExecuteInput(const FName& PinName)
{
#if WITH_SERVER_CODE
	if (PinName == INPIN_RequestReinforce)
	{
		RequestReinforcement();
	}
	else if (PinName == INPIN_EnableAutoReinforce)
	{
		const bool bWasEnabled = bAutoReinforcementEnabled;
		bAutoReinforcementEnabled = true;
		ReconcileAutoReinforcementTimer();
		if (!bWasEnabled)
		{
			if (IFlowFightActorsReinforcementHost* Host = GetFightHost();
				Host && ShouldAutoReinforceOnActorDefeated(Host->GetOverallAliveUnitPercent()))
			{
				RequestReinforcement();
			}
		}
	}
	else if (PinName == INPIN_DisableAutoReinforce)
	{
		AbortReinforcement(true);
	}
#endif

	Super::ExecuteInput(PinName);
}

void UFlowNodeAddOn_ReinforcementBase::Cleanup()
{
#if WITH_SERVER_CODE
	ResetReinforcementState();
#endif

	Super::Cleanup();
}

#if WITH_SERVER_CODE
bool UFlowNodeAddOn_ReinforcementBase::IsReinforcementScheduledOrInProgress() const
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	return FLOW_IS_ENUM_IN_SUBRANGE(ReinforcementState, EFlowFightReinforcementState::ScheduledOrInProgress);
}

bool UFlowNodeAddOn_ReinforcementBase::IsExhausted() const
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	return ReinforcementState == EFlowFightReinforcementState::Exhausted;
}

bool UFlowNodeAddOn_ReinforcementBase::ShouldAutoReinforceOnActorDefeated(float AliveUnitPercent) const
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	FLOW_ASSERT_ENUM_MAX(EFlowFightAutoReinforcementTrigger, 2);
	if (!bAutoReinforcementEnabled || ReinforcementState != EFlowFightReinforcementState::Idle
		|| Params.AutoReinforcementTrigger != EFlowFightAutoReinforcementTrigger::WhenAnyActorDefeated)
	{
		return false;
	}

	const int32 AlivePercent = FMath::RoundHalfFromZero(AliveUnitPercent * 100.0f);
	return AlivePercent <= FMath::Min<int32>(Params.AutoReinforcementPercentAlive, 100);
}

void UFlowNodeAddOn_ReinforcementBase::RequestReinforcement()
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	if (ReinforcementState == EFlowFightReinforcementState::Idle)
	{
		SetReinforcementState(EFlowFightReinforcementState::Scheduled);
	}
}

void UFlowNodeAddOn_ReinforcementBase::AbortReinforcement(bool bDisableAutoReinforcement)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	if (bDisableAutoReinforcement)
	{
		bAutoReinforcementEnabled = false;
	}

	if (FLOW_IS_ENUM_IN_SUBRANGE(ReinforcementState, EFlowFightReinforcementState::Abortable))
	{
		SetReinforcementState(EFlowFightReinforcementState::AbortReinforcement);
		UpdateReinforcementState();
	}

	ReconcileAutoReinforcementTimer();
	if (IFlowFightActorsReinforcementHost* Host = GetFightHost())
	{
		Host->ReevaluateFightCompletion();
	}
}

IFlowFightActorsReinforcementHost* UFlowNodeAddOn_ReinforcementBase::GetFightHost() const
{
	return Cast<IFlowFightActorsReinforcementHost>(GetFlowNode());
}

void UFlowNodeAddOn_ReinforcementBase::SetReinforcementState(EFlowFightReinforcementState NextState)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	if (ReinforcementState == NextState)
	{
		return;
	}

	const EFlowFightReinforcementState PreviousState = ReinforcementState;
	ReinforcementState = NextState;

	ManageScheduledReinforcementTimer(PreviousState, NextState);
	ReconcileAutoReinforcementTimer();

	if (NextState == EFlowFightReinforcementState::StartReinforcement)
	{
		StartReinforcement();
	}
}

void UFlowNodeAddOn_ReinforcementBase::UpdateReinforcementState()
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	int32 Transitions = 0;
	EFlowFightReinforcementState PreviousState = EFlowFightReinforcementState::Invalid;
	while (ReinforcementState != PreviousState && ReinforcementState != EFlowFightReinforcementState::Invalid)
	{
		check(++Transitions <= 8);
		PreviousState = ReinforcementState;
		if (ReinforcementState == EFlowFightReinforcementState::StartReinforcement)
		{
			if (Params.MaxReinforcementRounds > 0
				&& CurrentReinforcementRound >= Params.MaxReinforcementRounds)
			{
				SetReinforcementState(EFlowFightReinforcementState::Exhausted);
			}
			else
			{
				SetReinforcementState(EFlowFightReinforcementState::Idle);
			}
		}
		else if (ReinforcementState == EFlowFightReinforcementState::AbortReinforcement)
		{
			SetReinforcementState(EFlowFightReinforcementState::Idle);
		}
	}

	if (ReinforcementState != EFlowFightReinforcementState::Invalid)
	{
		if (IFlowFightActorsReinforcementHost* Host = GetFightHost())
		{
			Host->ReevaluateFightCompletion();
		}
	}
}

void UFlowNodeAddOn_ReinforcementBase::ResetReinforcementState()
{
	bAutoReinforcementEnabled = false;
	SetReinforcementState(EFlowFightReinforcementState::Invalid);
	CurrentReinforcementRound = 0;
}

void UFlowNodeAddOn_ReinforcementBase::ReconcileAutoReinforcementTimer()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FTimerManager& Timers = World->GetTimerManager();
	if (!ShouldHaveAutoReinforcementTimer())
	{
		Timers.ClearTimer(AutoReinforcementTimerHandle);
		return;
	}

	if (Timers.IsTimerActive(AutoReinforcementTimerHandle)
		|| Timers.IsTimerPending(AutoReinforcementTimerHandle))
	{
		return;
	}

	const float Delay = GetRandomDelay(Params.AutoReinforcementTimeRange);
	if (Delay > 0.0f)
	{
		Timers.SetTimer(AutoReinforcementTimerHandle, this,
			&UFlowNodeAddOn_ReinforcementBase::OnAutoReinforcementTimerExpired, Delay, false);
	}
	else
	{
		AutoReinforcementTimerHandle = Timers.SetTimerForNextTick(this,
			&UFlowNodeAddOn_ReinforcementBase::OnAutoReinforcementTimerExpired);
	}
}

void UFlowNodeAddOn_ReinforcementBase::ManageScheduledReinforcementTimer(
	EFlowFightReinforcementState PreviousState, EFlowFightReinforcementState NextState)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FTimerManager& Timers = World->GetTimerManager();
	if (PreviousState == EFlowFightReinforcementState::Scheduled)
	{
		Timers.ClearTimer(ScheduledReinforcementTimerHandle);
	}

	if (NextState != EFlowFightReinforcementState::Scheduled)
	{
		return;
	}

	const float Delay = GetRandomDelay(Params.StartDelayTimeRange);
	if (Delay > 0.0f)
	{
		Timers.SetTimer(ScheduledReinforcementTimerHandle, this,
			&UFlowNodeAddOn_ReinforcementBase::OnScheduledReinforcementTimerExpired, Delay, false);
	}
	else
	{
		ScheduledReinforcementTimerHandle = Timers.SetTimerForNextTick(this,
			&UFlowNodeAddOn_ReinforcementBase::OnScheduledReinforcementTimerExpired);
	}
}

bool UFlowNodeAddOn_ReinforcementBase::ShouldHaveAutoReinforcementTimer() const
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	FLOW_ASSERT_ENUM_MAX(EFlowFightAutoReinforcementTrigger, 2);
	return bAutoReinforcementEnabled
		&& ReinforcementState == EFlowFightReinforcementState::Idle
		&& Params.AutoReinforcementTrigger == EFlowFightAutoReinforcementTrigger::WhenTimerExpires;
}

void UFlowNodeAddOn_ReinforcementBase::StartReinforcement()
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	if (IFlowFightActorsReinforcementHost* Host = GetFightHost())
	{
		const int32 NumQueued = Host->ExecuteReinforcementPass(Params.ReinforceMethod,
			Params.bShuffleCohortSpawnOrderEachReinforcementRound);
		if (ReinforcementState == EFlowFightReinforcementState::StartReinforcement && NumQueued > 0)
		{
			++CurrentReinforcementRound;
		}
	}
}
#endif

void UFlowNodeAddOn_ReinforcementBase::OnAutoReinforcementTimerExpired()
{
#if WITH_SERVER_CODE
	if (ShouldHaveAutoReinforcementTimer())
	{
		RequestReinforcement();
	}
#endif
}

void UFlowNodeAddOn_ReinforcementBase::OnScheduledReinforcementTimerExpired()
{
#if WITH_SERVER_CODE
	FLOW_ASSERT_ENUM_MAX(EFlowFightReinforcementState, 5);
	if (ReinforcementState == EFlowFightReinforcementState::Scheduled)
	{
		SetReinforcementState(EFlowFightReinforcementState::StartReinforcement);
		UpdateReinforcementState();
	}
#endif
}

#if WITH_EDITOR
EDataValidationResult UFlowNodeAddOn_ReinforcementBase::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	if (Params.AutoReinforcementPercentAlive > 100)
	{
		Context.AddError(FText::FromString(TEXT("Auto-Reinforcement Percent Alive must be between 0 and 100.")));
		return EDataValidationResult::Invalid;
	}

	return ParentResult;
}
#endif
