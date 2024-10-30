// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowPredicateInterface.h"

#include "FlowNodeAddOn_PredicateNOT.generated.h"

// Forward Declarations
class UFlowNode;

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
};
