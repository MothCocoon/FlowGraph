#pragma once

#include "GameFramework/WorldSettings.h"
#include "FlowWorldSettings.generated.h"

/**
 * World Settings used to start a Flow for this world
 */
UCLASS()
class FLOW_API AFlowWorldSettings : public AWorldSettings
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Flow")
	class UFlowAsset* FlowAsset;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
