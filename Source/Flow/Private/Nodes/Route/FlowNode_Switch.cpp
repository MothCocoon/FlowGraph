// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Switch.h"
#include "AddOns/FlowNodeAddOn.h"
#include "FlowSettings.h"
#include "Interfaces/FlowSwitchCaseInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Switch)

#define LOCTEXT_NAMESPACE "FlowNode_Switch"

const FName UFlowNode_Switch::INPIN_Evaluate = TEXT("Evaluate");
const FName UFlowNode_Switch::OUTPIN_DefaultCase = TEXT("None Passed");

UFlowNode_Switch::UFlowNode_Switch()
{
#if WITH_EDITOR
	Category = TEXT("Route|Logic");
	NodeDisplayStyle = FlowNodeStyle::Logic;
#endif

	InputPins.Reset();
	InputPins.Add(FFlowPin(INPIN_Evaluate));

	OutputPins.Reset();
	OutputPins.Add(FFlowPin(OUTPIN_DefaultCase.ToString(), FString(TEXT("Triggered when no cases pass (during a Switch Evaluate)"))));

	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

EFlowAddOnAcceptResult UFlowNode_Switch::AcceptFlowNodeAddOnChild_Implementation(const UFlowNodeAddOn* AddOnTemplate, const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (IFlowSwitchCaseInterface::ImplementsInterfaceSafe(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::TentativeAccept;
	}

	return Super::AcceptFlowNodeAddOnChild_Implementation(AddOnTemplate, AdditionalAddOnsToAssumeAreChildren);
}

void UFlowNode_Switch::ExecuteInput(const FName& PinName)
{
	int32 TriggeringCaseCount = 0;

	// Trigger the IFlowSwitchCaseInterface addons that pass
	const EFlowForEachAddOnFunctionReturnValue SwitchCaseResult = 
		ForEachAddOnForClassConst<UFlowSwitchCaseInterface>(
			[&TriggeringCaseCount, this](const UFlowNodeAddOn& SwitchCaseAddOn)
			{
				const IFlowSwitchCaseInterface* SwitchCaseInterface = CastChecked<IFlowSwitchCaseInterface>(&SwitchCaseAddOn);

				if (IFlowSwitchCaseInterface::Execute_TryTriggerForCase(&SwitchCaseAddOn))
				{
					++TriggeringCaseCount;

					if (bOnlyTriggerFirstPassingCase)
					{
						return EFlowForEachAddOnFunctionReturnValue::BreakWithSuccess;
					}
				}

				return EFlowForEachAddOnFunctionReturnValue::Continue;
			});

	if (TriggeringCaseCount == 0)
	{
		// Trigger the default case if none of the cases passed
		constexpr bool bFinish = true;
		TriggerOutput(OUTPIN_DefaultCase, bFinish);
	}
}

FText UFlowNode_Switch::K2_GetNodeTitle_Implementation() const
{
	if (!bOnlyTriggerFirstPassingCase && GetDefault<UFlowSettings>()->bUseAdaptiveNodeTitles)
	{
		return FText::Format(LOCTEXT("SwitchTitle", "{0} (All Passing)"), { Super::K2_GetNodeTitle_Implementation() });
	}

	return Super::K2_GetNodeTitle_Implementation();
}

#undef LOCTEXT_NAMESPACE