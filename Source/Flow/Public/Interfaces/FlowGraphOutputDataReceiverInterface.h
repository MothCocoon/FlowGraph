// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"

#include "FlowGraphOutputDataReceiverInterface.generated.h"

struct FFlowOutputDataPinValues;

/**
 * Interface for objects that receive a snapshot of a Flow Asset's output data pin values
 * when the asset's graph finishes execution.
 * Example: UFlowNode_SubGraph receives the snapshot and caches it for downstream pin resolution.
 */
UINTERFACE(MinimalAPI, NotBlueprintable, DisplayName = "Flow Graph Output Data Receiver Interface")
class UFlowGraphOutputDataReceiverInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowGraphOutputDataReceiverInterface
{
	GENERATED_BODY()

public:
	virtual void ReceiveOutputDataSnapshot(const FFlowOutputDataPinValues& OutputDataPinValues) { }
};
