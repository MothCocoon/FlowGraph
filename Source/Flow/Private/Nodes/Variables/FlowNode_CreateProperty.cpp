// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Variables/FlowNode_CreateProperty.h"

UFlowNode_CreateProperty::UFlowNode_CreateProperty()
{
#if WITH_EDITOR
	Category = TEXT("Variables");
	NodeStyle = EFlowNodeStyle::Default;
#endif
}

void UFlowNode_CreateProperty::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}
