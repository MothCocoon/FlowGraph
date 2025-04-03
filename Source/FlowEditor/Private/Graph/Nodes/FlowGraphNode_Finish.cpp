// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode_Finish.h"
#include "Graph/Widgets/SFlowGraphNode_Finish.h"

#include "Nodes/Graph/FlowNode_Finish.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphNode_Finish)

UFlowGraphNode_Finish::UFlowGraphNode_Finish(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_Finish::StaticClass()};
}

TSharedPtr<SGraphNode> UFlowGraphNode_Finish::CreateVisualWidget()
{
	return SNew(SFlowGraphNode_Finish, this);
}
