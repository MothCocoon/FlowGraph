#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNodeLogicalAND.generated.h"

/**
 * Logical AND
 */
UCLASS(meta = (DisplayName = "AND"))
class FLOW_API UFlowNodeLogicalAND final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

private:
	TSet<FName> ExecutedInputNames;
	
#if WITH_EDITOR
public:
	virtual bool CanUserAddInput() const override { return true; };
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
};
