// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode_Reroute.h"
#include "SGraphNodeKnot.h"

#include "Graph/FlowGraph.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Route/FlowNode_Reroute.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphNode_Reroute)

UFlowGraphNode_Reroute::UFlowGraphNode_Reroute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssignedNodeClasses = { UFlowNode_Reroute::StaticClass() };
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

void UFlowGraphNode_Reroute::ConfigureRerouteNodeFromPinConnections(UEdGraphPin& InPin, UEdGraphPin& OutPin)
{
	UFlowNode_Reroute* RerouteTemplate = Cast<UFlowNode_Reroute>(NodeInstance);
	if (!IsValid(RerouteTemplate))
	{
		return;
	}

	// IMPORTANT:
	// Use editor templates for "ConnectedNode" context.
	// GetFlowNodeBase() may return the inspected PIE instance, which we do not want to use for editor graph mutation.
	const UFlowGraphNode* FlowGraphNodeIn = Cast<UFlowGraphNode>(InPin.GetOwningNode());
	const UFlowNode* NodeInTemplate = FlowGraphNodeIn ? Cast<UFlowNode>(FlowGraphNodeIn->GetNodeTemplate()) : nullptr;

	const UFlowGraphNode* FlowGraphNodeOut = Cast<UFlowGraphNode>(OutPin.GetOwningNode());
	const UFlowNode* NodeOutTemplate = FlowGraphNodeOut ? Cast<UFlowNode>(FlowGraphNodeOut->GetNodeTemplate()) : nullptr;

	// Break existing wire first (we're inserting ourselves in between)
	InPin.BreakLinkTo(&OutPin);

	// Canonical reroute type: pick one type for BOTH pins.
	// Prefer the "source" (OutPin) type since it is the value provider.
	const FEdGraphPinType CanonicalType = OutPin.PinType;

	// Apply to our graph pins (visuals/wire coloring)
	if (InputPins.Num() > 0 && InputPins[0])
	{
		InputPins[0]->PinType = CanonicalType;
	}
	if (OutputPins.Num() > 0 && OutputPins[0])
	{
		OutputPins[0]->PinType = CanonicalType;
	}

	// Apply to our template pins (future allocations)
	{
		// If possible, configure with context of the connected nodes; otherwise fall back to self.
		const UFlowNode& InContext = NodeInTemplate ? *NodeInTemplate : *RerouteTemplate;
		const UFlowNode& OutContext = NodeOutTemplate ? *NodeOutTemplate : *RerouteTemplate;

		RerouteTemplate->ConfigureInputPin(InContext, CanonicalType);
		RerouteTemplate->ConfigureOutputPin(OutContext, CanonicalType);
	}

	// Restore reroute wiring
	if (OutputPins.Num() > 0 && OutputPins[0])
	{
		InPin.MakeLinkTo(OutputPins[0]);
	}
	if (InputPins.Num() > 0 && InputPins[0])
	{
		OutPin.MakeLinkTo(InputPins[0]);
	}

	// Nudge visuals
	if (UEdGraph* Graph = GetGraph())
	{
		Graph->NotifyNodeChanged(this);
	}
}

void UFlowGraphNode_Reroute::NodeConnectionListChanged()
{
	Super::NodeConnectionListChanged();
	ReconfigureFromConnections();
}

void UFlowGraphNode_Reroute::ApplyTypeFromConnectedPin(const UEdGraphPin& OtherPin)
{
	if (InputPins.Num() == 0 || OutputPins.Num() == 0)
	{
		return;
	}

	UEdGraphPin* const InputPin = InputPins[0];
	UEdGraphPin* const OutputPin = OutputPins[0];
	if (!InputPin || !OutputPin)
	{
		return;
	}

	UFlowNode_Reroute* RerouteTemplate = Cast<UFlowNode_Reroute>(NodeInstance);
	if (!IsValid(RerouteTemplate))
	{
		return;
	}

	const FEdGraphPinType NewType = OtherPin.PinType;

	// Nothing to do?
	if (InputPin->PinType == NewType && OutputPin->PinType == NewType)
	{
		return;
	}

	// Apply to graph pins (visual + connection coloring)
	InputPin->PinType = NewType;
	OutputPin->PinType = NewType;

	// Update template pins too, so future reconstructions allocate correct pin types.
	// Pass "connected node" context if possible; fall back to self.
	const UFlowNode* ConnectedTemplate = nullptr;
	if (const UFlowGraphNode* OtherGraphNode = Cast<UFlowGraphNode>(OtherPin.GetOwningNode()))
	{
		ConnectedTemplate = Cast<UFlowNode>(OtherGraphNode->GetNodeTemplate());
	}
	const UFlowNode& ConnectedNodeRef = ConnectedTemplate ? *ConnectedTemplate : *RerouteTemplate;

	RerouteTemplate->ConfigureInputPin(ConnectedNodeRef, NewType);
	RerouteTemplate->ConfigureOutputPin(ConnectedNodeRef, NewType);

	// Avoid reconstruct storms (esp. during paste). PinType changes are enough for visuals/wire colors.
	// If the graph is locked, defer the retype pass until UnlockUpdates().
	if (UFlowGraph* FlowGraph = Cast<UFlowGraph>(GetGraph()))
	{
		if (FlowGraph->IsLocked())
		{
			FlowGraph->EnqueueRerouteTypeFixup(this);
			return;
		}
	}

	if (UEdGraph* Graph = GetGraph())
	{
		Graph->NotifyNodeChanged(this);
	}
}

void UFlowGraphNode_Reroute::ReconfigureFromConnections()
{
	if (InputPins.Num() == 0 || OutputPins.Num() == 0)
	{
		return;
	}

	UEdGraphPin* const InputPin = InputPins[0];
	UEdGraphPin* const OutputPin = OutputPins[0];
	if (!InputPin || !OutputPin)
	{
		return;
	}

	// Determine desired type from whichever side is connected
	const UEdGraphPin* TypeSourceLinkedPin = nullptr;

	if (InputPin->LinkedTo.Num() > 0 && InputPin->LinkedTo[0])
	{
		TypeSourceLinkedPin = InputPin->LinkedTo[0];
	}
	else if (OutputPin->LinkedTo.Num() > 0 && OutputPin->LinkedTo[0])
	{
		TypeSourceLinkedPin = OutputPin->LinkedTo[0];
	}

	if (!TypeSourceLinkedPin)
	{
		// No connections => don't reset type here. Keep last known type.
		return;
	}

	ApplyTypeFromConnectedPin(*TypeSourceLinkedPin);
}
