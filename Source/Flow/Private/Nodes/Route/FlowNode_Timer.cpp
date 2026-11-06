// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Route/FlowNode_Timer.h"

#include "Engine/World.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Timer)

#define LOCTEXT_NAMESPACE "FlowNode_Timer"

FName UFlowNode_Timer::INPIN_CompletionTime;

UFlowNode_Timer::UFlowNode_Timer()
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeDisplayStyle = FlowNodeStyle::Latent;
#endif

	InputPins.Add(FFlowPin(TEXT("Skip")));
	InputPins.Add(FFlowPin(TEXT("Restart")));

	OutputPins.Empty();
	OutputPins.Add(FFlowPin(TEXT("Completed")));
	OutputPins.Add(FFlowPin(TEXT("Step")));
	OutputPins.Add(FFlowPin(TEXT("Skipped")));

	INPIN_CompletionTime = GET_MEMBER_NAME_CHECKED(ThisClass, CompletionTime);
}

void UFlowNode_Timer::InitializeInstance()
{
	Super::InitializeInstance();

	// Initialize to the configured value,
	// but we will overwrite this with the results of ResolveCompletionTime() when the timer is started
	ResolvedCompletionTime = CompletionTime;
}

void UFlowNode_Timer::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("In"))
	{
		if (CompletionTimerHandle.IsValid() || StepTimerHandle.IsValid())
		{
			LogError(TEXT("Timer already active"));
			return;
		}

		SetTimer();
	}
	else if (PinName == TEXT("Skip"))
	{
		TriggerOutput(TEXT("Skipped"), true);
	}
	else if (PinName == TEXT("Restart"))
	{
		Restart();
	}
}

void UFlowNode_Timer::FastForwardInput(const FName& PinName, const FName& OutputPinName)
{
	const bool bFastForwardCompleted = OutputPinName == TEXT("Completed");

	if (bFastForwardCompleted || OutputPinName == TEXT("Step"))
	{
		// Timer starts like after a real input
		if (!PinName.IsNone())
		{
			ExecuteInputForSelfAndAddOns(PinName);
		}

		// Time passes at once, so steps fire like while waiting
		const int32 MaxSteps = StepTime > 0.0f ? FMath::CeilToInt(ResolvedCompletionTime / StepTime) : 0;
		for (int32 StepIndex = 0; StepIndex < MaxSteps && GetActivationState() == EFlowNodeState::Active && (bFastForwardCompleted || StepIndex == 0); ++StepIndex)
		{
			OnStep();
		}

		if (bFastForwardCompleted && GetActivationState() == EFlowNodeState::Active)
		{
			OnCompletion();
		}

		// Running timers continue from the fast forwarded step
		if (!bFastForwardCompleted && GetActivationState() == EFlowNodeState::Active && GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(StepTimerHandle, this, &UFlowNode_Timer::OnStep, StepTime, true);
			GetWorld()->GetTimerManager().SetTimer(CompletionTimerHandle, this, &UFlowNode_Timer::OnCompletion, FMath::Max(ResolvedCompletionTime - SumOfSteps, UE_KINDA_SMALL_NUMBER), false);
		}
	}
	else
	{
		Super::FastForwardInput(PinName, OutputPinName);
	}
}

bool UFlowNode_Timer::CanFastForward(const FName& InputPinName, const FName& OutputPinName) const
{
	// Only Skip input finishes the timer by Skipped output, Step output requires step time
	return (InputPinName == TEXT("Skip")) == (OutputPinName == TEXT("Skipped")) && (OutputPinName != TEXT("Step") || StepTime > 0.0f);
}

void UFlowNode_Timer::SetTimer()
{
	if (GetWorld())
	{
		if (StepTime > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(StepTimerHandle, this, &UFlowNode_Timer::OnStep, StepTime, true);
		}

		ResolvedCompletionTime = ResolveCompletionTime();
		if (ResolvedCompletionTime > UE_KINDA_SMALL_NUMBER)
		{
			GetWorld()->GetTimerManager().SetTimer(CompletionTimerHandle, this, &UFlowNode_Timer::OnCompletion, ResolvedCompletionTime, false);
		}
		else
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UFlowNode_Timer::OnCompletion);
		}
	}
	else
	{
		LogError(TEXT("No valid world"));
		TriggerOutput(TEXT("Completed"), true);
	}
}

void UFlowNode_Timer::Restart()
{
	Cleanup();

	RemainingStepTime = 0.0f;
	RemainingCompletionTime = 0.0f;

	SetTimer();
}

float UFlowNode_Timer::ResolveCompletionTime() const
{
	// Get the CompletionTime from either the default (property) or the data pin (if connected)
	float ResolvedTime = CompletionTime;
	TryResolveDataPinValue<FFlowPinType_Float>(INPIN_CompletionTime, ResolvedTime);

	return ResolvedTime;
}

void UFlowNode_Timer::OnStep()
{
	SumOfSteps += StepTime;

	if (SumOfSteps >= ResolvedCompletionTime)
	{
		TriggerOutput(TEXT("Completed"), true);
	}
	else
	{
		TriggerOutput(TEXT("Step"));
	}
}

void UFlowNode_Timer::OnCompletion()
{
	// Completion scheduled for the next tick outlives the timer finished in the meantime
	if (GetActivationState() == EFlowNodeState::Active)
	{
		TriggerOutput(TEXT("Completed"), true);
	}
}

void UFlowNode_Timer::Cleanup()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CompletionTimerHandle);
	}
	CompletionTimerHandle.Invalidate();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(StepTimerHandle);
	}
	StepTimerHandle.Invalidate();

	SumOfSteps = 0.0f;

	Super::Cleanup();
}

void UFlowNode_Timer::OnSave_Implementation()
{
	if (GetWorld())
	{
		if (CompletionTimerHandle.IsValid())
		{
			RemainingCompletionTime = GetWorld()->GetTimerManager().GetTimerRemaining(CompletionTimerHandle);
		}

		if (StepTimerHandle.IsValid())
		{
			RemainingStepTime = GetWorld()->GetTimerManager().GetTimerRemaining(StepTimerHandle);
		}
	}
}

void UFlowNode_Timer::OnLoad_Implementation()
{
	if (RemainingStepTime > 0.0f || RemainingCompletionTime > 0.0f)
	{
		if (RemainingStepTime > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(StepTimerHandle, this, &UFlowNode_Timer::OnStep, StepTime, true, RemainingStepTime);
		}

		GetWorld()->GetTimerManager().SetTimer(CompletionTimerHandle, this, &UFlowNode_Timer::OnCompletion, RemainingCompletionTime, false);

		RemainingStepTime = 0.0f;
		RemainingCompletionTime = 0.0f;
	}
}

#if WITH_EDITOR
FString UFlowNode_Timer::GetStatusString() const
{
	FString ProgressString;
	if (StepTime > 0.0f)
	{
		ProgressString = FString::Printf(TEXT("%.*f"), 2, SumOfSteps);
	}
	else if (CompletionTimerHandle.IsValid() && GetWorld())
	{
		ProgressString = FString::Printf(TEXT("%.*f"), 2, GetWorld()->GetTimerManager().GetTimerElapsed(CompletionTimerHandle));
	}

	if (!ProgressString.IsEmpty())
	{
		return FText::Format(LOCTEXT("ProgressStatus", "Progress: {0}"), { FText::FromString(ProgressString) }).ToString();
	}

	return FString();
}

void UFlowNode_Timer::UpdateNodeConfigText_Implementation()
{
	if (IsInputConnected(INPIN_CompletionTime))
	{
		// CompletionTime will be sourced from the data pin

		if (StepTime > 0.0f)
		{
			const FString StepTimeString = FString::Printf(TEXT("%.*f"), 2, StepTime);

			SetNodeConfigText(FText::Format(LOCTEXT("TimerConfigPinWithStep", "Step by {1}"), { FText::FromString(StepTimeString) }));
		}
		else
		{
			SetNodeConfigText(FText());
		}

		return;
	}

	if (CompletionTime > UE_KINDA_SMALL_NUMBER)
	{
		const FString CompletionTimeString = FString::Printf(TEXT("%.*f"), 2, CompletionTime);

		if (StepTime > 0.0f)
		{
			const FString StepTimeString = FString::Printf(TEXT("%.*f"), 2, StepTime);

			SetNodeConfigText(FText::Format(LOCTEXT("TimerConfigWithStep", "Time: {0}, step by {1}"), { FText::FromString(CompletionTimeString), FText::FromString(StepTimeString) }));
		}
		else
		{
			SetNodeConfigText(FText::Format(LOCTEXT("TimerConfig", "Time: {0}"), { FText::FromString(CompletionTimeString) }));
		}
	}
	else
	{
		SetNodeConfigText(FText(LOCTEXT("CompletesNextTick", "Completes in next tick")));
	}
}
#endif

#undef LOCTEXT_NAMESPACE

#if WITH_EDITOR
const FFlowAgentDoc& UFlowNode_Timer::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("CompletionTime is overridable via an input data pin at runtime - if that pin is connected, the property value is ignored in favor of the resolved data pin value. Set StepTime to 0 to disable periodic Step firings."),
		/*Tags*/     { TEXT("route"), TEXT("timer"), TEXT("delay") },
		/*Articles*/ {  });
	return Doc;
}
#endif
