#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Counter.generated.h"

/**
 * Counts how many times signal entered this node
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Counter"))
class FLOW_API UFlowNode_Counter final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Counter", meta = (ClampMin = 2))
	int32 Goal;

private:
	int32 CurrentSum;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
	virtual FString GetStatusString() const override;
#endif
};
