#include "Graph/Nodes/FlowGraphNode_ExecutionSequence.h"
#include "Nodes/Route/FlowNode_ExecutionSequence.h"

#include "Textures/SlateIcon.h"

UFlowGraphNode_ExecutionSequence::UFlowGraphNode_ExecutionSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_ExecutionSequence::StaticClass()};
}

FSlateIcon UFlowGraphNode_ExecutionSequence::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("FlowEditorStyle", "GraphEditor.Sequence_16x");
	return Icon;
}
