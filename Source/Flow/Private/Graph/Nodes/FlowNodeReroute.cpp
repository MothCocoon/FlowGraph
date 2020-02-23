#include "FlowNodeReroute.h"

UFlowNodeReroute::UFlowNodeReroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::Logic;
#endif
}

void UFlowNodeReroute::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}