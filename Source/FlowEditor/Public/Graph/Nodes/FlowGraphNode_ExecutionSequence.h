// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowGraphNode_ExecutionSequence.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_ExecutionSequence : public UFlowGraphNode
{
	GENERATED_BODY()

public:
	UFlowGraphNode_ExecutionSequence();

	// UEdGraphNode
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	// --
};
