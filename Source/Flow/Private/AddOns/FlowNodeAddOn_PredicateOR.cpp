// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn_PredicateOR.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn_PredicateOR)

UFlowNodeAddOn_PredicateOR::UFlowNodeAddOn_PredicateOR()
	: Super()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::AddOn_Predicate_Composite;
	Category = TEXT("Composite");
#endif
}

EFlowAddOnAcceptResult UFlowNodeAddOn_PredicateOR::AcceptFlowNodeAddOnChild_Implementation(
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

bool UFlowNodeAddOn_PredicateOR::EvaluatePredicate_Implementation() const
{
	return EvaluatePredicateOR(AddOns);
}

bool UFlowNodeAddOn_PredicateOR::EvaluatePredicateOR(const TArray<UFlowNodeAddOn*>& AddOns)
{
	int32 FalseCount = 0;
	for (int Index = 0; Index < AddOns.Num(); ++Index)
	{
		const UFlowNodeAddOn* AddOn = AddOns[Index];

		if (IFlowPredicateInterface::ImplementsInterfaceSafe(AddOn))
		{
			const bool bResult = IFlowPredicateInterface::Execute_EvaluatePredicate(AddOn);

			if (bResult)
			{
				return true;
			}
			else
			{
				++FalseCount;
			}
		}
	}

	if (FalseCount == 0)
	{
		// For parity with PredicateAND, the "no AddOns (that qualify)" case results in a "true" result
		return true;
	}
	else
	{
		return false;
	}
}
