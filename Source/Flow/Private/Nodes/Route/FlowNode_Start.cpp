#include "Nodes/Route/FlowNode_Start.h"

UFlowNode_Start::UFlowNode_Start(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	InputPins = {};
}

void UFlowNode_Start::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
