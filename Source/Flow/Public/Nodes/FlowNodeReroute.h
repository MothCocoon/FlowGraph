#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNodeReroute.generated.h"

/**
 * Reroute
 */
UCLASS(meta = (DisplayName = "Reroute"))
class FLOW_API UFlowNodeReroute final : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
