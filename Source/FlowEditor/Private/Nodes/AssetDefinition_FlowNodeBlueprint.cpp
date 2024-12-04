// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/AssetDefinition_FlowNodeBlueprint.h"
#include "Nodes/FlowNodeBlueprintFactory.h"
#include "Nodes/FlowNodeBlueprint.h"
#include "Nodes/FlowNode.h"
#include "Graph/FlowGraphSettings.h"
#include "FlowEditorModule.h"


#define LOCTEXT_NAMESPACE "AssetTypeActions_FlowNodeBlueprint"

TSoftClassPtr<UObject> UAssetDefinition_FlowNodeBlueprint::GetAssetClass() const
{
	return UFlowNodeBlueprint::StaticClass();
}

FText UAssetDefinition_FlowNodeBlueprint::GetAssetDisplayName() const
{
	return LOCTEXT("AssetTypeActions_FlowNodeBlueprint", "Flow Node Blueprint");
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_FlowNodeBlueprint::GetAssetCategories() const
{
	if (UFlowGraphSettings::Get()->bExposeFlowAssetCreation)
	{
		static const auto Categories = {FFLowAssetCategoryPaths::Flow};
		return Categories;
	}

	return {};
}

UFactory* UAssetDefinition_FlowNodeBlueprint::GetFactoryForBlueprintType(UBlueprint* InBlueprint) const
{
	UFlowNodeBlueprintFactory* FlowNodeBlueprintFactory = NewObject<UFlowNodeBlueprintFactory>();
	FlowNodeBlueprintFactory->ParentClass = TSubclassOf<UFlowNode>(*InBlueprint->GeneratedClass);
	return FlowNodeBlueprintFactory;
}

#undef LOCTEXT_NAMESPACE
