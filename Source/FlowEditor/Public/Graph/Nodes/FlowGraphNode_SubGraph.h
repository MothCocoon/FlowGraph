// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowGraphNode_SubGraph.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_SubGraph : public UFlowGraphNode
{
	GENERATED_BODY()

public:
	UFlowGraphNode_SubGraph();

	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	// --

	// UFlowGraphNode
	virtual void OnNodeDoubleClickedInPIE() const override;
	// --
};
