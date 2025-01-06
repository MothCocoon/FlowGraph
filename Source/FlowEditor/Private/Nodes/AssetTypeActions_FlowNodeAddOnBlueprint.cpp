// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/AssetTypeActions_FlowNodeAddOnBlueprint.h"
#include "Nodes/FlowNodeBlueprintFactory.h"
#include "Nodes/FlowNodeAddOnBlueprint.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Graph/FlowGraphSettings.h"
#include "FlowEditorModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_FlowNodeAddOnBlueprint"

FText FAssetTypeActions_FlowNodeAddOnBlueprint::GetName() const
{
	return LOCTEXT("AssetTypeActions_FlowNodeBlueprint", "Flow Node AddOn Blueprint");
}

uint32 FAssetTypeActions_FlowNodeAddOnBlueprint::GetCategories()
{
	return UFlowGraphSettings::Get()->bExposeFlowNodeCreation ? FFlowEditorModule::FlowAssetCategory : 0;
}

UClass* FAssetTypeActions_FlowNodeAddOnBlueprint::GetSupportedClass() const
{
	return UFlowNodeAddOnBlueprint::StaticClass();
}

UFactory* FAssetTypeActions_FlowNodeAddOnBlueprint::GetFactoryForBlueprintType(UBlueprint* InBlueprint) const
{
	UFlowNodeAddOnBlueprintFactory* FlowNodeAddOnBlueprintFactory = NewObject<UFlowNodeAddOnBlueprintFactory>();
	FlowNodeAddOnBlueprintFactory->ParentClass = TSubclassOf<UFlowNodeAddOn>(*InBlueprint->GeneratedClass);
	return FlowNodeAddOnBlueprintFactory;
}

#undef LOCTEXT_NAMESPACE
