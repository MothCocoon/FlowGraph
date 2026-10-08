// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "FlowNode_CustomEventBase.h"
#include "FlowNode_CustomInput.generated.h"

/**
 * Defines a named input entry point for this sub-graph. When the parent SubGraph node receives
 * a signal on the matching named pin, execution resumes here. Useful for externally triggering
 * specific behaviors within a sub-graph (e.g. a "Cancel" signal from the parent encounter flow).
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Custom Input"))
class FLOW_API UFlowNode_CustomInput : public UFlowNode_CustomEventBase
{
	GENERATED_BODY()

public:
	UFlowNode_CustomInput();

	virtual void ExecuteInput(const FName& PinName) override;

public:
	virtual void PostEditImport() override;

#if WITH_EDITOR
	virtual FText K2_GetNodeTitle_Implementation() const override;
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
