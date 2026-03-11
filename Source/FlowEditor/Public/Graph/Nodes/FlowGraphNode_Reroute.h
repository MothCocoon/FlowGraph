// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowGraphNode_Reroute.generated.h"

UCLASS()
class FLOWEDITOR_API UFlowGraphNode_Reroute : public UFlowGraphNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const override;
	// --

	virtual bool CanPlaceBreakpoints() const override;

	void ConfigureRerouteNodeFromPinConnections(UEdGraphPin& InPin, UEdGraphPin &OutPin);

	virtual void NodeConnectionListChanged() override;

	/* Re-type this reroute based on a pin it is connected to (or is being connected to).
     * This is the single place that should:
	 * - update reroute graph pin types
	 * - update reroute template (UFlowNode_Reroute) pin types
	 * - refresh visuals without forcing a reconstruct storm */
	void ApplyTypeFromConnectedPin(const UEdGraphPin& OtherPin);

private:
	void ReconfigureFromConnections();
};