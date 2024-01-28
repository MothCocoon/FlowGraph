// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Reroute.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Reroute)

UFlowNode_Reroute::UFlowNode_Reroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
#endif

	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

void UFlowNode_Reroute::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
