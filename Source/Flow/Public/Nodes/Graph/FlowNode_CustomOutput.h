// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "FlowNode_CustomEventBase.h"
#include "FlowNode_CustomOutput.generated.h"

/**
 * Fires a named output pin on the parent SubGraph node that contains this sub-graph, allowing
 * this sub-graph to signal back to the parent encounter flow. The triggered output pin name
 * on the SubGraph node matches the EventName set on this node.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Custom Output"))
class FLOW_API UFlowNode_CustomOutput final : public UFlowNode_CustomEventBase
{
	GENERATED_BODY()

public:
	UFlowNode_CustomOutput();

	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
	virtual FText K2_GetNodeTitle_Implementation() const override;
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
