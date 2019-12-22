#include "FlowGraphNode_In.h"
#include "GraphEditorSettings.h"

#define LOCTEXT_NAMESPACE "FlowGraphNode_In"

UFlowGraphNode_In::UFlowGraphNode_In(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UFlowGraphNode_In::GetTooltipText() const
{
	return LOCTEXT("InToolTip", "Input of flow graph");
}

#undef LOCTEXT_NAMESPACE