// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode_Start.h"
#include "Graph/Widgets/SFlowGraphNode_Start.h"

#include "Nodes/Graph/FlowNode_Start.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphNode_Start)

UFlowGraphNode_Start::UFlowGraphNode_Start(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_Start::StaticClass()};
}

TSharedPtr<SGraphNode> UFlowGraphNode_Start::CreateVisualWidget()
{
	return SNew(SFlowGraphNode_Start, this);
}
