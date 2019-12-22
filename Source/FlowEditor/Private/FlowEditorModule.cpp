#include "FlowEditorModule.h"
#include "FlowEditorStyle.h"
#include "Graph/FlowAssetActions.h"
#include "Graph/FlowAssetEditor.h"
#include "Graph/FlowGraphConnectionDrawingPolicy.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Graph/Widgets/FlowGraphPanelNodeFactory.h"

#include "Graph/FlowAsset.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "Modules/ModuleManager.h"
#include "Utils.h"

#define LOCTEXT_NAMESPACE "FlowEditor"

EAssetTypeCategories::Type FFlowEditorModule::FlowAssetCategory = (EAssetTypeCategories::Type)0;

void FFlowEditorModule::StartupModule()
{
	FFlowEditorStyle::Initialize();

	// register assets
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FlowAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Flow")), LOCTEXT("FlowAssetCategory", "Flow"));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FlowAssetActions));

	// register visual utilities
	FEdGraphUtilities::RegisterVisualNodeFactory(MakeShareable(new FFlowGraphPanelNodeFactory()));
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MakeShareable(new FFlowGraphConnectionDrawingPolicyFactory));

	// menu extensibility
	FlowAssetExtensibility.Init();
}

void FFlowEditorModule::ShutdownModule()
{
	FFlowEditorStyle::Shutdown();

	FlowAssetExtensibility.Reset();

	if (FlowGraphConnectionFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinConnectionFactory(FlowGraphConnectionFactory);
	}
}

TSharedRef<FFlowAssetEditor> FFlowEditorModule::CreateFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UFlowAsset* FlowAsset)
{
	TSharedRef<FFlowAssetEditor> NewFlowAssetEditor(new FFlowAssetEditor());
	NewFlowAssetEditor->InitFlowAssetEditor(Mode, InitToolkitHost, FlowAsset);
	return NewFlowAssetEditor;
}

TSharedPtr<FExtensibilityManager> FFlowEditorModule::GetFlowAssetMenuExtensibilityManager()
{
	return FlowAssetExtensibility.MenuExtensibilityManager;
}

TSharedPtr<FExtensibilityManager> FFlowEditorModule::GetFlowAssetToolBarExtensibilityManager()
{
	return FlowAssetExtensibility.ToolBarExtensibilityManager;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFlowEditorModule, FlowEditor);
DEFINE_LOG_CATEGORY(LogFlowEditor);