#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_ExecutionRandom.generated.h"

/**
 * Executes a random output.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Random"))
class FLOW_API UFlowNode_ExecutionRandom final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	virtual bool CanUserAddOutput() const override { return true; }
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
