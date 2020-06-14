#include "Nodes/FlowNodeIn.h"

UFlowNodeIn::UFlowNodeIn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	InputNames = {};
}
