#include "Nodes/Route/FlowNode_Delay.h"

#define FLOW_CPP_DELAY(TimeInSeconds, ...) GetWorld()->GetTimerManager().SetTimer(TimerHandle_Delay, FTimerDelegate().CreateLambda(__VA_ARGS__), TimeInSeconds, false);

UFlowNode_Delay::UFlowNode_Delay()
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Condition;
#endif

	DelayInSeconds = 0.2;

	OutputPins.Empty();
	OutputPins.Add(FFlowPin(TEXT("Finished"), TEXT("Executes after 'Delay In Seconds' time")));
}

void UFlowNode_Delay::ExecuteInput(const FName& PinName)
{
	if (DelayInSeconds < KINDA_SMALL_NUMBER)
	{
		TriggerFirstOutput(true);
	}
	else
	{
		FLOW_CPP_DELAY(DelayInSeconds, [&]()
		{
			TriggerFirstOutput(true);
		});
	}
}

void UFlowNode_Delay::Cleanup()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Delay);
}

#if WITH_EDITOR
FString UFlowNode_Delay::GetNodeDescription() const
{
	if (DelayInSeconds < KINDA_SMALL_NUMBER)
	{
		return "Executes instantly";
	}
	
	return FString::Printf(TEXT("Delay: %.4f"), DelayInSeconds);
}

FString UFlowNode_Delay::GetStatusString() const
{
	if (GetWorld() && DelayInSeconds > KINDA_SMALL_NUMBER)
	{
		return FString::Printf(TEXT("Remaining: %.2f"), GetWorld()->GetTimerManager().GetTimerRemaining(TimerHandle_Delay));
	}

	return "";
}
#endif

#undef FLOW_CPP_DELAY
