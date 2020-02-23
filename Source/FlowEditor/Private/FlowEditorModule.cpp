#include "FlowEditorModule.h"
#include "FlowEditorStyle.h"
#include "Graph/FlowAssetActions.h"
#include "Graph/FlowAssetEditor.h"
#include "Graph/FlowGraphConnectionDrawingPolicy.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "LevelEditor/SLevelEditorFlow.h"

#include "FlowAsset.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "LevelEditor.h"
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
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MakeShareable(new FFlowGraphConnectionDrawingPolicyFactory));

	// menu extensibility
	FlowAssetExtensibility.Init();

	// add Flow Toolbar
	if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddToolBarExtension("Game", EExtensionHook::After, nullptr, FToolBarExtensionDelegate::CreateRaw(this, &FFlowEditorModule::CreateFlowToolbar));
		LevelEditorModule->GetToolBarExtensibilityManager()->AddExtender(MenuExtender);
	}
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

void FFlowEditorModule::CreateFlowToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection("Flow");
	{
		ToolbarBuilder.AddWidget(SNew(SLevelEditorFlow));
	}
	ToolbarBuilder.EndSection();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFlowEditorModule, FlowEditor);
DEFINE_LOG_CATEGORY(LogFlowEditor);