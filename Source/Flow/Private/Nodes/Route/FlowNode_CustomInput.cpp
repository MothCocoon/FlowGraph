// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_CustomInput.h"

UFlowNode_CustomInput::UFlowNode_CustomInput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeStyle = EFlowNodeStyle::InOut;
#endif

	InputPins.Empty();
	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
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

EDataValidationResult UFlowNode_CustomInput::ValidateNode()
{
	if (EventName.IsNone())
	{
		ValidationLog.Error<UFlowNode>(TEXT("Event Name is empty!"), this);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif
