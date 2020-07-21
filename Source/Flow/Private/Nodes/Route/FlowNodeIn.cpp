#include "Nodes/Route/FlowNodeIn.h"

UFlowNodeIn::UFlowNodeIn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	InputNames = {};
}
