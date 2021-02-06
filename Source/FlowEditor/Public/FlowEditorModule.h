#pragma once

#include "AssetTypeCategories.h"
#include "IAssetTypeActions.h"
#include "Modules/ModuleInterface.h"
#include "PropertyEditorDelegates.h"
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

DECLARE_LOG_CATEGORY_EXTERN(LogFlowEditor, Log, All)

class FFlowEditorModule : public IModuleInterface
{
public:
	static EAssetTypeCategories::Type FlowAssetCategory;

private:
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetActions;
	TSet<FName> CustomClassLayouts;
	TSharedPtr<FGraphPanelPinConnectionFactory> FlowGraphConnectionFactory;
	FExtensibilityManagers FlowAssetExtensibility;

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterAssets();
	void UnregisterAssets();

	void RegisterCustomClassLayout(const TSubclassOf<UObject> Class, const FOnGetDetailCustomizationInstance DetailLayout);

public:
	TSharedPtr<FExtensibilityManager> GetFlowAssetMenuExtensibilityManager() const;
	TSharedPtr<FExtensibilityManager> GetFlowAssetToolBarExtensibilityManager() const;

	FDelegateHandle FlowTrackCreateEditorHandle;

private:
	void CreateFlowToolbar(FToolBarBuilder& ToolbarBuilder) const;

public:
	TSharedRef<FFlowAssetEditor> CreateFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UFlowAsset* FlowAsset);
};
