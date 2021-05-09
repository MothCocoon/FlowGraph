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
	UPROPERTY(EditAnywhere, Category = "ObservedComponent")
	FGameplayTagContainer IdentityTags;

	// This node will become Completed, if Success Limit > 0 and Success Count reaches this limit
	// Set this to zero, if you'd like receive events indefinitely (node would finish work only if explicitly Stopped)
	UPROPERTY(EditAnywhere, Category = "Lifetime", meta = (ClampMin = 0))
	int32 SuccessLimit;

	// This node will become Completed, if Success Limit > 0 and Success Count reaches this limit
	UPROPERTY(VisibleAnywhere, Category = "Lifetime")
	int32 SuccessCount;
	
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<UFlowComponent>> RegisteredActors;

protected:
	virtual void PostLoad() override;
	
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void OnLoad_Implementation() override;

	virtual void StartObserving();
	virtual void StopObserving();

	UFUNCTION()
	virtual void OnComponentRegistered(UFlowComponent* Component);

	UFUNCTION()
    virtual void OnComponentTagAdded(UFlowComponent* Component, const FGameplayTagContainer& AddedTags);

	UFUNCTION()
	virtual void OnComponentUnregistered(UFlowComponent* Component);

	UFUNCTION()
    virtual void OnComponentTagRemoved(UFlowComponent* Component, const FGameplayTagContainer& RemovedTags);

	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) {}
	virtual void ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) {}

	UFUNCTION()
	virtual void OnEventReceived();
	
	virtual void Cleanup() override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual FString GetStatusString() const override;
#endif

private:
	UPROPERTY()
	FGameplayTag IdentityTag_DEPRECATED;
};
