// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowNode_CustomEventBase.h"
#include "FlowNode_CustomInput.generated.h"

/**
 * Triggers output upon activation of Input (matching this EventName) on the SubGraph node containing this graph
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Custom Input"))
class FLOW_API UFlowNode_CustomInput : public UFlowNode_CustomEventBase
{
	GENERATED_UCLASS_BODY()

	friend class UFlowAsset;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

public:
	virtual void PostEditImport() override;

#if WITH_EDITOR
public:
	virtual FText GetNodeTitle() const override;
#endif
};
