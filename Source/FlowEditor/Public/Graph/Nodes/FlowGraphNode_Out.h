#pragma once

#include "FlowGraphNode.h"
#include "FlowGraphNode_Out.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_Out : public UFlowGraphNode
{
	GENERATED_UCLASS_BODY()

public:
	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	// --
};