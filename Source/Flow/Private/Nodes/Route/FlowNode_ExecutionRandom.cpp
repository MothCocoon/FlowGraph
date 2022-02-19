#include "Nodes/Route/FlowNode_ExecutionRandom.h"

UFlowNode_ExecutionRandom::UFlowNode_ExecutionRandom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Logic;
#endif

	SetNumberedOutputPins(0, 1);
}

void UFlowNode_ExecutionRandom::ExecuteInput(const FName& PinName)
{
	const int32 LastPinIndex = OutputPins.Num() - 1;
	const FFlowPin& SelectedPin = OutputPins[FMath::RandRange(0, LastPinIndex)];

	TriggerOutput(SelectedPin.PinName, false);

	Finish();
}
