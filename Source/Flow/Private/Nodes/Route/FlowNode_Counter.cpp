// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Counter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Counter)

UFlowNode_Counter::UFlowNode_Counter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Goal(2)
	, CurrentSum(0)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeDisplayStyle = FlowNodeStyle::Condition;
#endif

	InputPins.Empty();
	InputPins.Add(FFlowPin(TEXT("Increment")));
	InputPins.Add(FFlowPin(TEXT("Decrement")));
	InputPins.Add(FFlowPin(TEXT("Skip")));

	OutputPins.Empty();
	OutputPins.Add(FFlowPin(TEXT("Zero")));
	OutputPins.Add(FFlowPin(TEXT("Step")));
	OutputPins.Add(FFlowPin(TEXT("Goal")));
	OutputPins.Add(FFlowPin(TEXT("Skipped")));
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
