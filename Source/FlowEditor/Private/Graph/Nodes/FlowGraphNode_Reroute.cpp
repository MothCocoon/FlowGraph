// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode_Reroute.h"
#include "SGraphNodeKnot.h"

#include "Nodes/Route/FlowNode_Reroute.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphNode_Reroute)

UFlowGraphNode_Reroute::UFlowGraphNode_Reroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_Reroute::StaticClass()};
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

bool UFlowGraphNode_Reroute::CanPlaceBreakpoints() const
{
	return false;
}
