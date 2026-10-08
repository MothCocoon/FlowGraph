// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowPredicateInterface.h"

#include "FlowNodeAddOn_PredicateNOT.generated.h"

class UFlowNode;

/**
 * Inverts the combined result of its attached child predicate AddOns, evaluating true only when they would otherwise
 * evaluate false.
 */
UCLASS(MinimalApi, NotBlueprintable, meta = (DisplayName = "NOT"))
class UFlowNodeAddOn_PredicateNOT
	: public UFlowNodeAddOn
	, public IFlowPredicateInterface
{
	GENERATED_BODY()

public:
	UFlowNodeAddOn_PredicateNOT();

	// UFlowNodeBase
	virtual EFlowAddOnAcceptResult AcceptFlowNodeAddOnChild_Implementation(const UFlowNodeAddOn* AddOnTemplate, const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const override;
	// --

	// IFlowPredicateInterface
	virtual bool EvaluatePredicate_Implementation() const override;
	// --

#if WITH_EDITOR
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
