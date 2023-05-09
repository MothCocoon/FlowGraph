// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_CustomInput.h"
#include "FlowSettings.h"

#define LOCTEXT_NAMESPACE "FlowNode_CustomInput"

UFlowNode_CustomInput::UFlowNode_CustomInput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins.Empty();
}

void UFlowNode_CustomInput::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}

#if WITH_EDITOR
FText UFlowNode_CustomInput::GetNodeTitle() const
{
	if (!EventName.IsNone() && UFlowSettings::Get()->bUseAdaptiveNodeTitles)
	{
		return FText::Format(LOCTEXT("CustomInputTitle", "{0} Input"), {FText::FromString(EventName.ToString())});
	}

	return Super::GetNodeTitle();
}
#endif

#undef LOCTEXT_NAMESPACE
