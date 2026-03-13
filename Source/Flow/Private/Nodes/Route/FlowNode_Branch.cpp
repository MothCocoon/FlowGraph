// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Branch.h"
#include "AddOns/FlowNodeAddOn_PredicateAND.h"
#include "AddOns/FlowNodeAddOn_PredicateOR.h"
#include "FlowSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Branch)

#define LOCTEXT_NAMESPACE "FlowNode_Branch"

const FName UFlowNode_Branch::INPIN_Evaluate = TEXT("Evaluate");
const FName UFlowNode_Branch::OUTPIN_True = TEXT("True");
const FName UFlowNode_Branch::OUTPIN_False = TEXT("False");

UFlowNode_Branch::UFlowNode_Branch()
{
#if WITH_EDITOR
	Category = TEXT("Route|Logic");
	NodeDisplayStyle = FlowNodeStyle::Logic;
#endif
	InputPins.Empty();
	InputPins.Add(FFlowPin(INPIN_Evaluate));

	OutputPins.Empty();
	OutputPins.Add(FFlowPin(OUTPIN_True));
	OutputPins.Add(FFlowPin(OUTPIN_False));

	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

EFlowAddOnAcceptResult UFlowNode_Branch::AcceptFlowNodeAddOnChild_Implementation(const UFlowNodeAddOn* AddOnTemplate, const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (IFlowPredicateInterface::ImplementsInterfaceSafe(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::TentativeAccept;
	}

	return Super::AcceptFlowNodeAddOnChild_Implementation(AddOnTemplate, AdditionalAddOnsToAssumeAreChildren);
}

void UFlowNode_Branch::ExecuteInput(const FName& PinName)
{
	bool bPassedRootPredicates = false;
	FName ResultPinName = OUTPIN_False;

	// Test the root-level IFlowPredicateInterface addons
	FLOW_ASSERT_ENUM_MAX(EFlowPredicateCombinationRule, 2);
	if (BranchCombinationRule == EFlowPredicateCombinationRule::AND)
	{
		bPassedRootPredicates = UFlowNodeAddOn_PredicateAND::EvaluatePredicateAND(AddOns);
	}
	else
	{
		check(BranchCombinationRule == EFlowPredicateCombinationRule::OR);

		bPassedRootPredicates = UFlowNodeAddOn_PredicateOR::EvaluatePredicateOR(AddOns);
	}

	constexpr bool bFinish = true;
	if (bPassedRootPredicates)
	{
		TriggerOutput(OUTPIN_True, bFinish);
	}
	else
	{
		TriggerOutput(OUTPIN_False, bFinish);
	}
}

FText UFlowNode_Branch::K2_GetNodeTitle_Implementation() const
{
	FLOW_ASSERT_ENUM_MAX(EFlowPredicateCombinationRule, 2);
	if (BranchCombinationRule != EFlowPredicateCombinationRule::AND && 
		GetDefault<UFlowSettings>()->bUseAdaptiveNodeTitles)
	{
		return FText::Format(LOCTEXT("BranchTitle", "{0} ({1})"), { Super::K2_GetNodeTitle_Implementation(), UEnum::GetDisplayValueAsText(BranchCombinationRule) });
	}

	return Super::K2_GetNodeTitle_Implementation();
}

#undef LOCTEXT_NAMESPACE