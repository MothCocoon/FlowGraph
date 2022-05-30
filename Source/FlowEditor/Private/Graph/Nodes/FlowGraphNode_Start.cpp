// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode_Start.h"
#include "Graph/Widgets/SFlowGraphNode_Start.h"

#include "Nodes/Route/FlowNode_Start.h"

UFlowGraphNode_Start::UFlowGraphNode_Start(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_Start::StaticClass()};
}

TSharedPtr<SGraphNode> UFlowGraphNode_Start::CreateVisualWidget()
{
	return SNew(SFlowGraphNode_Start, this);
}
