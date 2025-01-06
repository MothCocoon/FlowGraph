// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "AssetTypeActions/AssetTypeActions_Blueprint.h"

class FLOWEDITOR_API FAssetTypeActions_FlowNodeAddOnBlueprint : public FAssetTypeActions_Blueprint
{
public:
	virtual FText GetName() const override;
	virtual uint32 GetCategories() override;
	virtual FColor GetTypeColor() const override { return FColor(255, 196, 128); }

	virtual UClass* GetSupportedClass() const override;

protected:
	// FAssetTypeActions_Blueprint
	virtual bool CanCreateNewDerivedBlueprint() const override { return false; }
	virtual UFactory* GetFactoryForBlueprintType(UBlueprint* InBlueprint) const override;
	// --
};
