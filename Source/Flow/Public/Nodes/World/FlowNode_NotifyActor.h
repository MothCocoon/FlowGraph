#pragma once

#include "GameplayTagContainer.h"

#include "Nodes/FlowNode.h"
#include "FlowNode_NotifyActor.generated.h"

/**
 * Finds all Flow Components with matching Identity Tag and calls ReceiveNotify event on these components
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Notify Actor"))
class FLOW_API UFlowNode_NotifyActor : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ObservedComponent")
	FGameplayTagContainer IdentityTags;
	
	UPROPERTY(EditDefaultsOnly, Category = "ObservedComponent")
	FGameplayTagContainer NotifyTags;

	virtual void PostLoad() override;
	
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif

private:
	UPROPERTY()
	FGameplayTag IdentityTag_DEPRECATED;

	UPROPERTY()
	FGameplayTag NotifyTag_DEPRECATED;
	
};
