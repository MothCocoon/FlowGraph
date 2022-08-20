// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_LogicalOR.generated.h"

/**
 * Logical OR
 * Output will be triggered only once
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "OR"))
class FLOW_API UFlowNode_LogicalOR final : public UFlowNode
{
	GENERATED_UCLASS_BODY()
#if WITH_EDITOR
	virtual bool CanUserAddInput() const override { return true; }

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	

protected:
	/** If set to true, this node will only execute once until reset */
	UPROPERTY(EditAnywhere)
	bool bOnce = false;

	UPROPERTY(SaveGame)
	bool bCompleted = false;

	virtual void ExecuteInput(const FName& PinName) override;
};
