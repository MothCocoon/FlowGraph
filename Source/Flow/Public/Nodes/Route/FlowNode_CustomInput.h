// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowNode_CustomNodeBase.h"

#include "FlowNode_CustomInput.generated.h"

/**
 * Triggers output upon activation of Input (matching this EventName) on the SubGraph node containing this graph
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Custom Input"))
class FLOW_API UFlowNode_CustomInput : public UFlowNode_CustomNodeBase
{
	GENERATED_UCLASS_BODY()

	friend class UFlowAsset;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FText GetNodeTitle() const override;
#endif
};
