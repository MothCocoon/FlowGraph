// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"

#include "Types/FlowDataPinResults.h"
#include "FlowDataPinValueSupplierInterface.generated.h"

/**
 * Interface to define a Flow Data Pin value supplier.  This is generally a UFlowNode subclass,
 * but we may support external suppliers that are not flow nodes in the future.
 * Example: for supplying configuration values for the root graph.
 */
UINTERFACE(MinimalAPI, NotBlueprintable, DisplayName = "Flow Data Pin Value Supplier Interface")
class UFlowDataPinValueSupplierInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowDataPinValueSupplierInterface
{
	GENERATED_BODY()

public:
	/* Can this node actually supply Data Pin values?
	 * Implementers of this interface will need to use their own logic to answer this question. */
	virtual bool CanSupplyDataPinValues() const { return true; }

	virtual FFlowDataPinResult TrySupplyDataPin(FName PinName) const { return FFlowDataPinResult(); }
};
