// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "Types/FlowBranchEnums.h"
#include "FlowNode_Branch.generated.h"

/**
 * FEvaluates its AddOns that implement the IFlowPredicateInterface to determine the output pin to trigger.
 */
UCLASS(MinimalApi, NotBlueprintable, meta = (DisplayName = "Branch"))
class UFlowNode_Branch : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_Branch();

	/* For root-level predicates on this branch, do we treat them as an "AND" (all must pass) or an "OR" (at least one must pass)? */
	UPROPERTY(EditAnywhere, Category = "Branch", DisplayName = "Root Combination Rule")
	EFlowPredicateCombinationRule BranchCombinationRule = EFlowPredicateCombinationRule::AND;

public:
	// UFlowNodeBase
	virtual EFlowAddOnAcceptResult AcceptFlowNodeAddOnChild_Implementation(const UFlowNodeAddOn* AddOnTemplate, const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const override;
	virtual FText K2_GetNodeTitle_Implementation() const override;
	// --

	/* Event reacting on triggering Input pin. */
	virtual void ExecuteInput(const FName& PinName) override;

	static const FName INPIN_Evaluate;
	static const FName OUTPIN_True;
	static const FName OUTPIN_False;
};
