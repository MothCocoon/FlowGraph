#include "Nodes/Route/FlowNode_Timer.h"

#include "Engine/World.h"
#include "TimerManager.h"

UFlowNode_Timer::UFlowNode_Timer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CompletionTime(1.0f)
	, StepTime(0.0f)
	, SumOfSteps(0.0f)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Latent;
#endif

	InputNames.Add(TEXT("Skip"));

	OutputNames.Empty();
	OutputNames.Add(TEXT("Completed"));
	OutputNames.Add(TEXT("Step"));
	OutputNames.Add(TEXT("Skipped"));
}

void UFlowNode_Timer::ExecuteInput(const FName& PinName)
{
	if (CompletionTime == 0.0f)
	{
		LogError("Invalid Timer settings");
		TriggerOutput(TEXT("Completed"), true);
		return;
	}

	if (PinName == TEXT("In"))
	{
		if (CompletionTimerHandle.IsValid() || StepTimerHandle.IsValid())
		{
			LogError("Timer already active");
			return;
		}

		if (GetWorld())
		{
			if (StepTime > 0.0f)
			{
				GetWorld()->GetTimerManager().SetTimer(StepTimerHandle, this, &UFlowNode_Timer::OnStep, StepTime, true);
			}

			GetWorld()->GetTimerManager().SetTimer(CompletionTimerHandle, this, &UFlowNode_Timer::OnCompletion, CompletionTime, false);
		}
		else
		{
			LogError("No valid world");
			TriggerOutput(TEXT("Completed"), true);
		}
	}
	else if (PinName == TEXT("Skip"))
	{
		TriggerOutput(TEXT("Skipped"), true);
	}
}

void UFlowNode_Timer::OnStep()
{
	SumOfSteps += StepTime;

	if (SumOfSteps >= CompletionTime)
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

#if WITH_EDITOR
FString UFlowNode_Timer::GetNodeDescription() const
{
	if (CompletionTime > 0.0f)
	{
		if (StepTime > 0.0f)
		{
			return FString::SanitizeFloat(CompletionTime, 2) + TEXT(", step by ") + FString::SanitizeFloat(StepTime, 2);
		}

		return FString::SanitizeFloat(CompletionTime, 2);
	}

	return TEXT("Invalid settings");
}

FString UFlowNode_Timer::GetStatusString() const
{
	if (StepTime > 0.0f)
	{
		return TEXT("Progress: ") + GetProgressAsString(SumOfSteps);
	}

	if (CompletionTimerHandle.IsValid() && GetWorld())
	{
		return TEXT("Progress: ") + GetProgressAsString(GetWorld()->GetTimerManager().GetTimerElapsed(CompletionTimerHandle));
	}

	return FString();
}
#endif
