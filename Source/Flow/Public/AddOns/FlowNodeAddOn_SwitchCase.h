// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowSwitchCaseInterface.h"
#include "Types/FlowBranchEnums.h"
#include "FlowNodeAddOn_SwitchCase.generated.h"

class UFlowNode;

UCLASS(MinimalApi, Blueprintable, meta = (DisplayName = "Case"))
class UFlowNodeAddOn_SwitchCase
	: public UFlowNodeAddOn
	, public IFlowSwitchCaseInterface
{
	GENERATED_BODY()

public:

	/* The output pin for this Switch Case, if it passes. */
	UPROPERTY(EditAnywhere, Category = "Switch")
	FName CaseName;

	/* The output pin for this Switch Case, if it passes. */
	UPROPERTY()
	mutable FName OutputPinName;

	/* For root-level predicates on this Switch Case, do we treat them as an "AND" (all must pass) or an "OR" (at least one must pass)? */
	UPROPERTY(EditAnywhere, Category = "Switch", DisplayName = "Root Combination Rule")
	EFlowPredicateCombinationRule BranchCombinationRule = EFlowPredicateCombinationRule::AND;

	/* The base PinName for the Switch Case output(s). */
	static const FName DefaultCaseName;

public:
	UFlowNodeAddOn_SwitchCase();

	// UFlowNodeBase
	virtual EFlowAddOnAcceptResult AcceptFlowNodeAddOnChild_Implementation(const UFlowNodeAddOn* AddOnTemplate, const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const override;
	virtual FText K2_GetNodeTitle_Implementation() const override;
	// --

#if WITH_EDITOR
	// UObject
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// --

	// IFlowContextPinSupplierInterface
	virtual bool SupportsContextPins() const override { return true; }
	virtual TArray<FFlowPin> GetContextOutputs() const override;
	// --
#endif

	// IFlowSwitchCaseInterface
	virtual bool TryTriggerForCase_Implementation() const override;
	// --
};
