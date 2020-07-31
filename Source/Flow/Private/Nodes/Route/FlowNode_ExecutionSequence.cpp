#include "Nodes/Route/FlowNode_ExecutionSequence.h"

UFlowNode_ExecutionSequence::UFlowNode_ExecutionSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Logic;
#endif

	SetNumericalOutputs(0, 1);
}

void UFlowNode_ExecutionSequence::ExecuteInput(const FName& PinName)
{
	for (const FName& Output : OutputNames)
	{
		TriggerOutput(Output, false);
	}

	Finish();
}
