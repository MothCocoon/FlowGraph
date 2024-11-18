// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Branch.h"
#include "AddOns/FlowNodeAddOn_PredicateAND.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Branch)

const FName UFlowNode_Branch::INPIN_Evaluate = TEXT("Evaluate");
const FName UFlowNode_Branch::OUTPIN_True = TEXT("True");
const FName UFlowNode_Branch::OUTPIN_False = TEXT("False");

UFlowNode_Branch::UFlowNode_Branch(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeDisplayStyle = FlowNodeStyle::Logic;
#endif
	InputPins.Empty();
	InputPins.Add(FFlowPin(INPIN_Evaluate));

	OutputPins.Empty();
	OutputPins.Add(FFlowPin(OUTPIN_True));
	OutputPins.Add(FFlowPin(OUTPIN_False));

	AllowedSignalModes = { EFlowSignalMode::Enabled, EFlowSignalMode::Disabled };
}

EFlowAddOnAcceptResult UFlowNode_Branch::AcceptFlowNodeAddOnChild_Implementation(
	const UFlowNodeAddOn* AddOnTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (IFlowPredicateInterface::ImplementsInterfaceSafe(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::TentativeAccept;
	}

	return Super::AcceptFlowNodeAddOnChild_Implementation(AddOnTemplate, AdditionalAddOnsToAssumeAreChildren);
}

void UFlowNode_Branch::ExecuteInput(const FName& PinName)
{
	bool bResult = UFlowNodeAddOn_PredicateAND::EvaluatePredicateAND(AddOns);

	if (bResult)
	{
		TriggerOutput(OUTPIN_True);
	}
	else
	{
		TriggerOutput(OUTPIN_False);
	}
}
