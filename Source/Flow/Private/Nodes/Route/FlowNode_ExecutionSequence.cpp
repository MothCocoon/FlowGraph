// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_ExecutionSequence.h"

UFlowNode_ExecutionSequence::UFlowNode_ExecutionSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Logic;
#endif

	SetNumberedOutputPins(0, 1);
}

void UFlowNode_ExecutionSequence::ExecuteInput(const FName& PinName)
{
	for (const FFlowPin& Output : OutputPins)
	{
		TriggerOutput(Output.PinName, false);
	}

	Finish();
}
