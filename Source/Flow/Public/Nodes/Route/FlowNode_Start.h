// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Start.generated.h"

/**
 * Execution of the graph always starts from this node
 */
UCLASS(NotBlueprintable, NotPlaceable, meta = (DisplayName = "Start"))
class FLOW_API UFlowNode_Start : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
