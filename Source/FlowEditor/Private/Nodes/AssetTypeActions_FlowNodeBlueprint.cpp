// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/AssetTypeActions_FlowNodeBlueprint.h"
#include "Nodes/FlowNodeBlueprintFactory.h"
#include "Nodes/FlowNodeBlueprint.h"
#include "Nodes/FlowNode.h"
#include "Graph/FlowGraphSettings.h"
#include "FlowEditorModule.h"


#define LOCTEXT_NAMESPACE "AssetTypeActions_FlowNodeBlueprint"

FText FAssetTypeActions_FlowNodeBlueprint::GetName() const
{
	return LOCTEXT("AssetTypeActions_FlowNodeBlueprint", "Flow Node Blueprint");
}

uint32 FAssetTypeActions_FlowNodeBlueprint::GetCategories()
{
	return UFlowGraphSettings::Get()->bExposeFlowNodeCreation ? FFlowEditorModule::FlowAssetCategory : 0;
}

UClass* FAssetTypeActions_FlowNodeBlueprint::GetSupportedClass() const
{
	return UFlowNodeBlueprint::StaticClass();
}

UFactory* FAssetTypeActions_FlowNodeBlueprint::GetFactoryForBlueprintType(UBlueprint* InBlueprint) const
{
	UFlowNodeBlueprintFactory* FlowNodeBlueprintFactory = NewObject<UFlowNodeBlueprintFactory>();
	FlowNodeBlueprintFactory->ParentClass = TSubclassOf<UFlowNode>(*InBlueprint->GeneratedClass);
	return FlowNodeBlueprintFactory;
}

#undef LOCTEXT_NAMESPACE
