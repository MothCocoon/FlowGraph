// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowGraphNode_ExecutionSequence.generated.h"

UCLASS()
class UFlowGraphNode_ExecutionSequence final : public UFlowGraphNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	// --
};
