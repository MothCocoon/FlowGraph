#include "Nodes/Operators/FlowNodeLogicalOR.h"

UFlowNodeLogicalOR::UFlowNodeLogicalOR(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::Logic;
#endif

	SetNumericalInputs(0, 1);
}

void UFlowNodeLogicalOR::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
