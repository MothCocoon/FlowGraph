// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

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
	TObjectPtr<UFlowComponent> FlowComponent;

public:
	UFlowComponent* GetFlowComponent() const { return FlowComponent; }
};
