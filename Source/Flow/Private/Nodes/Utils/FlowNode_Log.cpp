// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Utils/FlowNode_Log.h"
#include "FlowModule.h"

#include "Engine/Engine.h"

UFlowNode_Log::UFlowNode_Log(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Message(TEXT("Log!"))
	, Verbosity(EFlowLogVerbosity::Warning)
	, bPrintToScreen(true)
	, Duration(5.0f)
	, TextColor(FColor::Yellow)
{
#if WITH_EDITOR
	Category = TEXT("Utils");
#endif
}

void UFlowNode_Log::ExecuteInput(const FName& PinName)
{
	switch (Verbosity)
	{
		case EFlowLogVerbosity::Error:
			UE_LOG(LogFlow, Error, TEXT("%s"), *Message);
			break;
		case EFlowLogVerbosity::Warning:
			UE_LOG(LogFlow, Warning, TEXT("%s"), *Message);
			break;
		case EFlowLogVerbosity::Display:
			UE_LOG(LogFlow, Display, TEXT("%s"), *Message);
			break;
		case EFlowLogVerbosity::Log:
			UE_LOG(LogFlow, Log, TEXT("%s"), *Message);
			break;
		case EFlowLogVerbosity::Verbose:
			UE_LOG(LogFlow, Verbose, TEXT("%s"), *Message);
			break;
		case EFlowLogVerbosity::VeryVerbose:
			UE_LOG(LogFlow, VeryVerbose, TEXT("%s"), *Message);
			break;
		default: ;
	}

	if (bPrintToScreen)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, TextColor, Message);
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNode_Log::GetNodeDescription() const
{
	return Message;
}
#endif
