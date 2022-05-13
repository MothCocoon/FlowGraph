// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Start.h"

UFlowNode_Start::UFlowNode_Start(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
	bCanDelete = bCanDuplicate = false;
#endif

	InputPins = {};
}

void UFlowNode_Start::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
