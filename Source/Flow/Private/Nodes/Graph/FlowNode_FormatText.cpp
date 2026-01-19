// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Graph/FlowNode_FormatText.h"
#include "Types/FlowPinTypesStandard.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_FormatText)

#define LOCTEXT_NAMESPACE "FlowNode_FormatText"

UFlowNode_FormatText::UFlowNode_FormatText(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Graph");
	NodeDisplayStyle = FlowNodeStyle::Terminal;
#endif

	OutputPins.Add(FFlowPin(TEXT("Formatted Text"), FFlowPinType_Text::GetPinTypeNameStatic()));
}

FFlowDataPinResult UFlowNode_FormatText::TrySupplyDataPin_Implementation(FName PinName) const
{
	if (PinName == TEXT("Formatted Text"))
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

	return Super::TrySupplyDataPin_Implementation(PinName);
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

void UFlowNode_FormatText::UpdateNodeConfigText_Implementation()
{
	constexpr bool bErrorIfInputPinNotFound = false;
	if (IsInputConnected(GET_MEMBER_NAME_CHECKED(ThisClass, FormatText), bErrorIfInputPinNotFound))
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