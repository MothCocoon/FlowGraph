#include "Nodes/Route/FlowNodeReroute.h"

UFlowNodeReroute::UFlowNodeReroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
#endif
}

void UFlowNodeReroute::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
