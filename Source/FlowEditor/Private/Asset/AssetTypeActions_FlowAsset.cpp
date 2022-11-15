// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/AssetTypeActions_FlowAsset.h"
#include "Asset/SFlowDiff.h"
#include "FlowEditorModule.h"
#include "Graph/FlowGraphSettings.h"

#include "FlowAsset.h"

#include "Toolkits/IToolkit.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_FlowAsset"

FText FAssetTypeActions_FlowAsset::GetName() const
{
	return LOCTEXT("AssetTypeActions_FlowAsset", "Flow Asset");
}

uint32 FAssetTypeActions_FlowAsset::GetCategories()
{
	return UFlowGraphSettings::Get()->bExposeFlowAssetCreation ? FFlowEditorModule::FlowAssetCategory : 0;
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
			const FFlowEditorModule* FlowModule = &FModuleManager::LoadModuleChecked<FFlowEditorModule>("FlowEditor");
			FlowModule->CreateFlowAssetEditor(Mode, EditWithinLevelEditor, FlowAsset);
		}
	}
}

void FAssetTypeActions_FlowAsset::PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision) const
{
	const UFlowAsset* OldFlow = CastChecked<UFlowAsset>(OldAsset);
	const UFlowAsset* NewFlow = CastChecked<UFlowAsset>(NewAsset);

	// sometimes we're comparing different revisions of one single asset (other 
	// times we're comparing two completely separate assets altogether)
	const bool bIsSingleAsset = (OldFlow->GetName() == NewFlow->GetName());

	static const FText BasicWindowTitle = LOCTEXT("FlowAssetDiff", "FlowAsset Diff");
	const FText WindowTitle = !bIsSingleAsset ? BasicWindowTitle : FText::Format(LOCTEXT("FlowAsset Diff", "{0} - FlowAsset Diff"), FText::FromString(NewFlow->GetName()));

	SFlowDiff::CreateDiffWindow(WindowTitle, OldFlow, NewFlow, OldRevision, NewRevision);
}

#undef LOCTEXT_NAMESPACE
