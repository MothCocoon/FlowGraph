// Fill out your copyright notice in the Description page of Project Settings.
// @tiramisoo - Advanced level sequence handling

#pragma once

#include "AssetDefinition.h"

/**
 * 
 */

class FLOWEDITOR_API UAssetDefinition_FlowLevelSequence : public UAssetDefinition
{
public:
	virtual FText GetAssetDisplayName() const override;
	virtual FLinearColor GetAssetColor() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	virtual FAssetSupportResponse CanLocalize(const FAssetData& InAsset) const override;

	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	virtual EAssetCommandResult PerformAssetDiff(const FAssetDiffArgs& DiffArgs) const override;
};