#pragma once

#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowGraphNode_Out.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_Out : public UFlowGraphNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	// --
};
