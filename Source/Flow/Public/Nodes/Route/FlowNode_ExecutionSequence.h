#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_ExecutionSequence.generated.h"

/**
 * Execution Sequence
 */
UCLASS(meta = (DisplayName = "Sequence"))
class FLOW_API UFlowNode_ExecutionSequence final : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
#if WITH_EDITOR
	virtual bool CanUserAddOutput() const override { return true; };
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
