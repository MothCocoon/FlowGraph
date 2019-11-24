#include "FlowGraphNode_Out.h"
#include "GraphEditorSettings.h"

#define LOCTEXT_NAMESPACE "FlowGraphNode_Out"

UFlowGraphNode_Out::UFlowGraphNode_Out(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FLinearColor UFlowGraphNode_Out::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->ResultNodeTitleColor;
}

FText UFlowGraphNode_Out::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("OutTitle", "Out");
}

FText UFlowGraphNode_Out::GetTooltipText() const
{
	return LOCTEXT("OutToolTip", "Output of flow graph");
}

#undef LOCTEXT_NAMESPACE
