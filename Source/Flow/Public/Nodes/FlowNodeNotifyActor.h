#pragma once

#include "GameplayTagContainer.h"

#include "Nodes/FlowNode.h"
#include "FlowNodeNotifyActor.generated.h"

/**
 * Notify Actor
 */
UCLASS(meta = (DisplayName = "Notify Actor"))
class FLOW_API UFlowNodeNotifyActor : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
private:
	UPROPERTY(EditAnywhere, Category = "NotifyActor")
	FGameplayTag ActorTag;

	UPROPERTY(EditAnywhere, Category = "NotifyActor")
	FGameplayTag NotifyTag;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};