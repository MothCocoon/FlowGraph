#include "Asset/AssetTypeActions_FlowAsset.h"
#include "FlowEditorModule.h"

#include "FlowAsset.h"

#include "Toolkits/IToolkit.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_FlowAsset"

FText FAssetTypeActions_FlowAsset::GetName() const
{
	return LOCTEXT("AssetTypeActions_FlowAsset", "Flow Asset");
}

uint32 FAssetTypeActions_FlowAsset::GetCategories()
{
	return FFlowEditorModule::FlowAssetCategory;
}

UClass* FAssetTypeActions_FlowAsset::GetSupportedClass() const
{
	return UFlowAsset::StaticClass();
}

void FAssetTypeActions_FlowAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UFlowAsset* FlowAsset = Cast<UFlowAsset>(*ObjIt))
		{
			FFlowEditorModule* FlowModule = &FModuleManager::LoadModuleChecked<FFlowEditorModule>("FlowEditor");
			FlowModule->CreateFlowAssetEditor(Mode, EditWithinLevelEditor, FlowAsset);
		}
	}
}

#undef LOCTEXT_NAMESPACE
