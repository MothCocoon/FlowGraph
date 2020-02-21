#pragma once

#include "GameFramework/WorldSettings.h"
#include "FlowWorldSettings.generated.h"

/**
 *
 */
UCLASS()
class FLOW_API AFlowWorldSettings : public AWorldSettings
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Flow")
	class UFlowAsset* FlowAsset;

	void StartFlow() const;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};