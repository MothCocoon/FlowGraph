// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "FlowUserSettings.generated.h"

/**
 * 
 */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Flow"))
class FLOW_API UFlowUserSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

	static UFlowUserSettings* Get() { return CastChecked<UFlowUserSettings>(UFlowUserSettings::StaticClass()->GetDefaultObject()); }
	
	UPROPERTY(EditDefaultsOnly, config, Category = "Debug")
	bool bSetFirstAssetInstanceAsInspected;
	
	// Keep last inspected instance of this FlowAsset between PIE sessions
	UPROPERTY(EditDefaultsOnly, config, Category = "Debug")
	bool bKeepLastInspectedInstance;
	
	virtual FName GetCategoryName() const override { return FName("Flow Graph"); }
#if WITH_EDITORONLY_DATA
	virtual FText GetSectionText() const override { return INVTEXT("User Settings"); }
#endif
};
