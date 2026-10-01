// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Actor/FlowNode_FightActorsBase.h"

#include "ActorSpawning/FlowActorSpawnRecord.h"
#include "AddOns/FlowNodeAddOn_ReinforcementBase.h"
#include "GameFramework/Actor.h"
#include "Interfaces/FlowFightActorsCohort.h"
#include "Nodes/FlowNodeBase.h"
#include "Types/FlowArray.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_FightActorsBase)

const FName UFlowNode_FightActorsBase::INPIN_DefeatAndDisableAutoReinforce(TEXT("Defeat And Disable Auto-Reinforce"));
const FName UFlowNode_FightActorsBase::OUTPIN_EachActorDefeated(TEXT("An Actor Defeated"));
const FName UFlowNode_FightActorsBase::OUTPIN_AllActorsDefeated(TEXT("All Actors Defeated"));

UFlowNode_FightActorsBase::UFlowNode_FightActorsBase()
{
	InputPins.Add(FFlowPin(INPIN_DefeatAndDisableAutoReinforce));
	OutputPins.Add(FFlowPin(OUTPIN_EachActorDefeated));
	OutputPins.Add(FFlowPin(OUTPIN_AllActorsDefeated));
}

void UFlowNode_FightActorsBase::ExecuteInput(const FName& PinName)
{
	Super::ExecuteInput(PinName);

#if WITH_SERVER_CODE
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorState, 4);
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorCompletionRule, 3);
	if (PinName == INPIN_DefeatAndDisableAutoReinforce && FightState != EFlowFightActorState::Invalid)
	{
		DefeatCohorts(true);

		if (FightState != EFlowFightActorState::Invalid
			&& CompletionRule == EFlowFightActorCompletionRule::WhenDefeatAndDisableAutoReinforceIsTriggered)
		{
			SetFightState(EFlowFightActorState::Complete);
		}
	}
#endif
}

void UFlowNode_FightActorsBase::Cleanup()
{
#if WITH_SERVER_CODE
	TGuardValue<bool> bCleanupGuard(bCleaningUpFight, true);
	SetFightState(EFlowFightActorState::Invalid);
#endif

	Super::Cleanup();
}

void UFlowNode_FightActorsBase::DeinitializeInstance()
{
#if WITH_SERVER_CODE
	SetFightState(EFlowFightActorState::Invalid);
#endif

	Super::DeinitializeInstance();
}

#if WITH_SERVER_CODE
void UFlowNode_FightActorsBase::StartSpawningPass()
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorState, 4);
	if (FightState != EFlowFightActorState::Invalid)
	{
		return;
	}

	SetFightState(EFlowFightActorState::Executing);

	const int32 NumQueued = ExecuteFightPass(InitialSpawnMethod, false, true);
	if (NumQueued >= 0 && ShouldCheckCompletionAfterPass(NumQueued > 0))
	{
		ReevaluateFightCompletion();
	}
}

int32 UFlowNode_FightActorsBase::ExecuteReinforcementPass(EFlowFightActorSpawnMethod Method, bool bShuffleCohorts)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorState, 4);
	if (FightState == EFlowFightActorState::Invalid)
	{
		return 0;
	}

	return ExecuteFightPass(Method, bShuffleCohorts, false);
}

int32 UFlowNode_FightActorsBase::ExecuteFightPass(
	EFlowFightActorSpawnMethod Method, bool bShuffleCohorts, bool bIsInitialPass)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorState, 4);
	using namespace EFlowFightActorSpawnMethod_Classifiers;
	if (!IsAnySpawnMethod(Method))
	{
		return 0;
	}

	FFlowActorSpawningAssistant* Assistant = SpawningAssistant.GetMutablePtr();
	if (!Assistant)
	{
		LogError(TEXT("Fight Actors has no spawning assistant."));
		return INDEX_NONE;
	}

	TArray<UFlowNodeAddOn*> Cohorts;
	ForEachAddOnForClass<UFlowFightActorsCohort>([&Cohorts](UFlowNodeAddOn& AddOn)
	{
		Cohorts.Add(&AddOn);
		return EFlowForEachAddOnFunctionReturnValue::Continue;
	});

	if (bShuffleCohorts)
	{
		FlowArray::ShuffleArray(Cohorts, CohortSpawnOrderRandomStream);
	}

	int32 NumQueued = 0;
	{
		TGuardValue<bool> Guard(bPreparingFightPass, true);
		for (UFlowNodeAddOn* AddOn : Cohorts)
		{
			if (!IsValid(AddOn) || FightState == EFlowFightActorState::Invalid)
			{
				break;
			}

			IFlowFightActorsCohort* Cohort = Cast<IFlowFightActorsCohort>(AddOn);

			const int32 CohortQueued = Cohort->ExecuteSpawningPass(Method, *Assistant, bIsInitialPass);
			NumQueued += CohortQueued;

			if (CohortQueued > 0 && 
				(IsSingleCohortMethod(Method)
				 || IsSingleActorMethod(Method)))
			{
				break;
			}
		}
	}
	return NumQueued;
}

void UFlowNode_FightActorsBase::DefeatAllSpawnedActors()
{
	DefeatCohorts(true);
}

void UFlowNode_FightActorsBase::DefeatCohorts(bool bDisableAutoReinforcement)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorState, 4);
	if (bDisableAutoReinforcement)
	{
		if (UFlowNodeAddOn_ReinforcementBase* Reinforcement = FindReinforcement())
		{
			Reinforcement->AbortReinforcement(true);
		}
	}

	TArray<UFlowNodeAddOn*> Cohorts;
	ForEachAddOnForClass<UFlowFightActorsCohort>([&Cohorts](UFlowNodeAddOn& AddOn)
	{
		Cohorts.Add(&AddOn);
		return EFlowForEachAddOnFunctionReturnValue::Continue;
	});

	for (UFlowNodeAddOn* AddOn : Cohorts)
	{
		if (IsValid(AddOn) && FightState != EFlowFightActorState::Invalid)
		{
			CastChecked<IFlowFightActorsCohort>(AddOn)->DefeatSpawnedActors();
		}
	}
}

void UFlowNode_FightActorsBase::CleanupActorRecords()
{
	ForEachAddOnForClass<UFlowFightActorsCohort>([](UFlowNodeAddOn& AddOn)
	{
		CastChecked<IFlowFightActorsCohort>(&AddOn)->CleanupActorRecords();
		return EFlowForEachAddOnFunctionReturnValue::Continue;
	});
}

bool UFlowNode_FightActorsBase::HasAnyOutstandingSpawns() const
{
	return bPreparingFightPass || GetFightActorCounts().PendingSpawnCount > 0;
}

FFlowFightActorCounts UFlowNode_FightActorsBase::GetFightActorCounts() const
{
	FFlowFightActorCounts Counts;
	ForEachAddOnForClassConst<UFlowFightActorsCohort>([&Counts](const UFlowNodeAddOn& AddOn)
	{
		CastChecked<IFlowFightActorsCohort>(&AddOn)->AppendActorCounts(Counts);
		return EFlowForEachAddOnFunctionReturnValue::Continue;
	});

	return Counts;
}

float UFlowNode_FightActorsBase::GetOverallAliveUnitPercent() const
{
	return GetFightActorCounts().CalculateAliveAndPendingVsInitialCountUnitPercent();
}

UFlowNodeAddOn_ReinforcementBase* UFlowNode_FightActorsBase::FindReinforcement() const
{
	for (UFlowNodeAddOn* AddOn : GetFlowNodeAddOnChildren())
	{
		if (UFlowNodeAddOn_ReinforcementBase* Reinforcement = Cast<UFlowNodeAddOn_ReinforcementBase>(AddOn))
		{
			return Reinforcement;
		}
	}

	return nullptr;
}

void UFlowNode_FightActorsBase::ReevaluateFightCompletion()
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorState, 4);
	if (FightState != EFlowFightActorState::Executing || bPreparingFightPass || bCleaningUpFight)
	{
		return;
	}

	SetFightState(EFlowFightActorState::CheckCompletion);
	UpdateFightState();
}

void UFlowNode_FightActorsBase::ReevaluateAfterDefeat()
{
	ReevaluateFightCompletion();
}

void UFlowNode_FightActorsBase::SetFightState(EFlowFightActorState NextState)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorState, 4);
	if (FightState == NextState)
	{
		return;
	}

	const EFlowFightActorState PreviousState = FightState;
	FightState = NextState;
	if (PreviousState == EFlowFightActorState::Invalid && NextState != EFlowFightActorState::Invalid)
	{
		CohortSpawnOrderRandomStream.Initialize(GetRandomSeed());
		OnFightInitialized(true);
	}

	if (NextState == EFlowFightActorState::Invalid && PreviousState != EFlowFightActorState::Invalid)
	{
		OnFightInitialized(false);
		if (UFlowNodeAddOn_ReinforcementBase* Reinforcement = FindReinforcement())
		{
			Reinforcement->AbortReinforcement(true);
		}
	}

	if (NextState == EFlowFightActorState::Complete)
	{
		TriggerOutput(OUTPIN_AllActorsDefeated, true);
	}
}

void UFlowNode_FightActorsBase::UpdateFightState()
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorState, 4);
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorCompletionRule, 3);

	EFlowFightActorState PreviousState = EFlowFightActorState::Invalid;
	int32 TransitionCount = 0;

	while (FightState != PreviousState && FightState != EFlowFightActorState::Invalid)
	{
		check(++TransitionCount <= 8);

		PreviousState = FightState;

		if (FightState == EFlowFightActorState::RequestReinforcement)
		{
			if (UFlowNodeAddOn_ReinforcementBase* Reinforcement = FindReinforcement())
			{
				Reinforcement->RequestReinforcement();
			}

			if (FightState == EFlowFightActorState::RequestReinforcement)
			{
				SetFightState(EFlowFightActorState::Executing);
			}

			continue;
		}

		if (FightState != EFlowFightActorState::CheckCompletion)
		{
			continue;
		}

		UFlowNodeAddOn_ReinforcementBase* Reinforcement = FindReinforcement();
		const FFlowFightActorCounts Counts = GetFightActorCounts();
		if (HasAnyOutstandingSpawns() || (Reinforcement && Reinforcement->IsReinforcementScheduledOrInProgress()))
		{
			SetFightState(EFlowFightActorState::Executing);
		}
		else if (Reinforcement && Reinforcement->ShouldAutoReinforceOnActorDefeated(
			Counts.CalculateAliveAndPendingVsInitialCountUnitPercent()))
		{
			SetFightState(EFlowFightActorState::RequestReinforcement);
		}
		else if (Counts.GetAliveAndPendingCount() == 0
			&& (CompletionRule == EFlowFightActorCompletionRule::AllActorsDefeatedWithNoReinforcementsScheduled
				|| (CompletionRule == EFlowFightActorCompletionRule::AllActorsDefeatedAndReinforcementsExhausted
					&& (!Reinforcement || Reinforcement->IsExhausted()))))
		{
			SetFightState(EFlowFightActorState::Complete);
		}
		else
		{
			SetFightState(EFlowFightActorState::Executing);
		}
	}
}

void UFlowNode_FightActorsBase::NotifyCohortPreSpawn(UFlowActorSpawnRecord& Record)
{
	HandlePreSpawnConfiguration(Record);
}

void UFlowNode_FightActorsBase::NotifyCohortPreFinishSpawn(UFlowActorSpawnRecord& Record)
{
	HandlePreFinishSpawnActor(Record);
}

void UFlowNode_FightActorsBase::NotifyCohortPostSpawn(UFlowActorSpawnRecord& Record)
{
	HandlePostSpawnConfiguration(Record);
	OnCohortActorSpawned(Record);
}

void UFlowNode_FightActorsBase::NotifyCohortSpawnAttemptFinished(UFlowActorSpawnRecord& Record)
{
	HandleFinishedSpawnAttempt(Record);
}

void UFlowNode_FightActorsBase::NotifyCohortActorCleanup(UFlowActorSpawnRecord& Record)
{
	OnBeforeCohortActorCleanup(Record);
	HandleCleanupSpawnedInstance(Record);
}

void UFlowNode_FightActorsBase::NotifyActorDefeated(AActor& Actor)
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorState, 4);
	if (FightState == EFlowFightActorState::Executing)
	{
		TriggerOutput(OUTPIN_EachActorDefeated, false);
	}

	ReevaluateAfterDefeat();
}

bool UFlowNode_FightActorsBase::ShouldCheckCompletionAfterPass(bool bQueuedAnyRecords) const
{
	return false;
}

void UFlowNode_FightActorsBase::OnCohortActorSpawned(UFlowActorSpawnRecord& Record)
{
}

void UFlowNode_FightActorsBase::OnBeforeCohortActorCleanup(UFlowActorSpawnRecord& Record)
{
}
#endif

EFlowAddOnAcceptResult UFlowNode_FightActorsBase::AcceptFlowNodeAddOnChild_Implementation(
	const UFlowNodeAddOn* AddOnTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (AddOnTemplate->IsClassOrImplementsInterface(*UFlowFightActorsCohort::StaticClass()))
	{
		return EFlowAddOnAcceptResult::TentativeAccept;
	}

	if (AddOnTemplate->IsA<UFlowNodeAddOn_ReinforcementBase>())
	{
		if (HasOtherDirectAddOnChildMatching(*UFlowNodeAddOn_ReinforcementBase::StaticClass(),
			AddOnTemplate, AdditionalAddOnsToAssumeAreChildren))
		{
			return EFlowAddOnAcceptResult::Reject;
		}

		return EFlowAddOnAcceptResult::TentativeAccept;
	}

	return Super::AcceptFlowNodeAddOnChild_Implementation(AddOnTemplate, AdditionalAddOnsToAssumeAreChildren);
}

void UFlowNode_FightActorsBase::UpdateNodeConfigText_Implementation()
{
#if WITH_EDITOR
	FTextBuilder TextBuilder;
	TextBuilder.AppendLine(FText::FromString(FString::Printf(TEXT("Initially, %s"),
		*StaticEnum<EFlowFightActorSpawnMethod>()->GetDisplayValueAsText(InitialSpawnMethod).ToString())));
	TextBuilder.AppendLine(FText::FromString(FString::Printf(TEXT("Complete when %s"),
		*StaticEnum<EFlowFightActorCompletionRule>()->GetDisplayValueAsText(CompletionRule).ToString())));
	SetNodeConfigText(TextBuilder.ToText());
#endif
}

#if WITH_EDITOR
EDataValidationResult UFlowNode_FightActorsBase::ValidateNode()
{
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorCompletionRule, 3);
	FLOW_ASSERT_ENUM_MAX(EFlowFightActorSpawnMethod, 6);
	EDataValidationResult Result = Super::ValidateNode();
	FDataValidationContext Context;
	for (const UFlowNodeAddOn* AddOn : GetFlowNodeAddOnChildren())
	{
		if (IsValid(AddOn))
		{
			Result = CombineDataValidationResults(Result, AddOn->IsDataValid(Context));
		}
	}

	if (const UFlowNodeAddOn_ReinforcementBase* Reinforcement = FindReinforcement();
		Reinforcement && CompletionRule == EFlowFightActorCompletionRule::AllActorsDefeatedAndReinforcementsExhausted)
	{
		const FFlowFightReinforcementParameters& Parameters = Reinforcement->GetParameters();
		if (Parameters.MaxReinforcementRounds == 0)
		{
			Context.AddError(FText::FromString(TEXT("ReinforcementsExhausted requires a finite MaxReinforcementRounds.")));
			Result = EDataValidationResult::Invalid;
		}
		else if (Parameters.ReinforceMethod == EFlowFightActorSpawnMethod::NoSpawning)
		{
			Context.AddError(FText::FromString(TEXT("ReinforcementsExhausted cannot use NoSpawning.")));
			Result = EDataValidationResult::Invalid;
		}
	}

	for (const FDataValidationContext::FIssue& Issue : Context.GetIssues())
	{
		ValidationLog.Error<UFlowNode>(*Issue.Message.ToString(), this);
	}

	return Result;
}
#endif
