// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_ExecutionSequence.generated.h"

/**
 * Executes all outputs sequentially
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Sequence"))
class FLOW_API UFlowNode_ExecutionSequence final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	virtual bool CanUserAddOutput() const override { return true; }
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
