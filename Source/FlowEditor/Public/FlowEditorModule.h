#pragma once

#include "AssetTypeCategories.h"
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Toolkits/AssetEditorToolkit.h"

class FSlateStyleSet;
struct FGraphPanelPinConnectionFactory;

class FFlowAssetEditor;
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

DECLARE_LOG_CATEGORY_EXTERN(LogFlowEditor, Log, All);

class FFlowEditorModule : public IModuleInterface
{
public:
	static EAssetTypeCategories::Type FlowAssetCategory;

private:
	TSharedPtr<FGraphPanelPinConnectionFactory> FlowGraphConnectionFactory;
	FExtensibilityManagers FlowAssetExtensibility;

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedRef<FFlowAssetEditor> CreateFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UFlowAsset* FlowAsset);
	TSharedPtr<FExtensibilityManager> GetFlowAssetMenuExtensibilityManager() const;
	TSharedPtr<FExtensibilityManager> GetFlowAssetToolBarExtensibilityManager() const;

private:
	void CreateFlowToolbar(FToolBarBuilder& ToolbarBuilder) const;
};