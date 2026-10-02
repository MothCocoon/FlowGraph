// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Switch.generated.h"

/**
 * Evaluates each output case's predicate AddOns and fires the output(s) whose predicates pass.
 * Unlike Branch (which has only True/False), Switch supports N named cases plus a DefaultCase
 * output that fires when no other case passes. Set bOnlyTriggerFirstPassingCase = false to
 * allow multiple cases to fire in a single evaluation.
 */
UCLASS(MinimalApi, NotBlueprintable, meta = (DisplayName = "Switch", Keywords = "switch branch case predicate"))
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

#if WITH_EDITOR
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
