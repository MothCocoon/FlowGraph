// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/Graph/FlowNode_DefineProperties.h"
#include "Interfaces/FlowNodeWithExternalDataPinSupplierInterface.h"
#include "FlowNode_Start.generated.h"

/**
 * Execution of the graph always starts from this node
 */
UCLASS(NotBlueprintable, NotPlaceable, meta = (DisplayName = "Start"))
class FLOW_API UFlowNode_Start
	: public UFlowNode_DefineProperties
	, public IFlowNodeWithExternalDataPinSupplierInterface
{
	GENERATED_UCLASS_BODY()

	friend class UFlowAsset;

protected:

	// External DataPin Value Supplier
	// (eg, the UFlowNode_SubGraph that instanced this Start node's flow asset)
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
	virtual FFlowDataPinResult TrySupplyDataPin_Implementation(FName PinName) const override;
	// --
};
