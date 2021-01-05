#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_CustomEvent.generated.h"

/**
 * Triggers output upon activation of Input (matching this EventName) on the SubGraph node containing this graph
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Custom Event"))
class FLOW_API UFlowNode_CustomEvent : public UFlowNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FName EventName;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};
