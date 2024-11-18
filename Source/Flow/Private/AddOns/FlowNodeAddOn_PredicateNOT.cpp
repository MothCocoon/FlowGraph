// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn_PredicateNOT.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn_PredicateNOT)

UFlowNodeAddOn_PredicateNOT::UFlowNodeAddOn_PredicateNOT()
	: Super()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::AddOn_Predicate_Composite;
	Category = TEXT("Composite");
#endif
}

EFlowAddOnAcceptResult UFlowNodeAddOn_PredicateNOT::AcceptFlowNodeAddOnChild_Implementation(
	const UFlowNodeAddOn* AddOnTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (AddOns.Num() >= 1 || !AdditionalAddOnsToAssumeAreChildren.IsEmpty())
	{
		// Must not have more than one child Add-On under any circumstances
		return EFlowAddOnAcceptResult::Reject;
	}
	else if (IFlowPredicateInterface::ImplementsInterfaceSafe(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::TentativeAccept;
	}
	else
	{
		return EFlowAddOnAcceptResult::Reject;
	}
}

bool UFlowNodeAddOn_PredicateNOT::EvaluatePredicate_Implementation() const
{
	if (AddOns.IsEmpty())
	{
		// For parity with PredicateAND, the "no AddOns (that qualify)" case results in a "true" result
		return true;
	}

	if (AddOns.Num() > 1)
	{
		const FString Message = FString::Printf(TEXT("%s may only have a single predicate AddOn child"), *GetName());
		UFlowNodeAddOn_PredicateNOT* MutableThis = const_cast<UFlowNodeAddOn_PredicateNOT*>(this);
		MutableThis->LogError(Message);
	}

	UFlowNodeAddOn* SingleChildAddOn = AddOns[0];

	if (!IFlowPredicateInterface::ImplementsInterfaceSafe(SingleChildAddOn))
	{
		const FString Message = FString::Printf(TEXT("%s requires a child AddOn that implements the IFlowPredicateInterface interface!"), *GetName());
		UFlowNodeAddOn_PredicateNOT* MutableThis = const_cast<UFlowNodeAddOn_PredicateNOT*>(this);
		MutableThis->LogError(Message);

		// For parity with PredicateAND, the "no AddOns (that qualify)" case results in a "true" result
		return true;
	}

	const bool bResult = !IFlowPredicateInterface::Execute_EvaluatePredicate(SingleChildAddOn);

	return bResult;
}