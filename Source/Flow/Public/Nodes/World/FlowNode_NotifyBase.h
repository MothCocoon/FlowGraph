#pragma once

#include "GameplayTagContainer.h"

#include "Nodes/FlowNode.h"
#include "FlowNode_NotifyBase.generated.h"

/**
 * Base for nodes providing generic communication between Flow Graph and actors with Flow Component
 */
UCLASS(Abstract, NotBlueprintable)
class FLOW_API UFlowNode_NotifyBase : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Notify")
	FGameplayTag IdentityTag;

	UPROPERTY(EditDefaultsOnly, Category = "Notify")
	FGameplayTag NotifyTag;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};
