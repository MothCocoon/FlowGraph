#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNodeLogicalOR.generated.h"

/**
 * Logical OR
 */
UCLASS(meta = (DisplayName = "OR"))
class FLOW_API UFlowNodeLogicalOR final : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
#if WITH_EDITOR
	virtual bool CanUserAddInput() const override { return true; };
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
