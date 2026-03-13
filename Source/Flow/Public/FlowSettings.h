// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPath.h"
#include "FlowSettings.generated.h"

/**
 * Mostly runtime settings of the Flow Graph.
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Flow"))
class FLOW_API UFlowSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/* If True, defer the Triggered Outputs for a FlowAsset while it is currently processing a TriggeredInput.
     * If False, use legacy behavior for backward compatability. */
	UPROPERTY(Config, EditAnywhere, Category = "Flow")
	bool bDeferTriggeredOutputsWhileTriggering;

	/* If enabled, runtime logs will be added when a flow node signal mode is set to Disabled. */
	UPROPERTY(Config, EditAnywhere, Category = "Flow")
	bool bLogOnSignalDisabled;

	/* If enabled, runtime logs will be added when a flow node signal mode is set to Pass-through. */
	UPROPERTY(Config, EditAnywhere, Category = "Flow")
	bool bLogOnSignalPassthrough;

	/* Set if to False, if you don't want to create client-side Flow Graphs.
	 * And you don't access to the Flow Component registry on clients. */
	UPROPERTY(Config, EditAnywhere, Category = "Networking")
	bool bCreateFlowSubsystemOnClients;

	/* Adjust the Titles for FlowNodes to be more expressive than default
	 * by incorporating data that would otherwise go in the Description. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bUseAdaptiveNodeTitles;
	
#if WITH_EDITOR
	DECLARE_DELEGATE(FFlowSettingsEvent);
	FFlowSettingsEvent OnAdaptiveNodeTitlesChanged;
#endif
	
	/* Default class to use as a FlowAsset's "ExpectedOwnerClass". */
	UPROPERTY(EditAnywhere, Config, Category = "Nodes")
	FSoftClassPath DefaultExpectedOwnerClass;

	UPROPERTY(Config, EditAnywhere, Category = "SaveSystem")
	bool bWarnAboutMissingIdentityTags;

public:
	UClass* GetDefaultExpectedOwnerClass() const;

#if WITH_EDITORONLY_DATA
	virtual FName GetCategoryName() const override { return FName("Flow Graph"); }
	virtual FText GetSectionText() const override { return INVTEXT("Settings"); }
#endif
};
