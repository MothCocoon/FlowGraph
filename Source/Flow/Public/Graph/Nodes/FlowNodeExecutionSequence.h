#pragma once

#include "FlowNode.h"
#include "FlowNodeExecutionSequence.generated.h"

/**
 * Execution Sequence
 */
UCLASS(meta = (DisplayName = "Sequence"))
class FLOW_API UFlowNodeExecutionSequence final : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual bool CanUserAddOutput() const override { return true; };
	
protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
