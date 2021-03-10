#pragma once

#include "GameplayTagContainer.h"

#include "Nodes/FlowNode.h"
#include "FlowNode_ComponentObserver.generated.h"

class UFlowComponent;

/**
 * Base class for nodes operating on actors with the Flow Component
 * Such nodes usually wait until a specific action occurs in the actor
 */
UCLASS(Abstract, NotBlueprintable)
class FLOW_API UFlowNode_ComponentObserver : public UFlowNode
{
	GENERATED_UCLASS_BODY()

    friend class FFlowNode_ComponentObserverDetails;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "ObservedComponent")
	FGameplayTag IdentityTag;

	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<UFlowComponent>> RegisteredActors;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void StartObserving();
	virtual void StopObserving();

	UFUNCTION()
	virtual void OnComponentRegistered(UFlowComponent* Component);

	UFUNCTION()
	virtual void OnComponentUnregistered(UFlowComponent* Component);

	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) {}
	virtual void ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) {}

	virtual void Cleanup() override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual FString GetStatusString() const override;
#endif
};
