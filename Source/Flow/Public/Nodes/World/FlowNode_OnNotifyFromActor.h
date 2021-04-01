#pragma once

#include "Nodes/World/FlowNode_ComponentObserver.h"
#include "FlowNode_OnNotifyFromActor.generated.h"

/**
 * Triggers output when Flow Component with matching Identity Tag calls NotifyGraph function with matching Notify Tag
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Notify From Actor"))
class FLOW_API UFlowNode_OnNotifyFromActor : public UFlowNode_ComponentObserver
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ObservedComponent")
	FGameplayTagContainer NotifyTags;

	virtual void PostLoad() override;

	virtual void ExecuteInput(const FName& PinName) override;

	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;
	virtual void ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif

private:
	UPROPERTY()
	FGameplayTag NotifyTag_DEPRECATED;
};
