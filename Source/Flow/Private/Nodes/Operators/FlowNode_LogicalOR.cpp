#include "Nodes/Operators/FlowNode_LogicalOR.h"

UFlowNode_LogicalOR::UFlowNode_LogicalOR(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Operators");
	NodeStyle = EFlowNodeStyle::Logic;
#endif

	SetNumericalInputs(0, 1);
}

void UFlowNode_LogicalOR::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
