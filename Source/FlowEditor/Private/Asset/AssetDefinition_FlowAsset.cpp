// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/AssetDefinition_FlowAsset.h"
#include "Asset/SFlowDiff.h"
#include "FlowEditorModule.h"
#include "Graph/FlowGraphSettings.h"

#include "FlowAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AssetDefinition_FlowAsset)

#define LOCTEXT_NAMESPACE "AssetDefinition_FlowAsset"

FText UAssetDefinition_FlowAsset::GetAssetDisplayName() const
{
	return LOCTEXT("AssetTypeActions_FlowAsset", "Flow Asset");
}

FLinearColor UAssetDefinition_FlowAsset::GetAssetColor() const
{
	return FColor(255, 196, 128);
}

TSoftClassPtr<UObject> UAssetDefinition_FlowAsset::GetAssetClass() const
{
	return UFlowAsset::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_FlowAsset::GetAssetCategories() const
{
	if (UFlowGraphSettings::Get()->bExposeFlowAssetCreation)
	{
		static const auto Categories = {FFLowAssetCategoryPaths::Flow};
		return Categories;
	}

	return {};
}

FAssetSupportResponse UAssetDefinition_FlowAsset::CanLocalize(const FAssetData& InAsset) const
{
	return FAssetSupportResponse::Supported();
}

EAssetCommandResult UAssetDefinition_FlowAsset::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UFlowAsset* FlowAsset : OpenArgs.LoadObjects<UFlowAsset>())
	{
		const FFlowEditorModule* FlowModule = &FModuleManager::LoadModuleChecked<FFlowEditorModule>("FlowEditor");
		FlowModule->CreateFlowAssetEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, FlowAsset);
	}

	return EAssetCommandResult::Handled;
}

EAssetCommandResult UAssetDefinition_FlowAsset::PerformAssetDiff(const FAssetDiffArgs& DiffArgs) const
{
	if (DiffArgs.OldAsset == nullptr && DiffArgs.NewAsset == nullptr)
	{
		return EAssetCommandResult::Unhandled;
	}

	const UFlowAsset* OldFlow = CastChecked<UFlowAsset>(DiffArgs.OldAsset);
	const UFlowAsset* NewFlow = CastChecked<UFlowAsset>(DiffArgs.NewAsset);

	// sometimes we're comparing different revisions of one single asset (other 
	// times we're comparing two completely separate assets altogether)
	const bool bIsSingleAsset = (OldFlow->GetName() == NewFlow->GetName());

	static const FText BasicWindowTitle = LOCTEXT("FlowAssetDiff", "FlowAsset Diff");
	const FText WindowTitle = !bIsSingleAsset ? BasicWindowTitle : FText::Format(LOCTEXT("FlowAsset Diff", "{0} - FlowAsset Diff"), FText::FromString(NewFlow->GetName()));

	SFlowDiff::CreateDiffWindow(WindowTitle, OldFlow, NewFlow, DiffArgs.OldRevision, DiffArgs.NewRevision);
	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE
