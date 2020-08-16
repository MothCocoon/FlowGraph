#pragma once

#include "Nodes/World/FlowNode_NotifyBase.h"
#include "FlowNode_OnNotifyFromActor.generated.h"

/**
 * On Notify From Actor
 */
UCLASS(meta = (DisplayName = "On Notify From Actor"))
class FLOW_API UFlowNode_OnNotifyFromActor : public UFlowNode_NotifyBase
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	UFUNCTION()
	void OnNotifyFromActor(class UFlowComponent* FlowComponent, const FGameplayTag& Tag);

protected:
	virtual void Cleanup() override;
};
