#pragma once

#include "Nodes/World/FlowNode_NotifyBase.h"
#include "FlowNode_NotifyActor.generated.h"

/**
 * Finds all Flow Components with matching Identity Tag and calls ReceiveNotify event on these components
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Notify Actor"))
class FLOW_API UFlowNode_NotifyActor : public UFlowNode_NotifyBase
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
