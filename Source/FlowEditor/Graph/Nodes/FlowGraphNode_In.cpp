#include "FlowGraphNode_In.h"
#include "GraphEditorSettings.h"

#define LOCTEXT_NAMESPACE "FlowGraphNode_In"

UFlowGraphNode_In::UFlowGraphNode_In(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FLinearColor UFlowGraphNode_In::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->ResultNodeTitleColor;
}

FText UFlowGraphNode_In::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("InTitle", "In");
}

FText UFlowGraphNode_In::GetTooltipText() const
{
	return LOCTEXT("InToolTip", "Input of flow graph");
}

#undef LOCTEXT_NAMESPACE
