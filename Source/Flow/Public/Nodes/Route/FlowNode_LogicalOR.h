// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_LogicalOR.generated.h"

/**
 * Logical OR
 * Output will be triggered only once
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "OR", Keywords = "|"))
class FLOW_API UFlowNode_LogicalOR final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "Lifetime", SaveGame)
	bool bEnabled;
	
	// This node will become Blocked (not executed any more), if Execution Limit > 0 and Execution Count reaches this limit
	// Set this to zero, if you'd like fire output indefinitely
	UPROPERTY(EditAnywhere, Category = "Lifetime", meta = (ClampMin = 0))
	int32 ExecutionLimit;

	// This node will become Blocked (not executed any more), if Execution Limit > 0 and Execution Count reaches this limit
	UPROPERTY(VisibleAnywhere, Category = "Lifetime", SaveGame)
	int32 ExecutionCount;

#if WITH_EDITOR
public:
	virtual bool CanUserAddInput() const override { return true; }
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

	void ResetCounter();
};
