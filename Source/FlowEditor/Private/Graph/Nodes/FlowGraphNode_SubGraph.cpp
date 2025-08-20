// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode_SubGraph.h"
#include "Graph/Widgets/SFlowGraphNode_SubGraph.h"

#include "FlowAsset.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphNode_SubGraph)

UFlowGraphNode_SubGraph::UFlowGraphNode_SubGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_SubGraph::StaticClass()};
}

TSharedPtr<SGraphNode> UFlowGraphNode_SubGraph::CreateVisualWidget()
{
	return SNew(SFlowGraphNode_SubGraph, this);
}

void UFlowGraphNode_SubGraph::OnNodeDoubleClickedInPIE() const
{
	UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(GetFlowNodeBase());
	ensureAlways(SubGraphNode);

	const TWeakObjectPtr<UFlowAsset> SubFlowInstance = GetFlowAsset()->GetFlowInstance(SubGraphNode);
	if (SubFlowInstance.IsValid())
	{
		SubGraphNode->GetFlowAsset()->GetTemplateAsset()->SetInspectedInstance(SubFlowInstance->GetDisplayName());
	}
}
