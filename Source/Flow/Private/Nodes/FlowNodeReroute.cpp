#include "Nodes/FlowNodeReroute.h"

UFlowNodeReroute::UFlowNodeReroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowNodeReroute::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
