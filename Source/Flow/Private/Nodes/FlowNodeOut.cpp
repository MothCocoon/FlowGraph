#include "Nodes/FlowNodeOut.h"

UFlowNodeOut::UFlowNodeOut(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	OutputNames = {};
}
