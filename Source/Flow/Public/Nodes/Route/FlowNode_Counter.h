// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Counter.generated.h"

/**
 * Counts the number of times its input pin is triggered and fires its output when the count reaches Goal.
 * Useful for gating flow until a fixed number of events have occurred (e.g. "after 3 enemies die").
 * Resets automatically via Cleanup.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Counter"))
class FLOW_API UFlowNode_Counter final : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_Counter();

protected:
	/* Number of times the input pin must be triggered before the output pin fires. */
	UPROPERTY(EditAnywhere, Category = "Counter", meta = (ClampMin = 2))
	int32 Goal = 2;

	UPROPERTY(SaveGame)
	int32 CurrentSum = 0;

public:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
	virtual FString GetStatusString() const override;
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
