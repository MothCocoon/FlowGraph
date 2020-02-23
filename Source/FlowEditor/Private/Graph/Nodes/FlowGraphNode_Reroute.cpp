#include "FlowGraphNode_Reroute.h"
#include "SGraphNodeKnot.h"

UFlowGraphNode_Reroute::UFlowGraphNode_Reroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedPtr<SGraphNode> UFlowGraphNode_Reroute::CreateVisualWidget()
{
	return SNew(SGraphNodeKnot, this);
}

bool UFlowGraphNode_Reroute::ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const
{
	OutInputPinIndex = 0;
	OutOutputPinIndex = 1;
	return true;
}