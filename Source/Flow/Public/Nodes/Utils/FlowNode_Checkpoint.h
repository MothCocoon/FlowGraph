// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Checkpoint.generated.h"

/**
 * Save the state of the game to the save file
 * It's recommended to replace this with game-specific variant and this node to UFlowGraphSettings::HiddenNodes
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Checkpoint", Keywords = "autosave, save"))
class FLOW_API UFlowNode_Checkpoint final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void OnLoad_Implementation() override;
};
