// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowNode_CustomEventBase.h"
#include "FlowNode_CustomOutput.generated.h"

/**
 * Triggers output on SubGraph node containing this graph
 * Triggered output name matches EventName selected on this node
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Custom Output"))
class FLOW_API UFlowNode_CustomOutput final : public UFlowNode_CustomEventBase
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
#endif
};
