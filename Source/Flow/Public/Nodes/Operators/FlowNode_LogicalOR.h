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
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
