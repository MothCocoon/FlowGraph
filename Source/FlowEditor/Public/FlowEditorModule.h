// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "AssetTypeCategories.h"
#include "IAssetTypeActions.h"
#include "Modules/ModuleInterface.h"
#include "PropertyEditorDelegates.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkit.h"

class FSlateStyleSet;
class FToolBarBuilder;
struct FGraphPanelPinConnectionFactory;

class FFlowAssetEditor;
class UFlowAsset;

struct FLOWEDITOR_API FFlowAssetCategoryPaths : EAssetCategoryPaths
{
	static FAssetCategoryPath Flow;
};

class FLOWEDITOR_API FFlowEditorModule : public IModuleInterface
{
public:
	static EAssetTypeCategories::Type FlowAssetCategory;

private:
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetActions;
	TSet<FName> CustomClassLayouts;
	TSet<FName> CustomStructLayouts;

	bool bIsRegisteredForAssetChanges = false;

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void RegisterForAssetChanges();

private:
	void TrySetFlowNodeDisplayStyleDefaults() const;

	void RegisterAssets();
	void UnregisterAssets();

	void RegisterDetailCustomizations();
	void UnregisterDetailCustomizations();

	void RegisterCustomClassLayout(const TSubclassOf<UObject> Class, const FOnGetDetailCustomizationInstance DetailLayout);
	void RegisterCustomStructLayout(const UScriptStruct& Struct, const FOnGetPropertyTypeCustomizationInstance DetailLayout);

public:
	FDelegateHandle FlowTrackCreateEditorHandle;
	FDelegateHandle ModulesChangedHandle;

private:
	void ModulesChangesCallback(FName ModuleName, EModuleChangeReason ReasonForChange) const;
	void RegisterAssetIndexers() const;

	void CreateFlowToolbar(FToolBarBuilder& ToolbarBuilder) const;

public:
	static TSharedRef<FFlowAssetEditor> CreateFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UFlowAsset* FlowAsset);

	void OnAssetUpdated(const FAssetData& AssetData);
	void OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath);
};