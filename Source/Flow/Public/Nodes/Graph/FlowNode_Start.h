// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/Graph/FlowNode_DefineProperties.h"
#include "Interfaces/FlowNodeWithExternalDataPinSupplierInterface.h"
#include "FlowNode_Start.generated.h"

/**
 * The mandatory single entry point for a standard Flow Asset - execution always begins here when the graph starts, firing its
 * output pin immediately. There is exactly one Start node per graph. Input data pins can be defined here to receive
 * values passed in from the parent SubGraph node or external suppliers.
 */
UCLASS(NotBlueprintable, NotPlaceable, meta = (DisplayName = "Start", Keywords = "start datapin"))
class FLOW_API UFlowNode_Start
	: public UFlowNode_DefineProperties
	, public IFlowNodeWithExternalDataPinSupplierInterface
{
	GENERATED_BODY()

public:
	UFlowNode_Start();

protected:
	/* External DataPin Value Supplier.
	 * Example: the UFlowNode_SubGraph that instanced this Start node's flow asset. */
	UPROPERTY(Transient)
	TScriptInterface<IFlowDataPinValueSupplierInterface> FlowDataPinValueSupplierInterface;

public:
	// IFlowCoreExecutableInterface
	virtual void ExecuteInput(const FName& PinName) override;
	// --

	// IFlowNodeWithExternalDataPinSupplierInterface
	virtual void SetDataPinValueSupplier(IFlowDataPinValueSupplierInterface* DataPinValueSupplier) override;
	virtual IFlowDataPinValueSupplierInterface* GetExternalDataPinSupplier() const override { return FlowDataPinValueSupplierInterface.GetInterface(); }
#if WITH_EDITOR
	virtual bool TryAppendExternalInputPins(TArray<FFlowPin>& InOutPins) const override;
#endif
	// --

	// IFlowDataPinValueSupplierInterface
	virtual FFlowDataPinResult TrySupplyDataPin(const FName PinName) const override;
	// --

#if WITH_EDITOR
public:
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
