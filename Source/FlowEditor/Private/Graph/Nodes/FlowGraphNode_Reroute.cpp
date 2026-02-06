// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode_Reroute.h"
#include "SGraphNodeKnot.h"

#include "Nodes/Route/FlowNode_Reroute.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphNode_Reroute)

UFlowGraphNode_Reroute::UFlowGraphNode_Reroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = {UFlowNode_Reroute::StaticClass()};
}

TSharedPtr<SGraphNode> UFlowGraphNode_Reroute::CreateVisualWidget()
{
	return SNew(SGraphNodeKnot, this);
}

bool UFlowGraphNode_Reroute::ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const
{
	OutInputPinIndex = 0;
	OutOutputPinIndex = 1;
	return true;
}

bool UFlowGraphNode_Reroute::CanPlaceBreakpoints() const
{
	return false;
}

#if WITH_EDITOR
void UFlowGraphNode_Reroute::ConfigureRerouteNodeFromPinConnections(UEdGraphPin& InPin, UEdGraphPin& OutPin)
{
	UFlowNode_Reroute* NewRerouteTemplate = Cast<UFlowNode_Reroute>(NodeInstance);

	UFlowGraphNode* FlowGraphNodeIn = Cast<UFlowGraphNode>(InPin.GetOwningNode());
	UFlowNode* NodeIn = Cast<UFlowNode>(FlowGraphNodeIn->GetFlowNodeBase());

	UFlowGraphNode* FlowGraphNodeOut = Cast<UFlowGraphNode>(OutPin.GetOwningNode());
	UFlowNode* NodeOut = Cast<UFlowNode>(FlowGraphNodeOut->GetFlowNodeBase());

	// Update the FlowPin structs on the UFlowNode_Reroute
	NewRerouteTemplate->ConfigureInputPin(*NodeIn, InPin.PinType);
	NewRerouteTemplate->ConfigureOutputPin(*NodeOut, OutPin.PinType);

	InPin.BreakLinkTo(&OutPin);

	// Copy the PinType information from the connected pins
 	InputPins[0]->PinType = InPin.PinType;
 	OutputPins[0]->PinType = OutPin.PinType;

	InPin.MakeLinkTo(OutputPins[0]);
	OutPin.MakeLinkTo(InputPins[0]);
}
#endif
