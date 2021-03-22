#include "Nodes/Route/FlowNode_CustomInput.h"

UFlowNode_CustomInput::UFlowNode_CustomInput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	InputNames.Empty();
}

void UFlowNode_CustomInput::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FString UFlowNode_CustomInput::GetNodeDescription() const
{
	return EventName.ToString();
}
#endif
