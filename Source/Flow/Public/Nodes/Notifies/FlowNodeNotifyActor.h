#pragma once

#include "FlowNodeNotifyBase.h"
#include "FlowNodeNotifyActor.generated.h"

/**
 * Notify Actor
 */
UCLASS(meta = (DisplayName = "Notify Actor"))
class FLOW_API UFlowNodeNotifyActor : public UFlowNodeNotifyBase
{
	GENERATED_UCLASS_BODY()
	
protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
