#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FlowComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlowComponentNotify, class UFlowComponent*, FlowComponent, const FGameplayTag&, NotifyTag);

/**
* Base component of Flow System - makes possible to communicate between Flow graph and actors
*/
UCLASS(meta = (BlueprintSpawnableComponent))
class FLOW_API UFlowComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Flow")
	FGameplayTagContainer IdentityTags;

	static FFlowComponentNotify OnNotifyFromComponent;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Receive notification from Flow graph or another Flow Component
	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FFlowComponentNotify ReceiveNotify;

	// Send notification to Flow graphs
	UFUNCTION(BlueprintCallable, Category = "Flow")
	void NotifyGraph(const FGameplayTag NotifyTag);

	// Send notification to another actor containing Flow Component
	UFUNCTION(BlueprintCallable, Category = "Flow")
	virtual void NotifyActor(const FGameplayTag ActorTag, const FGameplayTag NotifyTag);
};
