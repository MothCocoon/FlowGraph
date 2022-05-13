// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Reroute.generated.h"

/**
 * Reroute
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Reroute"))
class FLOW_API UFlowNode_Reroute final : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
