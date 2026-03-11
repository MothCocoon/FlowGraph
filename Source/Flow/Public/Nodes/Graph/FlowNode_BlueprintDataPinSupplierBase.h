// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_BlueprintDataPinSupplierBase.generated.h"

/**
 * FlowNode to give an event to blueprint for supplying data pin values on-demand.
 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "Blueprint Data-Pin Supplier base"))
class FLOW_API UFlowNode_BlueprintDataPinSupplierBase : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_BlueprintDataPinSupplierBase();

	// IFlowDataPinValueSupplierInterface
	virtual FFlowDataPinResult TrySupplyDataPin(FName PinName) const override;
	// --

	/* Blueprint signature for TrySupplyDataPin override. */
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin")
	FFlowDataPinResult BP_TrySupplyDataPin(FName PinName) const;

	/* Blueprint access for the 'standard' implementation of TrySupplyDataPin
	 * For cases where they want to override some pins, but maybe not all, they can have the BP
	 * override call this version to handle any cases it doesn't want to handle. */
	UFUNCTION(BlueprintPure, Category = DataPins, DisplayName = "Try Supply DataPin (standard implementation)")
	FFlowDataPinResult BP_Super_TrySupplyDataPin(FName PinName) const { return Super::TrySupplyDataPin(PinName); }
};
