// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"

#include "FlowNode_Branch.generated.h"

// FEvaluates its AddOns that implement the IFlowPredicateInterface to determine the output pin to trigger
UCLASS(MinimalApi, NotBlueprintable, meta = (DisplayName = "Branch"))
class UFlowNode_Branch : public UFlowNode
{
	GENERATED_UCLASS_BODY()

public:

	// UFlowNodeBase
	virtual EFlowAddOnAcceptResult AcceptFlowNodeAddOnChild_Implementation(const UFlowNodeAddOn* AddOnTemplate, const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const override;
	// --

	// Event reacting on triggering Input pin
	virtual void ExecuteInput(const FName& PinName) override;

	static const FName INPIN_Evaluate;
	static const FName OUTPIN_True;
	static const FName OUTPIN_False;
};
