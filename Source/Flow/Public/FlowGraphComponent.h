#pragma once

#include "Components/ActorComponent.h"
#include "FlowGraphComponent.generated.h"

class UFlowAsset;

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class FLOW_API UFlowGraphComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Flow")
	UFlowAsset* FlowAsset;

	UPROPERTY(EditAnywhere, Category = "Flow")
	bool bAuthorityOnly = true;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
