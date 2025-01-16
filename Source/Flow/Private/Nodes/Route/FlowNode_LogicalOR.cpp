// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_LogicalOR.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_LogicalOR)

UFlowNode_LogicalOR::UFlowNode_LogicalOR(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bEnabled(true)
	, ExecutionLimit(1)
	, ExecutionCount(0)
{
#if WITH_EDITOR
	Category = TEXT("Route|Logic");
	NodeDisplayStyle = FlowNodeStyle::Logic;
#endif

	SetNumberedInputPins(0, 1);
	InputPins.Add(FFlowPin(TEXT("Enable"), TEXT("Enabling resets Execution Count")));
	InputPins.Add(FFlowPin(TEXT("Disable"), TEXT("Disabling resets Execution Count")));
}

void UFlowNode_LogicalOR::ExecuteInput(const FName& PinName)
{
	if (PinName == TEXT("Enable"))
	{
		if (!bEnabled)
		{
			ResetCounter();
			bEnabled = true;
		}
		return;
	}

	if (PinName == TEXT("Disable"))
	{
		if (bEnabled)
		{
			bEnabled = false;
			Finish();
		}
		return;
	}

	if (bEnabled && PinName.ToString().IsNumeric())
	{
		ExecutionCount++;
		if (ExecutionLimit > 0 && ExecutionCount == ExecutionLimit)
		{
			bEnabled = false;
		}

		TriggerFirstOutput(true);
	}
}

void UFlowNode_LogicalOR::Cleanup()
{
	ResetCounter();
}

void UFlowNode_LogicalOR::ResetCounter()
{
	ExecutionCount = 0;
}
