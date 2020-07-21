#include "Nodes/Utils/FlowNodeLog.h"
#include "FlowModule.h"

#include "Engine/Engine.h"

UFlowNodeLog::UFlowNodeLog(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bPrintToScreen(true)
	, Duration(5.0f)
	, TextColor(FColor::Yellow)
{
#if WITH_EDITOR
	Category = TEXT("Utils");
#endif
}

void UFlowNodeLog::ExecuteInput(const FName& PinName)
{
	UE_LOG(LogFlow, Warning, TEXT("%s"), *Message);

	if (bPrintToScreen)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, TextColor, Message);
	}

	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNodeLog::GetNodeDescription() const
{
	return Message;
}
#endif
