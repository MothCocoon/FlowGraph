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

#if WITH_EDITOR
const FFlowAgentDoc& UFlowNode_Finish::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("Place one at every logical end point of the graph (success, failure, timeout, etc.) - multiple Finish nodes are allowed, and whichever one is triggered first wins."),
		/*Tags*/     { TEXT("graph"), TEXT("finish"), TEXT("output") },
		/*Articles*/ {  });
	return Doc;
}
#endif
