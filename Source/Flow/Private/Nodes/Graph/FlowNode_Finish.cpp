// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Graph/FlowNode_Finish.h"

#include "FlowAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Finish)

UFlowNode_Finish::UFlowNode_Finish()
{
	OutputPins = {};
}

void UFlowNode_Finish::ExecuteInput(const FName& PinName)
{
	CommitOutputDataPinValues();

	// this will call FinishFlow()
	Finish();
}
