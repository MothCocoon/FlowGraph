// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Switch.generated.h"

/**
 * Similar to a Branch flow node, provides a "Switch" style logic (ie, C/C++),
 * where cases are evaluated and triggered if their predicates pass.
 * By default, only the first passing case is triggered (see bOnlyTriggerFirstPassingCase).
 */
UCLASS(MinimalApi, NotBlueprintable, meta = (DisplayName = "Switch"))
class UFlowNode_Switch : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_Switch();

	/* Only trigger the switch output for the first passing case during a single Evaluate
	 * (if false, all passing cases will trigger) */
	UPROPERTY(EditAnywhere, Category = "Switch")
	bool bOnlyTriggerFirstPassingCase = true;

	// UFlowNodeBase
	virtual EFlowAddOnAcceptResult AcceptFlowNodeAddOnChild_Implementation(const UFlowNodeAddOn* AddOnTemplate, const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const override;
	virtual FText K2_GetNodeTitle_Implementation() const override;
	// --

	/* Event reacting on triggering Input pin. */
	virtual void ExecuteInput(const FName& PinName) override;

	static const FName INPIN_Evaluate;
	static const FName OUTPIN_DefaultCase;
};
