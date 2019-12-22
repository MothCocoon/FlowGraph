#include "FlowGraphNode_Out.h"
#include "GraphEditorSettings.h"

#define LOCTEXT_NAMESPACE "FlowGraphNode_Out"

UFlowGraphNode_Out::UFlowGraphNode_Out(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UFlowGraphNode_Out::GetTooltipText() const
{
	return LOCTEXT("OutToolTip", "Output of flow graph");
}

#undef LOCTEXT_NAMESPACE