// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Reroute.generated.h"

/**
 * Reroute.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Reroute"))
class FLOW_API UFlowNode_Reroute final : public UFlowNode
{
	GENERATED_BODY()
	
public:
	UFlowNode_Reroute();

protected:
	// IFlowCoreExecutableInterface
	virtual void ExecuteInput(const FName& PinName) override;
	// --

	// IFlowDataPinValueSupplierInterface
	virtual FFlowDataPinResult TrySupplyDataPin(FName PinName) const override;
	// --

public:
#if WITH_EDITOR
	// For configuration from connecting pins via UFlowGraphNode_Reroute
	void ConfigureInputPin(const UFlowNode& ConnectedNode, const FEdGraphPinType& EdGraphPinType);
	void ConfigureOutputPin(const UFlowNode& ConnectedNode, const FEdGraphPinType& EdGraphPinType);
#endif
};
