// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Graph/FlowNode_FormatText.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_FormatText)

#define LOCTEXT_NAMESPACE "FlowNode_FormatText"

const FName UFlowNode_FormatText::OUTPIN_TextOutput("Formatted Text");

UFlowNode_FormatText::UFlowNode_FormatText(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Graph");
	NodeDisplayStyle = FlowNodeStyle::Terminal;
#endif

	OutputPins.Add(FFlowPin(OUTPIN_TextOutput, EFlowPinType::Text));
}

FFlowDataPinResult_Name UFlowNode_FormatText::TrySupplyDataPinAsName_Implementation(const FName& PinName) const
{
	FText FormattedText;
	const EFlowDataPinResolveResult FormatResult = TryResolveFormatText(PinName, FormattedText);
	if (FormatResult != EFlowDataPinResolveResult::Invalid)
	{
		if (FormatResult == EFlowDataPinResolveResult::Success)
		{
			return FFlowDataPinResult_Name(FName(FormattedText.ToString()));
		}
		else
		{
			return FFlowDataPinResult_Name(FormatResult);
		}
	}

	return Super::TrySupplyDataPinAsName_Implementation(PinName);
}

FFlowDataPinResult_String UFlowNode_FormatText::TrySupplyDataPinAsString_Implementation(const FName& PinName) const
{
	FText FormattedText;
	const EFlowDataPinResolveResult FormatResult = TryResolveFormatText(PinName, FormattedText);
	if (FormatResult != EFlowDataPinResolveResult::Invalid)
	{
		if (FormatResult == EFlowDataPinResolveResult::Success)
		{
			return FFlowDataPinResult_String(FormattedText.ToString());
		}
		else
		{
			return FFlowDataPinResult_String(FormatResult);
		}
	}

	return Super::TrySupplyDataPinAsString_Implementation(PinName);
}

FFlowDataPinResult_Text UFlowNode_FormatText::TrySupplyDataPinAsText_Implementation(const FName& PinName) const
{
	FText FormattedText;
	const EFlowDataPinResolveResult FormatResult = TryResolveFormatText(PinName, FormattedText);
	if (FormatResult != EFlowDataPinResolveResult::Invalid)
	{
		if (FormatResult == EFlowDataPinResolveResult::Success)
		{
			return FFlowDataPinResult_Text(FormattedText);
		}
		else
		{
			return FFlowDataPinResult_Text(FormatResult);
		}
	}

	return Super::TrySupplyDataPinAsText_Implementation(PinName);
}

EFlowDataPinResolveResult UFlowNode_FormatText::TryResolveFormatText(const FName& PinName, FText& OutFormattedText) const
{
	if (PinName == OUTPIN_TextOutput)
	{
		if (TryFormatTextWithNamedPropertiesAsParameters(FormatText, OutFormattedText))
		{
			return EFlowDataPinResolveResult::Success;
		}
		else
		{
			return EFlowDataPinResolveResult::FailedWithError;
		}
	}

	return EFlowDataPinResolveResult::Invalid;
}

#if WITH_EDITOR

void UFlowNode_FormatText::UpdateNodeConfigText_Implementation()
{
	constexpr bool bErrorIfInputPinNotFound = false;
	const bool bIsInputConnected = IsInputConnected(GET_MEMBER_NAME_CHECKED(ThisClass, FormatText), bErrorIfInputPinNotFound);

	if (bIsInputConnected)
	{
		SetNodeConfigText(FText());
	}
	else
	{
		SetNodeConfigText(FormatText);
	}
}

#endif

#undef LOCTEXT_NAMESPACE