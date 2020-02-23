#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FlowComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlowNotify, const FGameplayTag&, Tag);

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

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FFlowNotify OnNotify;
};