#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Checkpoint.generated.h"

/**
 * Save the state of the game
 * It's recommended to replace this with game-specific variant and this node to UFlowGraphSettings::HiddenNodes
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Checkpoint"))
class FLOW_API UFlowNode_Checkpoint : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
