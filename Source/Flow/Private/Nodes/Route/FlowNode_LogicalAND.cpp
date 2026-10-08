// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Route/FlowNode_LogicalAND.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_LogicalAND)

UFlowNode_LogicalAND::UFlowNode_LogicalAND()
{
#if WITH_EDITOR
	Category = TEXT("Route|Logic");
	NodeDisplayStyle = FlowNodeStyle::Logic;
#endif

	SetNumberedInputPins(0, 1);
}

void UFlowNode_LogicalAND::ExecuteInput(const FName& PinName)
{
	ExecutedInputNames.Add(PinName);

	if (ExecutedInputNames.Num() == InputPins.Num())
	{
		TriggerFirstOutput(true);
	}
}

void UFlowNode_LogicalAND::Cleanup()
{
	ExecutedInputNames.Empty();
}

#if WITH_EDITOR
FString UFlowNode_LogicalAND::GetStatusString() const
{
	FTextBuilder TextBuilder;

	if (ActivationState != EFlowNodeState::NeverActivated)
	{
		for (const FName& PinName : ExecutedInputNames)
		{
			TextBuilder.AppendLine(PinName.ToString());
		}
	}

	return TextBuilder.ToText().ToString();
}

const FFlowAgentDoc& UFlowNode_LogicalAND::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("Add input pins via the node's context menu to synchronize an arbitrary number of parallel branches before continuing - the node tracks distinct pin names triggered, not trigger count, so re-triggering the same input pin twice does not help it fire early."),
		/*Tags*/     { TEXT("route"), TEXT("logic"), TEXT("synchronize") },
		/*Articles*/ {  });
	return Doc;
}
#endif
