#pragma once

#include "FlowNodeNotifyBase.h"
#include "FlowNodeOnNotifyFromActor.generated.h"

/**
 * On Notify From Actor
 */
UCLASS(meta = (DisplayName = "On Notify From Actor"))
class FLOW_API UFlowNodeOnNotifyFromActor : public UFlowNodeNotifyBase
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
