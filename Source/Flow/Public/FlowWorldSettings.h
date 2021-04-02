#pragma once

#include "GameFramework/WorldSettings.h"
#include "FlowWorldSettings.generated.h"

class UFlowGraphComponent;

/**
 * World Settings used to start a Flow for this world
 */
UCLASS()
class FLOW_API AFlowWorldSettings : public AWorldSettings
{
	GENERATED_UCLASS_BODY()

public:	
	UPROPERTY(EditAnywhere, Category = "Flow")
	UFlowGraphComponent* FlowGraph;
};
