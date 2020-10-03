#include "Nodes/AssetTypeActions_FlowNodeBlueprint.h"
#include "Nodes/FlowNodeBlueprintFactory.h"
#include "FlowEditorModule.h"

#include "Nodes/FlowNodeBlueprint.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_FlowNodeBlueprint"

FText FAssetTypeActions_FlowNodeBlueprint::GetName() const
{
	return LOCTEXT("AssetTypeActions_FlowNodeBlueprint", "Flow Node Blueprint");
}

uint32 FAssetTypeActions_FlowNodeBlueprint::GetCategories()
{
	return FFlowEditorModule::FlowAssetCategory;
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
