#pragma once

#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"
#include "FlowSettings.generated.h"

class UFlowNode;

/**
 *
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Flow"))
class UFlowSettings final : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

	static UFlowSettings* Get() { return CastChecked<UFlowSettings>(UFlowSettings::StaticClass()->GetDefaultObject()); }

	// How many nodes of given class should be preloaded with the Flow Asset instance?
	UPROPERTY(Config, EditAnywhere, Category = "Preload")
	TMap<TSubclassOf<UFlowNode>, int32> DefaultPreloadDepth;
};
