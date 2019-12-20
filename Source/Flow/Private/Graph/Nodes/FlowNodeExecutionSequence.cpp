#include "FlowNodeExecutionSequence.h"

UFlowNodeExecutionSequence::UFlowNodeExecutionSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::Logic;
#endif

	SetNumericalOutputs(0, 1);
}

void UFlowNodeExecutionSequence::ExecuteInput(const FName& PinName)
{
	CreatedOutputs.ValueSort([](const uint8& A, const uint8& B) {
		return A < B;
	});

	for (const TPair<FName, uint8>& Output : CreatedOutputs)
	{
		TriggerOutput(Output.Key, false);
	}

	Finish();
}
