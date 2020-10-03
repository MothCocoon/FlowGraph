#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Finish.generated.h"

/**
 * Finish
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Finish"))
class FLOW_API UFlowNode_Finish : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
