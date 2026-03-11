// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"
#include "FlowNodeWithExternalDataPinSupplierInterface.generated.h"

class IFlowDataPinValueSupplierInterface;
struct FFlowPin;

/**
 * Interface for special flow node types that support an external data pin supplier.
 * The primary (only?) implementing node is UFlowNode_Start, which supplies its pin data externally from
 * either the SubGraph that instanced the graph that is being started.
 */
UINTERFACE(MinimalAPI, NotBlueprintable, DisplayName = "Flow Node With External Data Pin Value Supplier Interface")
class UFlowNodeWithExternalDataPinSupplierInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowNodeWithExternalDataPinSupplierInterface
{
	GENERATED_BODY()

public:
	/* Set the external DataPinValueSupplier for this node to use. */
	virtual void SetDataPinValueSupplier(IFlowDataPinValueSupplierInterface* DataPinValueSupplier) = 0;

	/* Append the external InputPins for the external supplier to include in its own pins (eg, UFlowNode_Subgraph). */
	virtual bool TryAppendExternalInputPins(TArray<FFlowPin>& InOutPins) const { return false; }

	/* Get the IFlowDataPinValueSupplierInterface for the external supplier for this node. */
	virtual IFlowDataPinValueSupplierInterface* GetExternalDataPinSupplier() const = 0;
};
