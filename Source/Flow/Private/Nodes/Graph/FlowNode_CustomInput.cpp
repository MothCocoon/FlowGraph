// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Graph/FlowNode_CustomInput.h"

#include "FlowSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_CustomInput)

#define LOCTEXT_NAMESPACE "FlowNode_CustomInput"

UFlowNode_CustomInput::UFlowNode_CustomInput()
{
	InputPins.Empty();
}

void UFlowNode_CustomInput::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}

void UFlowNode_CustomInput::PostEditImport()
{
	// Reset EventName after duplicating or copy/pasting
	EventName = NAME_None;
}

#if WITH_EDITOR
FText UFlowNode_CustomInput::K2_GetNodeTitle_Implementation() const
{
	if (!EventName.IsNone() && GetDefault<UFlowSettings>()->bUseAdaptiveNodeTitles)
	{
		return FText::Format(LOCTEXT("CustomInputTitle", "{0} Input"), {FText::FromString(EventName.ToString())});
	}

	return Super::K2_GetNodeTitle_Implementation();
}
#endif

#undef LOCTEXT_NAMESPACE

#if WITH_EDITOR
const FFlowAgentDoc& UFlowNode_CustomInput::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("Set EventName to the name the parent SubGraph node should expose as an input pin. Useful for letting a parent graph trigger a specific behavior inside a running sub-graph (e.g. a Cancel signal) rather than waiting for the sub-graph to finish naturally."),
		/*Tags*/     { TEXT("graph"), TEXT("subgraph"), TEXT("input") },
		/*Articles*/ {  });
	return Doc;
}
#endif
