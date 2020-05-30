#pragma once

#include "GameplayTagContainer.h"

#include "Nodes/FlowNode.h"
#include "FlowNodeOnNotifyFromActor.generated.h"

/**
 * On Notify From Actor
 */
UCLASS(meta = (DisplayName = "On Notify From Actor"))
class FLOW_API UFlowNodeOnNotifyFromActor : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
private:
	UPROPERTY(EditAnywhere, Category = "NotifyActor")
	FGameplayTag ActorTag;

	UPROPERTY(EditAnywhere, Category = "NotifyActor")
	FGameplayTag NotifyTag;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	UFUNCTION()
	void OnNotifyFromActor(class UFlowComponent* FlowComponent, const FGameplayTag& Tag);

protected:
	virtual void Cleanup() override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};