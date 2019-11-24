#pragma once

#include "FlowNode.h"
#include "FlowNodeOut.generated.h"

/**
 * Out
 */
UCLASS(meta = (DisplayName = "Out"))
class FLOW_API UFlowNodeOut : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
#if WITH_EDITOR
public:
	virtual bool HasOutputPins() const override { return false; }
#endif
};
