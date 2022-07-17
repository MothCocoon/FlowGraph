// Copyright Acinex Games 2020


#include "FlowEditor/Public/Graph/Nodes/FlowGraphNode_PropertyGetter.h"

#include "FlowEditor/Public/Graph/Widgets/SFlowGraphNode_PropertyGetter.h"
#include "Nodes/Utils/FlowNode_PropertyGetter.h"


UFlowGraphNode_PropertyGetter::UFlowGraphNode_PropertyGetter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_PropertyGetter::StaticClass()};
}

TSharedPtr<SGraphNode> UFlowGraphNode_PropertyGetter::CreateVisualWidget()
{
	return SNew(SFlowGraphNode_PropertyGetter, this);
}
