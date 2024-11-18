// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn_PredicateAND.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn_PredicateAND)

UFlowNodeAddOn_PredicateAND::UFlowNodeAddOn_PredicateAND()
	: Super()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::AddOn_Predicate_Composite;
	Category = TEXT("Composite");
#endif
}

EFlowAddOnAcceptResult UFlowNodeAddOn_PredicateAND::AcceptFlowNodeAddOnChild_Implementation(
	const UFlowNodeAddOn* AddOnTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (IFlowPredicateInterface::ImplementsInterfaceSafe(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::TentativeAccept;
	}
	else
	{
		// All AddOn children MUST implement IFlowPredicateInterface
		// (so do not return Super's implementation which will return Undetermined)
		return EFlowAddOnAcceptResult::Reject;
	}
}

bool UFlowNodeAddOn_PredicateAND::EvaluatePredicate_Implementation() const
{
	return EvaluatePredicateAND(AddOns);
}

bool UFlowNodeAddOn_PredicateAND::EvaluatePredicateAND(const TArray<UFlowNodeAddOn*>& AddOns)
{
	for (int Index = 0; Index < AddOns.Num(); ++Index)
	{
		const UFlowNodeAddOn* AddOn = AddOns[Index];

		if (IFlowPredicateInterface::ImplementsInterfaceSafe(AddOn))
		{
			const bool bResult = IFlowPredicateInterface::Execute_EvaluatePredicate(AddOn);

			if (!bResult)
			{
				return false;
			}
		}
	}

	return true;
}
