// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Reroute.h"
#include "FlowAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Reroute)

UFlowNode_Reroute::UFlowNode_Reroute()
{
#if WITH_EDITOR
	Category = TEXT("Route");
#endif

	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

void UFlowNode_Reroute::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}

#if WITH_EDITOR
void UFlowNode_Reroute::ConfigureInputPin(const UFlowNode& ConnectedNode, const FEdGraphPinType& EdGraphPinType)
{
	FFlowPin* InputPin = FindInputPinByName(UFlowNode::DefaultInputPin.PinName);
	check(InputPin);

	InputPin->ConfigureFromEdGraphPin(EdGraphPinType);
}

void UFlowNode_Reroute::ConfigureOutputPin(const UFlowNode& ConnectedNode, const FEdGraphPinType& EdGraphPinType)
{
	FFlowPin* OutputPin = FindOutputPinByName(UFlowNode::DefaultOutputPin.PinName);
	check(OutputPin);

	OutputPin->ConfigureFromEdGraphPin(EdGraphPinType);
}
#endif

FFlowDataPinResult UFlowNode_Reroute::TrySupplyDataPin(FName PinName) const
{
	const FFlowPin* InputPin = FindInputPinByName(UFlowNode::DefaultInputPin.PinName);
	if (!InputPin)
	{
		return FFlowDataPinResult(EFlowDataPinResolveResult::FailedUnknownPin);
	}

	FConnectedPin ConnectedPin;
	if (!FindFirstInputPinConnection(*InputPin, ConnectedPin))
	{
		return FFlowDataPinResult(EFlowDataPinResolveResult::FailedNotConnected);
	}

	const UFlowNode* ConnectedFlowNodeSupplier = GetFlowAsset()->GetNode(ConnectedPin.NodeGuid);
	if (!IsValid(ConnectedFlowNodeSupplier))
	{
		checkf(IsValid(ConnectedFlowNodeSupplier), TEXT("This node should be valid if IsInputConnected returned true"));

		return FFlowDataPinResult(EFlowDataPinResolveResult::FailedNotConnected);
	}

	// Hand-off to the connected flow node to supply the value
	return ConnectedFlowNodeSupplier->TrySupplyDataPin(ConnectedPin.PinName);
}
