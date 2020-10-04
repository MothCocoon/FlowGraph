#include "Nodes/Route/FlowNode_Counter.h"

UFlowNode_Counter::UFlowNode_Counter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Goal(2)
	, CurrentSum(0)
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

void UFlowNode_Counter::ExecuteInput(const FName& PinName)
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

void UFlowNode_Counter::Cleanup()
{
	CurrentSum = 0;
}

#if WITH_EDITOR
FString UFlowNode_Counter::GetNodeDescription() const
{
	return FString::FromInt(Goal);
}

FString UFlowNode_Counter::GetStatusString() const
{
	return FString::FromInt(CurrentSum);
}
#endif
