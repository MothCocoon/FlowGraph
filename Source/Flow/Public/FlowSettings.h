// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"
#include "FlowSettings.generated.h"

class UFlowNode;

/**
 *
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Flow"))
class FLOW_API UFlowSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

	static UFlowSettings* Get() { return CastChecked<UFlowSettings>(UFlowSettings::StaticClass()->GetDefaultObject()); }

	// Set if to False, if you don't want to create client-side Flow Graphs
	// And you don't access to the Flow Component registry on clients
	UPROPERTY(Config, EditAnywhere, Category = "Networking")
	bool bCreateFlowSubsystemOnClients;

	// How many nodes of given class should be preloaded with the Flow Asset instance?
	UPROPERTY(Config, EditAnywhere, Category = "Preload")
	TMap<TSubclassOf<UFlowNode>, int32> DefaultPreloadDepth;
	
	UPROPERTY(Config, EditAnywhere, Category = "SaveSystem")
	bool bWarnAboutMissingIdentityTags;

	// If enabled, runtime logs will be added when a flow node signal mode is set to Disabled
	UPROPERTY(Config, EditAnywhere, Category = "Flow")
	bool bLogOnSignalDisabled;

	// If enabled, runtime logs will be added when a flow node signal mode is set to Pass-through
	UPROPERTY(Config, EditAnywhere, Category = "Flow")
	bool bLogOnSignalPassthrough;

	// Adjust the Titles for FlowNodes to be more expressive than default
	// by incorporating data that would otherwise go in the Description
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bUseAdaptiveNodeTitles;
};
