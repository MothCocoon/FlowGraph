// Fill out your copyright notice in the Description page of Project Settings.
// @tiramisoo - Advanced level sequence handling

#include "Asset/AssetDefinition_FlowLevelSequence.h"
#include "FlowEditorModule.h"

#include "LevelSequence.h"
#include "Asset/FlowAssetEditor.h"
#include "Toolkits/SimpleAssetEditor.h"

#define LOCTEXT_NAMESPACE "AssetDefinition_FlowLevelSequence"

FText UAssetDefinition_FlowLevelSequence::GetAssetDisplayName() const
{
	return LOCTEXT("AssetDefinition_FlowLevelSequence", "Level Sequence");
}

FLinearColor UAssetDefinition_FlowLevelSequence::GetAssetColor() const
{
	return FColor(255, 196, 128);
}

TSoftClassPtr<UObject> UAssetDefinition_FlowLevelSequence::GetAssetClass() const
{
	return ULevelSequence::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_FlowLevelSequence::GetAssetCategories() const
{
	return UAssetDefinition::GetAssetCategories();
}

FAssetSupportResponse UAssetDefinition_FlowLevelSequence::CanLocalize(const FAssetData& InAsset) const
{
	return FAssetSupportResponse::Supported();
}

//@TODO: Add advanced handling for focus on sequencer - see FlowEditorModule.cpp:142
EAssetCommandResult UAssetDefinition_FlowLevelSequence::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	//TSharedRef<FSimpleAssetEditor> Editor = FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);	
	//Editor->FocusWindow();

	return EAssetCommandResult::Handled;
}

EAssetCommandResult UAssetDefinition_FlowLevelSequence::PerformAssetDiff(const FAssetDiffArgs& DiffArgs) const
{
	return UAssetDefinition::PerformAssetDiff(DiffArgs);
}

#undef LOCTEXT_NAMESPACE
