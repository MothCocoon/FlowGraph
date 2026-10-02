// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_LogicalAND.generated.h"

/**
 * Fires its output only after ALL of its input pins have each been triggered at least once.
 * Output fires exactly once per activation cycle.
 * Add input pins via the node context menu.
 * Use to synchronize multiple parallel branches before continuing.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "AND", Keywords = "&"))
class FLOW_API UFlowNode_LogicalAND final : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_LogicalAND();

private:
	UPROPERTY(SaveGame)
	TSet<FName> ExecutedInputNames;

public:
#if WITH_EDITOR
	virtual bool CanUserAddInput() const override { return true; }

	virtual FString GetStatusString() const override;

#endif

	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

#if WITH_EDITOR
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
