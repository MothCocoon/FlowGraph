// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"

#include "FlowMultiGateTestTypes.generated.h"

/**
 * Stays active and counts received inputs.
 */
UCLASS(Hidden, NotBlueprintable, NotPlaceable, meta = (ExcludeFromFlowCatalog))
class UFlowMultiGateTestSink : public UFlowNode
{
	GENERATED_BODY()

public:
	virtual void ExecuteInput(const FName& PinName) override { ++ExecuteInputCount; }

	int32 ExecuteInputCount = 0;
};
