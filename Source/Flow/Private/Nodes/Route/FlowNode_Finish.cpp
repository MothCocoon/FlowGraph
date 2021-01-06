#include "Nodes/Route/FlowNode_Finish.h"

UFlowNode_Finish::UFlowNode_Finish(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	OutputNames = {};
}

void UFlowNode_Finish::ExecuteInput(const FName& PinName)
{
	// this will call FinishFlow()
	Finish();
}
