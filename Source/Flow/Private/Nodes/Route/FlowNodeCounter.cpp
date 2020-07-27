#include "Nodes/Route/FlowNodeCounter.h"

UFlowNodeCounter::UFlowNodeCounter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Goal(2)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Condition;
#endif

	InputNames.Empty();
	InputNames.Add(TEXT("Increment"));
	InputNames.Add(TEXT("Decrement"));
	InputNames.Add(TEXT("Skip"));

	OutputNames.Empty();
	OutputNames.Add(TEXT("Zero"));
	OutputNames.Add(TEXT("Step"));
	OutputNames.Add(TEXT("Goal"));
	OutputNames.Add(TEXT("Skipped"));
}

void UFlowNodeCounter::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("Increment"))
	{		
		CurrentSum++;
		if (CurrentSum == Goal)
		{
			TriggerOutput(TEXT("Goal"), true);
		}
		else
		{
			TriggerOutput(TEXT("Step"));
		}
		return;
	}

	if (PinName == TEXT("Decrement"))
	{
		CurrentSum--;
		if (CurrentSum == 0)
		{
			TriggerOutput(TEXT("Zero"), true);
		}
		else
		{
			TriggerOutput(TEXT("Step"));
		}
		return;
	}

	if (PinName == TEXT("Skip"))
	{
		TriggerOutput(TEXT("Skipped"), true);
	}
}

void UFlowNodeCounter::Cleanup()
{
	CurrentSum = 0;
}

#if WITH_EDITOR
FString UFlowNodeCounter::GetNodeDescription() const
{
	return FString::FromInt(Goal);
}

FString UFlowNodeCounter::GetStatusString() const
{
	return FString::FromInt(CurrentSum);
}
#endif
