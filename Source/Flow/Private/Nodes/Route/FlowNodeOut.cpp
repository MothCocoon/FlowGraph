#include "Nodes/Route/FlowNodeOut.h"

UFlowNodeOut::UFlowNodeOut(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	OutputNames = {};
}
