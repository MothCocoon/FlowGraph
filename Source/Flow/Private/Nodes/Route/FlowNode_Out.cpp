#include "Nodes/Route/FlowNode_Out.h"

UFlowNode_Out::UFlowNode_Out(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	OutputNames = {};
}
