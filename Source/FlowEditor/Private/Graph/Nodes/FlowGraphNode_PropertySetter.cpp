
#include "Graph/Nodes/FlowGraphNode_PropertySetter.h"

#include "FlowEditor/Public/Graph/Widgets/SFlowGraphNode_PropertySetter.h"
#include "Nodes/Utils/FlowNode_PropertySetter.h"


UFlowGraphNode_PropertySetter::UFlowGraphNode_PropertySetter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_PropertySetter::StaticClass()};
}

TSharedPtr<SGraphNode> UFlowGraphNode_PropertySetter::CreateVisualWidget()
{
	return SNew(SFlowGraphNode_PropertySetter, this);
}
