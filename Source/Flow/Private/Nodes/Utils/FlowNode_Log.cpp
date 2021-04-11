#include "Nodes/Utils/FlowNode_Log.h"
#include "FlowModule.h"

#include "Engine/Engine.h"

UFlowNode_Log::UFlowNode_Log(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Message(TEXT("Log!"))
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
	UE_LOG(LogFlow, Warning, TEXT("%s"), *Message);

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
