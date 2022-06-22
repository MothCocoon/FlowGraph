// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "AssetTypeCategories.h"
#include "IAssetTypeActions.h"
#include "Modules/ModuleInterface.h"

class FSlateStyleSet;
struct FGraphPanelPinConnectionFactory;

class FFlowAssetEditor;
class UFlowAsset;

DECLARE_LOG_CATEGORY_EXTERN(LogFlowEditor, Log, All)

class FLOWEDITOR_API FFlowEditorModule : public IModuleInterface
{
public:
	static EAssetTypeCategories::Type FlowAssetCategory;

private:
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetActions;
	TSet<FName> CustomClassLayouts;
	TSharedPtr<FGraphPanelPinConnectionFactory> FlowGraphConnectionFactory;

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterAssets();
	void UnregisterAssets();

	void RegisterPropertyCustomizations() const;
	void RegisterCustomClassLayout(const TSubclassOf<UObject> Class, const FOnGetDetailCustomizationInstance DetailLayout);

public:
	FDelegateHandle FlowTrackCreateEditorHandle;
	FDelegateHandle ModulesChangedHandle;

private:
	void ModulesChangesCallback(FName ModuleName, EModuleChangeReason ReasonForChange);
	void RegisterAssetIndexers() const;

	void CreateFlowToolbar(FToolBarBuilder& ToolbarBuilder) const;

public:
	static TSharedRef<FFlowAssetEditor> CreateFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UFlowAsset* FlowAsset);
};
