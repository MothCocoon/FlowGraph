#include "AssetTypeActions_FlowAsset.h"
#include "../FlowEditorModule.h"

#include "Flow/Graph/FlowAsset.h"

#include "ContentBrowserModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IContentBrowserSingleton.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_FlowAsset::GetSupportedClass() const
{
	return UFlowAsset::StaticClass();
}

uint32 FAssetTypeActions_FlowAsset::GetCategories()
{
	return FlowAssetCategory;
}

void FAssetTypeActions_FlowAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

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
