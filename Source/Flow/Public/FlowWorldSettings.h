#pragma once

#include "GameFramework/WorldSettings.h"
#include "FlowWorldSettings.generated.h"

class UFlowComponent;

/**
 * World Settings used to start a Flow for this world
 */
UCLASS()
class FLOW_API AFlowWorldSettings : public AWorldSettings
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Flow", meta = (AllowPrivateAccess = "true"))
	UFlowComponent* FlowComponent;

public:
	UFlowComponent* GetFlowComponent() const { return FlowComponent; }

	virtual void PostLoad() override;
	virtual void PostInitializeComponents() override;

private:
	UPROPERTY()
	class UFlowAsset* FlowAsset_DEPRECATED;
};
