// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowGraphNode.h"
#include "FlowGraphNode_PropertyGetter.generated.h"

/**
 * 
 */
UCLASS()
class FLOWEDITOR_API UFlowGraphNode_PropertyGetter : public UFlowGraphNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	// --
};
