// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Timer.h"
#include "FlowSettings.h"

#include "Engine/World.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Timer)

#define LOCTEXT_NAMESPACE "FlowNode_Timer"

FName UFlowNode_Timer::INPIN_CompletionTime;

UFlowNode_Timer::UFlowNode_Timer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CompletionTime(1.0f)
	, StepTime(0.0f)
	, ResolvedCompletionTime(0.0f)
	, SumOfSteps(0.0f)
	, RemainingCompletionTime(0.0f)
	, RemainingStepTime(0.0f)
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

	INPIN_CompletionTime = GET_MEMBER_NAME_CHECKED(UFlowNode_Timer, CompletionTime);
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
	FFlowDataPinResult_Float CompletionTimeResult = TryResolveDataPinAsFloat(INPIN_CompletionTime);

	if (CompletionTimeResult.Result == EFlowDataPinResolveResult::FailedMissingPin)
	{
		// Handle lookup of a UFlowNode_Timer that predated DataPins
		CompletionTimeResult.Result = EFlowDataPinResolveResult::Success;
		CompletionTimeResult.Value = CompletionTime;
	}

	check(CompletionTimeResult.Result == EFlowDataPinResolveResult::Success);

	return static_cast<float>(CompletionTimeResult.Value);
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
	TriggerOutput(TEXT("Completed"), true);
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

void UFlowNode_Timer::UpdateNodeConfigText_Implementation()
{
	constexpr bool bErrorIfInputPinNotFound = false;
	const bool bIsInputConnected = IsInputConnected(INPIN_CompletionTime);

	if (bIsInputConnected)
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

#endif

#undef LOCTEXT_NAMESPACE
