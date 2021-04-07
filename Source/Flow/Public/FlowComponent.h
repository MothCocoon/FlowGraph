#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "FlowTypes.h"
#include "FlowComponent.generated.h"

class UFlowAsset;
class UFlowSubsystem;

DECLARE_MULTICAST_DELEGATE_TwoParams(FFlowComponentNotify, class UFlowComponent*, const FGameplayTag&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlowComponentDynamicNotify, class UFlowComponent*, FlowComponent, const FGameplayTag&, NotifyTag);

/**
* Base component of Flow System - makes possible to communicate between Actor, Flow Subsystem and Flow Graphs
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

//////////////////////////////////////////////////////////////////////////
// Root Flow

public:
	// Asset that might instantiated as "Root Flow" 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RootFlow")
	UFlowAsset* RootFlow;

	// If true, component will start Root Flow on Begin Play
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RootFlow")
	bool bAutoStartRootFlow;

	// Networking mode for creating this Root Flow
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RootFlow")
	EFlowNetMode RootFlowMode;

	// If false, another Root Flow instance won't be created from this component, if this Flow Asset is already instantiated
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RootFlow")
	bool bAllowMultipleInstances;
	
	// This will instantiate Flow Asset assigned on this component.
	// Created Flow Asset instance will be a "root flow", as additional Flow Assets can be instantiated via Sub Graph node
	UFUNCTION(BlueprintCallable, Category = "RootFlow")
	void StartRootFlow();

	// This will destroy instantiated Flow Asset - created from asset assigned on this component.
	UFUNCTION(BlueprintCallable, Category = "RootFlow")
	void FinishRootFlow();

	UFUNCTION(BlueprintPure, Category = "RootFlow")
	UFlowAsset* GetRootFlowInstance();

//////////////////////////////////////////////////////////////////////////
// Helpers
	
	UFlowSubsystem* GetFlowSubsystem() const;
	bool IsFlowNetMode(const EFlowNetMode NetMode) const;
};
