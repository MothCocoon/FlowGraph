// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Route/FlowNode_ExecutionSequence.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_ExecutionSequence)

UFlowNode_ExecutionSequence::UFlowNode_ExecutionSequence()
{
#if WITH_EDITOR
	Category = TEXT("Route");
	NodeDisplayStyle = FlowNodeStyle::Logic;
#endif

	SetNumberedOutputPins(0, 1);
	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

void UFlowNode_ExecutionSequence::ExecuteInput(const FName& PinName)
{
	if (bSavePinExecutionState)
	{
		ExecuteNewConnections();
	}
	else
	{
		for (const FFlowPin& Output : OutputPins)
		{
			TriggerOutput(Output.PinName, false);
		}

		Finish();
	}
}

void UFlowNode_ExecutionSequence::OnLoad_Implementation()
{
	ExecuteNewConnections();
}

void UFlowNode_ExecutionSequence::Cleanup()
{
	ExecutedConnections.Empty();

	Super::Cleanup();
}

void UFlowNode_ExecutionSequence::ExecuteNewConnections()
{
	for (const FFlowPin& Output : OutputPins)
	{
		const FConnectedPin& Connection = GetConnection(Output.PinName);
		if (!ExecutedConnections.Contains(Connection.NodeGuid))
		{
			ExecutedConnections.Emplace(Connection.NodeGuid);
			TriggerOutput(Output.PinName, false);
		}
	}

	Finish();
}

#if WITH_EDITOR
FString UFlowNode_ExecutionSequence::GetNodeDescription() const
{
	if (bSavePinExecutionState)
	{
		return TEXT("Saves pin execution state");
	}

	return Super::GetNodeDescription();
}

const FFlowAgentDoc& UFlowNode_ExecutionSequence::GetAgentDoc() const
{
	static const FFlowAgentDoc Doc = MakeAgentDoc(
		/*Guidance*/ TEXT("Enable bSavePinExecutionState to persist, across save/load, which output connections have already fired - reconnecting or adding new outputs is detected automatically and those new connections still execute even after a load."),
		/*Tags*/     { TEXT("route"), TEXT("sequence"), TEXT("fan-out") },
		/*Articles*/ {  });
	return Doc;
}
#endif
