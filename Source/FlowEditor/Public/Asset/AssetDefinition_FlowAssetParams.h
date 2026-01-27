// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "AssetDefinitionDefault.h"
#include "AssetDefinition_FlowAssetParams.generated.h"

/**
* Asset Definition for Flow Asset Params, providing Content Browser integration.
*/
UCLASS()
class FLOWEDITOR_API UAssetDefinition_FlowAssetParams : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	// UAssetDefinition interface
	virtual FText GetAssetDisplayName() const override;
	virtual FLinearColor GetAssetColor() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	virtual FAssetSupportResponse CanLocalize(const FAssetData& InAsset) const override;
	// --
};