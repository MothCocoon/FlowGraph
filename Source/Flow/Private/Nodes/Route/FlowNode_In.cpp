#include "Nodes/Route/FlowNode_In.h"

UFlowNode_In::UFlowNode_In(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	InputNames = {};
}
