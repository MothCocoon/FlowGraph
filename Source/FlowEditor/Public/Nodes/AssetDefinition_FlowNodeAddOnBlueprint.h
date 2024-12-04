// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Script/AssetDefinition_Blueprint.h"
#include "AssetDefinition_FlowNodeAddOnBlueprint.generated.h"

UCLASS()
class FLOWEDITOR_API UAssetDefinition_FlowNodeAddOnBlueprint : public UAssetDefinition_Blueprint
{
	GENERATED_BODY()
public:
	// UAssetDefinition
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual FLinearColor GetAssetColor() const override { return FColor(255, 196, 128); }
	virtual FText GetAssetDisplayName() const override;
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	// --

protected:
	// FAssetTypeActions_Blueprint
	//virtual bool CanCreateNewDerivedBlueprint() const override { return false; }
	virtual UFactory* GetFactoryForBlueprintType(UBlueprint* InBlueprint) const override;
	// --
};
