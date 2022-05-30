// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode_Finish.h"
#include "Graph/Widgets/SFlowGraphNode_Finish.h"

#include "Nodes/Route/FlowNode_Finish.h"

UFlowGraphNode_Finish::UFlowGraphNode_Finish(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_Finish::StaticClass()};
}

TSharedPtr<SGraphNode> UFlowGraphNode_Finish::CreateVisualWidget()
{
	return SNew(SFlowGraphNode_Finish, this);
}
