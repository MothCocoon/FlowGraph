// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Developer/FlowNode_Log.h"
#include "FlowLogChannels.h"

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
	FLOW_ASSERT_ENUM_MAX(EFlowDataPinResolveResult, 8);
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

	if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, NamedProperties))
	{
		for (FFlowNamedDataPinProperty& NamedProperty : NamedProperties)
		{
			const UScriptStruct* ScriptStruct = NamedProperty.DataPinValue.GetScriptStruct();
			if (IsValid(ScriptStruct) && ScriptStruct->IsChildOf<FFlowDataPinValue>())
			{
				FFlowDataPinValue& Value = NamedProperty.DataPinValue.GetMutable<FFlowDataPinValue>();
				Value.bIsInputPin = true;
			}
		}
	}

	Super::PostEditChangeChainProperty(PropertyChainEvent);
}

void UFlowNode_Log::UpdateNodeConfigText_Implementation()
{
	constexpr bool bErrorIfInputPinNotFound = false;
	const bool bIsInputConnected = IsInputConnected(GET_MEMBER_NAME_CHECKED(ThisClass, Message), bErrorIfInputPinNotFound);

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