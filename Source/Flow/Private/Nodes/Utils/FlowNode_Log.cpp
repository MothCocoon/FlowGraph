// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Utils/FlowNode_Log.h"
#include "FlowLogChannels.h"
#include "FlowSettings.h"

#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Log)

#define LOCTEXT_NAMESPACE "FlowNode_Log"

UFlowNode_Log::UFlowNode_Log(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Message()
	, Verbosity(EFlowLogVerbosity::Warning)
	, bPrintToScreen(true)
	, Duration(5.0f)
	, TextColor(FColor::Yellow)
{
#if WITH_EDITOR
	Category = TEXT("Utils");
	NodeDisplayStyle = FlowNodeStyle::Developer;
#endif
}

void UFlowNode_Log::ExecuteInput(const FName& PinName)
{
	// Get the Message from either the default (Message property) or the data pin (if connected)
	FFlowDataPinResult_String MessageResult = TryResolveDataPinAsString(GET_MEMBER_NAME_CHECKED(UFlowNode_Log, Message));

	if (MessageResult.Result == EFlowDataPinResolveResult::FailedMissingPin)
	{
		// Handle lookup of a FlowNode_Log that predated DataPins
		MessageResult.Result = EFlowDataPinResolveResult::Success;
		MessageResult.SetValue(Message);
	}

	check(MessageResult.Result == EFlowDataPinResolveResult::Success);

	switch (Verbosity)
	{
		case EFlowLogVerbosity::Error:
			UE_LOG(LogFlow, Error, TEXT("%s"), *MessageResult.Value);
			break;
		case EFlowLogVerbosity::Warning:
			UE_LOG(LogFlow, Warning, TEXT("%s"), *MessageResult.Value);
			break;
		case EFlowLogVerbosity::Display:
			UE_LOG(LogFlow, Display, TEXT("%s"), *MessageResult.Value);
			break;
		case EFlowLogVerbosity::Log:
			UE_LOG(LogFlow, Log, TEXT("%s"), *MessageResult.Value);
			break;
		case EFlowLogVerbosity::Verbose:
			UE_LOG(LogFlow, Verbose, TEXT("%s"), *MessageResult.Value);
			break;
		case EFlowLogVerbosity::VeryVerbose:
			UE_LOG(LogFlow, VeryVerbose, TEXT("%s"), *MessageResult.Value);
			break;
		default: ;
	}

	if (bPrintToScreen)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, TextColor, MessageResult.Value);
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR

void UFlowNode_Log::UpdateNodeConfigText_Implementation()
{
	constexpr bool bErrorIfInputPinNotFound = false;
	const bool bIsInputConnected = IsInputConnected(GET_MEMBER_NAME_CHECKED(UFlowNode_Log, Message), bErrorIfInputPinNotFound);

	if (bIsInputConnected)
	{
		SetNodeConfigText(FText());
	}
	else
	{
		SetNodeConfigText(FText::FromString(Message));
	}
}

#endif

#undef LOCTEXT_NAMESPACE