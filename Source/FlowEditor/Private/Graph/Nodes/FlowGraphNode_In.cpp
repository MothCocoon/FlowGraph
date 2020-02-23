#include "FlowGraphNode_In.h"
#include "Graph/Widgets/SFlowGraphNode_In.h"

UFlowGraphNode_In::UFlowGraphNode_In(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedPtr<SGraphNode> UFlowGraphNode_In::CreateVisualWidget()
{
	return SNew(SFlowGraphNode_In, this);
}