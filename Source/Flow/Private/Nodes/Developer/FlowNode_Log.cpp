// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Developer/FlowNode_Log.h"
#include "FlowLogChannels.h"

#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Log)

#define LOCTEXT_NAMESPACE "FlowNode_Log"

UFlowNode_Log::UFlowNode_Log()
	: Message()
	, Verbosity(EFlowLogVerbosity::Warning)
	, bPrintToScreen(true)
	, Duration(5.0f)
	, TextColor(FColor::Yellow)
{
#if WITH_EDITOR
	Category = TEXT("Developer");
	NodeDisplayStyle = FlowNodeStyle::Developer;
#endif

	InputPins = { UFlowNode::DefaultInputPin };
	OutputPins = { UFlowNode::DefaultOutputPin };
}

void UFlowNode_Log::ExecuteInput(const FName& PinName)
{
	// Get the Message from either the default (Message property) or the data pin (if connected)
	FString ResolvedMessage;
	const EFlowDataPinResolveResult MessageResult = TryResolveDataPinValue<FFlowPinType_String>(GET_MEMBER_NAME_CHECKED(ThisClass, Message), ResolvedMessage);

	// #FlowDataPinLegacy - retire this backward compatibility when we remove legacy data pin support?  
	FLOW_ASSERT_ENUM_MAX(EFlowDataPinResolveResult, 9);
	if (MessageResult == EFlowDataPinResolveResult::FailedUnknownPin)
	{
		// Handle lookup of a FlowNode_Log that predated DataPins
		ResolvedMessage = Message;
	}
	// --

	// Format Message with named properties
	FText FormattedMessage = FText::FromString(ResolvedMessage);
	(void) TryFormatTextWithNamedPropertiesAsParameters(FormattedMessage, FormattedMessage);

	// Display the message

	switch (Verbosity)
	{
		case EFlowLogVerbosity::Error:
			UE_LOG(LogFlow, Error, TEXT("%s"), *FormattedMessage.ToString());
			break;
		case EFlowLogVerbosity::Warning:
			UE_LOG(LogFlow, Warning, TEXT("%s"), *FormattedMessage.ToString());
			break;
		case EFlowLogVerbosity::Display:
			UE_LOG(LogFlow, Display, TEXT("%s"), *FormattedMessage.ToString());
			break;
		case EFlowLogVerbosity::Log:
			UE_LOG(LogFlow, Log, TEXT("%s"), *FormattedMessage.ToString());
			break;
		case EFlowLogVerbosity::Verbose:
			UE_LOG(LogFlow, Verbose, TEXT("%s"), *FormattedMessage.ToString());
			break;
		case EFlowLogVerbosity::VeryVerbose:
			UE_LOG(LogFlow, VeryVerbose, TEXT("%s"), *FormattedMessage.ToString());
			break;
		default: ;
	}

	if (bPrintToScreen)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, TextColor, FormattedMessage.ToString());
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
void UFlowNode_Log::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChainEvent)
{
	const auto& Property = PropertyChainEvent.PropertyChain.GetActiveMemberNode()->GetValue();
	constexpr bool bIsInput = true;
	OnPostEditEnsureAllNamedPropertiesPinDirection(*Property, bIsInput);

	Super::PostEditChangeChainProperty(PropertyChainEvent);
}

void UFlowNode_Log::OnEditorPinConnectionsChanged(const TArray<FFlowPinConnectionChange>& Changes)
{
	Super::OnEditorPinConnectionsChanged(Changes);

	UpdateNodeConfigText();
}

void UFlowNode_Log::UpdateNodeConfigText_Implementation()
{
	constexpr bool bErrorIfInputPinNotFound = true;

	FConnectedPin ConnectedPin;
	const bool bIsInputConnected = FindFirstInputPinConnection(GET_MEMBER_NAME_CHECKED(ThisClass, Message), bErrorIfInputPinNotFound, ConnectedPin);

	if (bIsInputConnected)
	{
		SetNodeConfigText(FText::Format(LOCTEXT("LogFromPin", "Message from: {0}"), { FText::FromString(ConnectedPin.PinName.ToString()) }));
	}
	else
	{
		SetNodeConfigText(FText::FromString(Message));
	}
}

#endif

#undef LOCTEXT_NAMESPACE