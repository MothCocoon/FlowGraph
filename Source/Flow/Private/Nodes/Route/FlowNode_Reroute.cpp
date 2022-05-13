// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Reroute.h"

UFlowNode_Reroute::UFlowNode_Reroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
#endif
}

void UFlowNode_Reroute::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
