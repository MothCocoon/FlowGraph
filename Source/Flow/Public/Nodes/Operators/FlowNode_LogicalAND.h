#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_LogicalAND.generated.h"

/**
 * Logical AND
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "AND"))
class FLOW_API UFlowNode_LogicalAND final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

private:
	TSet<FName> ExecutedInputNames;
	
#if WITH_EDITOR
public:
	virtual bool CanUserAddInput() const override { return true; }
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
};
