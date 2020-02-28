#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FlowComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlowGraphNotify, const FGameplayTag&, Tag);
DECLARE_MULTICAST_DELEGATE_TwoParams(FFlowActorNotify, UFlowComponent* /*FlowComponent*/, const FGameplayTag& /*Tag*/);

/**
* Base component which makes possible to communicate between flow graphs and custom blueprint actors
*/
UCLASS(meta = (BlueprintSpawnableComponent))
class FLOW_API UFlowComponent final : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Flow")
	FGameplayTagContainer IdentityTags;

	static FFlowActorNotify OnNotifyFromActor;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FFlowGraphNotify NotifyFromGraph;

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void NotifyGraph(const FGameplayTag Tag);
};