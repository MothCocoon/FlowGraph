#pragma once

#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowGraphNode_In.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_In : public UFlowGraphNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }
	// --
};
