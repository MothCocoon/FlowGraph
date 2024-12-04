// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Script/AssetDefinition_Blueprint.h"
#include "AssetDefinition_FlowNodeBlueprint.generated.h"

UCLASS()
class FLOWEDITOR_API UAssetDefinition_FlowNodeBlueprint : public UAssetDefinition_Blueprint
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
	// UAssetDefinition_Blueprint
	virtual UFactory* GetFactoryForBlueprintType(UBlueprint* InBlueprint) const override;
	// --
};
