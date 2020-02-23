#include "FlowAssetActions.h"
#include "FlowEditorModule.h"

#include "FlowAsset.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FlowAssetActions::GetSupportedClass() const
{
	return UFlowAsset::StaticClass();
}

uint32 FlowAssetActions::GetCategories()
{
	return FFlowEditorModule::FlowAssetCategory;
}

void FlowAssetActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
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