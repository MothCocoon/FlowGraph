// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_LogicalOR.generated.h"

/**
 * Fires its output whenever ANY of its input pins is triggered, subject to ExecutionLimit.
 * Use to merge multiple parallel branches into a single continuation point.
 * Set ExecutionLimit to 0 to allow unlimited firings; set to N to block after N total firings.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "OR", Keywords = "|"))
class FLOW_API UFlowNode_LogicalOR final : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_LogicalOR();

protected:
	UPROPERTY(EditAnywhere, Category = "Lifetime", SaveGame)
	bool bEnabled = true;

	/* This node will become Blocked (not executed anymore), if Execution Limit > 0 and Execution Count reaches this limit.
	 * Set this to zero, if you'd like fire output indefinitely. */
	UPROPERTY(EditAnywhere, Category = "Lifetime", meta = (ClampMin = 0))
	int32 ExecutionLimit = 1;

	/* This node will become Blocked (not executed anymore), if Execution Limit > 0 and Execution Count reaches this limit. */
	UPROPERTY(VisibleAnywhere, Category = "Lifetime", SaveGame)
	int32 ExecutionCount = 0;

public:
#if WITH_EDITOR
	virtual bool CanUserAddInput() const override { return true; }
#endif

	virtual void ExecuteInput(const FName& PinName) override;

protected:
	void ResetCounter();

#if WITH_EDITOR
public:
	virtual FString GetStatusString() const override;
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
