#pragma once

#include "AssetTypeCategories.h"
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Toolkits/AssetEditorToolkit.h"

class FSlateStyleSet;
struct FGraphPanelPinConnectionFactory;

class IFlowAssetEditor;
class UFlowAsset;

struct FExtensibilityManagers
{
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;

	void Init()
	{
		MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
		ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);
	}

	void Reset()
	{
		MenuExtensibilityManager.Reset();
		ToolBarExtensibilityManager.Reset();
	}
};

static EAssetTypeCategories::Type FlowAssetCategory;

DECLARE_LOG_CATEGORY_EXTERN(LogFlowEditor, Log, All);

class FFlowEditorModule : public IModuleInterface
{
public:
	static FString FlowContentDir;

private:
	TSharedPtr<FSlateStyleSet> FlowStyleSet;
	TSharedPtr<FGraphPanelPinConnectionFactory> FlowGraphConnectionFactory;
	FExtensibilityManagers FlowAssetExtensibility;

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedRef<IFlowAssetEditor> CreateFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UFlowAsset* FlowAsset);
	TSharedPtr<FExtensibilityManager> GetFlowAssetMenuExtensibilityManager();
	TSharedPtr<FExtensibilityManager> GetFlowAssetToolBarExtensibilityManager();

private:
	void SetupIcons();
};
