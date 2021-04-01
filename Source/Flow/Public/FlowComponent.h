#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FlowComponent.generated.h"

class UFlowSubsystem;

DECLARE_MULTICAST_DELEGATE_TwoParams(FFlowComponentNotify, class UFlowComponent*, const FGameplayTag&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlowComponentDynamicNotify, class UFlowComponent*, FlowComponent, const FGameplayTag&, NotifyTag);

/**
* Base component of Flow System - makes possible to communicate between Flow graph and actors
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class FLOW_API UFlowComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

//////////////////////////////////////////////////////////////////////////
// Identity Tags
	friend class UFlowSubsystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow")
	FGameplayTagContainer IdentityTags;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Flow")
	void AddIdentityTag(const FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Flow")
    void AddIdentityTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Flow")
	void RemoveIdentityTag(const FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Flow")
    void RemoveIdentityTags(FGameplayTagContainer Tags);

//////////////////////////////////////////////////////////////////////////
// Notify Tags: set only in runtime
	
	// Send notification from the actor to Flow graphs
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Flow")
    void NotifyGraph(const FGameplayTag NotifyTag);

	FFlowComponentNotify OnNotifyFromComponent;
	
	// Receive notification from Flow graph or another Flow Component
	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FFlowComponentDynamicNotify ReceiveNotify;

	virtual void NotifyFromGraph(const FGameplayTagContainer& NotifyTags);

	// Send notification to another actor containing Flow Component
	UFUNCTION(BlueprintCallable, Category = "Flow")
	virtual void NotifyActor(const FGameplayTag ActorTag, const FGameplayTag NotifyTag);

protected:
	UFlowSubsystem* GetFlowSubsystem() const;
};
