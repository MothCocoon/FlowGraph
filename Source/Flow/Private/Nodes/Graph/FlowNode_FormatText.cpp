// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Graph/FlowNode_FormatText.h"
#include "Types/FlowPinTypesStandard.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_FormatText)

#define LOCTEXT_NAMESPACE "FlowNode_FormatText"

const FName UFlowNode_FormatText::OUTPIN_TextOutput("Formatted Text");

UFlowNode_FormatText::UFlowNode_FormatText()
{
#if WITH_EDITOR
	Category = TEXT("Graph");
	NodeDisplayStyle = FlowNodeStyle::Terminal;
#endif

	OutputPins.Add(FFlowPin(OUTPIN_TextOutput, FFlowPinType_Text::GetPinTypeNameStatic()));
}

FFlowDataPinResult UFlowNode_FormatText::TrySupplyDataPin(FName PinName) const
{
	if (PinName == OUTPIN_TextOutput)
	{
		FText FormattedText;
		const EFlowDataPinResolveResult FormatResult = TryResolveFormatText(PinName, FormattedText);
	
		if (FlowPinType::IsSuccess(FormatResult))
		{
			return FFlowDataPinResult(FFlowDataPinValue_Text(FormattedText));
		}
		else
		{
			return FFlowDataPinResult(FormatResult);
		}
	}

	return Super::TrySupplyDataPin(PinName);
}

EFlowDataPinResolveResult UFlowNode_FormatText::TryResolveFormatText(const FName& PinName, FText& OutFormattedText) const
{
	if (TryFormatTextWithNamedPropertiesAsParameters(FormatText, OutFormattedText))
	{
		return EFlowDataPinResolveResult::Success;
	}
	else
	{
		LogError(FString::Printf(TEXT("Could not format text '%s' with properties as parameters"), *FormatText.ToString()), EFlowOnScreenMessageType::Temporary);

		return EFlowDataPinResolveResult::FailedWithError;
	}
}

#if WITH_EDITOR
void UFlowNode_FormatText::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChainEvent)
{
	const auto& Property = PropertyChainEvent.PropertyChain.GetActiveMemberNode()->GetValue();
	constexpr bool bIsInput = true;
	OnPostEditEnsureAllNamedPropertiesPinDirection(*Property, bIsInput);

	Super::PostEditChangeChainProperty(PropertyChainEvent);
}

void UFlowNode_FormatText::UpdateNodeConfigText_Implementation()
{
	constexpr bool bErrorIfInputPinNotFound = true;
	FConnectedPin ConnectedPin;
	const bool bIsInputConnected = FindFirstInputPinConnection(GET_MEMBER_NAME_CHECKED(ThisClass, FormatText), bErrorIfInputPinNotFound, ConnectedPin);

	if (bIsInputConnected)
	{
		SetNodeConfigText(FText::Format(LOCTEXT("FormatTextFromPin", "Format from: {0}"), { FText::FromString(ConnectedPin.PinName.ToString()) }));
	}
	else
	{
		SetNodeConfigText(FormatText);
	}
}

#endif

#undef LOCTEXT_NAMESPACE