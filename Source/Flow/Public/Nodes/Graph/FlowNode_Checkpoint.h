// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Checkpoint.generated.h"

/**
 * Save the state of the game to the save file.
 * It's recommended to replace this with game-specific variant and this node to UFlowGraphSettings::HiddenNodes.
 */
UCLASS(NotBlueprintable, Config = Game, defaultconfig, meta = (DisplayName = "Checkpoint", Keywords = "autosave, save"))
class FLOW_API UFlowNode_Checkpoint final : public UFlowNode
{
	GENERATED_BODY()
	
public:
	UFlowNode_Checkpoint();

protected:
	/* Change setting by editing DefaultGame.ini, add section.
	 * [/Script/Flow.FlowNode_Checkpoint]
	 * bUseAsyncSave=True */
	UPROPERTY(VisibleAnywhere, Config, Category = "Checkpoint")
	bool bUseAsyncSave;

	virtual void ExecuteInput(const FName& PinName) override;
	virtual void OnLoad_Implementation() override;
};
