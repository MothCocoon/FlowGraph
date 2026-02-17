// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode_Branch.h"
#include "Nodes/Route/FlowNode_Branch.h"
#include "Nodes/Route/FlowNode_Switch.h"

#include "Textures/SlateIcon.h"

UFlowGraphNode_Branch::UFlowGraphNode_Branch(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = { UFlowNode_Branch::StaticClass(), UFlowNode_Switch::StaticClass() };
}

FSlateIcon UFlowGraphNode_Branch::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("FlowEditorStyle", "GraphEditor.Branch_16x");
	return Icon;
}
