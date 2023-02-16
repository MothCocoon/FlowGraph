// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Timer.h"

#include "Engine/World.h"
#include "TimerManager.h"

UFlowNode_Timer::UFlowNode_Timer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CompletionTime(1.0f)
	, StepTime(0.0f)
	, bLoopIndefinitely(false)
	, SumOfSteps(0.0f)
	, RemainingCompletionTime(0.0f)
	, RemainingStepTime(0.0f)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Latent;
#endif

	InputPins.Add(FFlowPin(TEXT("Skip")));
	InputPins.Add(FFlowPin(TEXT("Restart")));

	OutputPins.Empty();
	OutputPins.Add(FFlowPin(TEXT("Completed")));
	OutputPins.Add(FFlowPin(TEXT("Step")));
	OutputPins.Add(FFlowPin(TEXT("Skipped")));
}

void UFlowNode_Timer::ExecuteInput(const FName& PinName)
{
	if (CompletionTime == 0.0f && (!bLoopIndefinitely || StepTime == 0.f))
	{
		LogError(TEXT("Invalid Timer settings"));
		TriggerOutput(TEXT("Completed"), true);
		return;
	}

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

		if (!bLoopIndefinitely || StepTime == 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(CompletionTimerHandle, this, &UFlowNode_Timer::OnCompletion, CompletionTime, false);
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

void UFlowNode_Timer::OnStep()
{
	if (!bLoopIndefinitely)
	{
		SumOfSteps += StepTime;
	}

	if (SumOfSteps >= CompletionTime && CompletionTime > 0.f)
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
FString UFlowNode_Timer::GetNodeDescription() const
{
	if (StepTime > 0.0f && bLoopIndefinitely)
	{
		const auto NodeDescription = FString::SanitizeFloat(CompletionTime, 2).Append(TEXT(", step by ")).Append(FString::SanitizeFloat(StepTime, 2));
		return FString::Printf(TEXT("%s (Loops)"), *NodeDescription);
	}
	
	if (CompletionTime > 0.0f)
	{
		if (StepTime > 0.0f)
		{
			const auto NodeDescription = FString::SanitizeFloat(CompletionTime, 2).Append(TEXT(", step by ")).Append(FString::SanitizeFloat(StepTime, 2));
			if (bLoopIndefinitely)
			{
				return FString::Printf(TEXT("%s (Loops)"), *NodeDescription);
			}
			
			return NodeDescription;
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
