#pragma once

#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowGraphNode_SubGraph.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_SubGraph : public UFlowGraphNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	// --
};
