#pragma once

#include "Engine/DeveloperSettings.h"
#include "FlowSettings.generated.h"

class UFlowNode;

/**
 *
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Flow"))
class UFlowSettings final : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

public:
	static UFlowSettings* Get() { return CastChecked<UFlowSettings>(UFlowSettings::StaticClass()->GetDefaultObject()); }

	// How many nodes of given class should be preloaded with the Flow Asset instance?
	UPROPERTY(EditAnywhere, config, Category = "Preload")
	TMap<TSubclassOf<UFlowNode>, int32> DefaultPreloadDepth;
};
