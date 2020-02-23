#include "FlowGraphNode_Out.h"
#include "Graph/Widgets/SFlowGraphNode_Out.h"

UFlowGraphNode_Out::UFlowGraphNode_Out(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedPtr<SGraphNode> UFlowGraphNode_Out::CreateVisualWidget()
{
	return SNew(SFlowGraphNode_Out, this);
}