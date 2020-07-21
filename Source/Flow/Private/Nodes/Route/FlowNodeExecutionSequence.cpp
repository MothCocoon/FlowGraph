#include "Nodes/Route/FlowNodeExecutionSequence.h"

UFlowNodeExecutionSequence::UFlowNodeExecutionSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::Logic;
#endif

	SetNumericalOutputs(0, 1);
}

void UFlowNodeExecutionSequence::ExecuteInput(const FName& PinName)
{
	for (const FName& Output : OutputNames)
	{
		TriggerOutput(Output, false);
	}

	Finish();
}
