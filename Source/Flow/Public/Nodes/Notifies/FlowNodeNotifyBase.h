#pragma once

#include "GameplayTagContainer.h"

#include "Nodes/FlowNode.h"
#include "FlowNodeNotifyBase.generated.h"

/**
 * Base for nodes providing generic communication between Flow Graph and actors with Flow Component
 */
UCLASS(Abstract)
class FLOW_API UFlowNodeNotifyBase : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
protected:
	UPROPERTY(EditAnywhere, Category = "NotifyActor")
	FGameplayTag IdentityTag;

	UPROPERTY(EditAnywhere, Category = "NotifyActor")
	FGameplayTag NotifyTag;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};
