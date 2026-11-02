// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Engine/DeveloperSettings.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/SoftObjectPath.h"

#include "FlowTypes.h"
#include "FlowSettings.generated.h"

struct FFlowPinConnectionPolicy;
struct FFlowPreloadPolicy;

/**
 * Mostly runtime settings of the Flow Graph.
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Flow"))
class FLOW_API UFlowSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

public:
	/* Returns a typed pointer to the current pin connection policy, or nullptr if unset/invalid. */
	const FFlowPinConnectionPolicy* GetPinConnectionPolicy() const;

	/* Returns a typed pointer to the current preload policy, or nullptr if unset/invalid. */
	const FFlowPreloadPolicy* GetPreloadPolicy() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/* The policy for connecting pins in the Flow Graph Editor. */
	UPROPERTY(EditAnywhere, config, Category = "Default Policies", DisplayName = "Pin Connection Policy", NoClear, meta = (ExcludeBaseStruct, BaseStruct = "/Script/Flow.FlowPinConnectionPolicy"))
	FInstancedStruct PinConnectionPolicy;

	UPROPERTY(EditAnywhere, config, Category = "Default Policies", DisplayName = "Preload Policy", NoClear, meta = (ExcludeBaseStruct, BaseStruct = "/Script/Flow.FlowPreloadPolicy"))
	FInstancedStruct PreloadPolicy;

	/* What happens after a SubGraph finishes: Deinitialize removes its instance, Keep retains it until rerun or owner deinitialization, and Reuse restarts the same instance on rerun.
	 * Flow Asset class can override it. */
	UPROPERTY(Config, EditAnywhere, Category = "Flow")
	ESubGraphFinishPolicy SubGraphFinishPolicy = ESubGraphFinishPolicy::Deinitialize;
	
	/* If True, defer the Triggered Outputs for a FlowAsset while it is currently processing a TriggeredInput.
     * If False, use legacy behavior for backward compatability. */
	UPROPERTY(Config, EditAnywhere, Category = "Flow")
	bool bDeferTriggeredOutputsWhileTriggering = true;

	/* If True, a Flow Asset containing an invalid node won't be instanced.
	 * If False, invalid nodes are skipped and the rest of the graph runs, as in legacy behavior.
	 * Keep False in a released game, if its cooked graphs might already contain invalid nodes. */
	UPROPERTY(Config, EditAnywhere, Category = "Flow")
	bool bRejectFlowInstancesWithInvalidNodes = false;

	/* If enabled, runtime logs will be added when a flow node signal mode is set to Disabled. */
	UPROPERTY(Config, EditAnywhere, Category = "Flow")
	bool bLogOnSignalDisabled = true;

	/* If enabled, runtime logs will be added when a flow node signal mode is set to Pass-through. */
	UPROPERTY(Config, EditAnywhere, Category = "Flow")
	bool bLogOnSignalPassthrough = true;

	/* Set if to False, if you don't want to create client-side Flow Graphs.
	 * And you don't access to the Flow Component registry on clients. */
	UPROPERTY(Config, EditAnywhere, Category = "Networking")
	bool bCreateFlowSubsystemOnClients = true;

	/* If True, Flow Asset instances replicate to clients as subobjects of their replicated Owner.
	 * Keep False, if the project handles Flow networking on its own. */
	UPROPERTY(Config, EditAnywhere, Category = "Networking", meta = (ConfigRestartRequired = true))
	bool bReplicateFlowInstances = false;

	/* Adjust the Titles for FlowNodes to be more expressive than default
	 * by incorporating data that would otherwise go in the Description. */
	UPROPERTY(EditAnywhere, config, Category = "Nodes")
	bool bUseAdaptiveNodeTitles = false;

#if WITH_EDITOR
	DECLARE_DELEGATE(FFlowSettingsEvent);
	FFlowSettingsEvent OnAdaptiveNodeTitlesChanged;
#endif

	/* Default class to use as a FlowAsset's "ExpectedOwnerClass". */
	UPROPERTY(EditAnywhere, Config, Category = "Nodes")
	FSoftClassPath DefaultExpectedOwnerClass;

	UPROPERTY(Config, EditAnywhere, Category = "SaveSystem")
	bool bWarnAboutMissingIdentityTags = true;

#if WITH_EDITORONLY_DATA
	/* Trigger validation on asset save.
	 * Flow Asset class can override it. */
	UPROPERTY(Config, EditAnywhere, Category = "Validation")
	bool bValidateAssetOnSave = true;
#endif

public:
	UClass* GetDefaultExpectedOwnerClass() const;

#if WITH_EDITORONLY_DATA
	virtual FName GetCategoryName() const override { return FName("Flow Graph"); }
	virtual FText GetSectionText() const override { return INVTEXT("Settings"); }
#endif
};
