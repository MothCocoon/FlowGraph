// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Variables/FlowNodeVariable.h"

UFlowNodeVariable::UFlowNodeVariable()
{
#if WITH_EDITOR
	Category = TEXT("Variables");
	NodeStyle = EFlowNodeStyle::Default;
#endif
}
