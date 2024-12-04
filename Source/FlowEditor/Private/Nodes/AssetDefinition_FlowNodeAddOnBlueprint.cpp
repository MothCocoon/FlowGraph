// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/AssetDefinition_FlowNodeAddOnBlueprint.h"
#include "Nodes/FlowNodeBlueprintFactory.h"
#include "Nodes/FlowNodeAddOnBlueprint.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Graph/FlowGraphSettings.h"
#include "FlowEditorModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_FlowNodeAddOnBlueprint"

TSoftClassPtr<UObject> UAssetDefinition_FlowNodeAddOnBlueprint::GetAssetClass() const
{
	return UFlowNodeAddOnBlueprint::StaticClass();
}

FText UAssetDefinition_FlowNodeAddOnBlueprint::GetAssetDisplayName() const
{
	return LOCTEXT("AssetTypeActions_FlowNodeBlueprint", "Flow Node AddOn Blueprint");
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_FlowNodeAddOnBlueprint::GetAssetCategories() const
{
	if (UFlowGraphSettings::Get()->bExposeFlowAssetCreation)
	{
		static const auto Categories = {FFLowAssetCategoryPaths::Flow};
		return Categories;
	}

	return {};
}

UFactory* UAssetDefinition_FlowNodeAddOnBlueprint::GetFactoryForBlueprintType(UBlueprint* InBlueprint) const
{
	UFlowNodeAddOnBlueprintFactory* FlowNodeAddOnBlueprintFactory = NewObject<UFlowNodeAddOnBlueprintFactory>();
	FlowNodeAddOnBlueprintFactory->ParentClass = TSubclassOf<UFlowNodeAddOn>(*InBlueprint->GeneratedClass);
	return FlowNodeAddOnBlueprintFactory;
}

#undef LOCTEXT_NAMESPACE
