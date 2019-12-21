#include "FlowEditorModule.h"
#include "Graph/FlowAssetActions.h"
#include "Graph/FlowAssetEditor.h"
#include "Graph/FlowGraphConnectionDrawingPolicy.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Graph/Widgets/FlowGraphPanelNodeFactory.h"

#include "Graph/FlowAsset.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Utils.h"


EAssetTypeCategories::Type FFlowEditorModule::FlowAssetCategory = (EAssetTypeCategories::Type)0;
FString FFlowEditorModule::FlowContentDir = IPluginManager::Get().FindPlugin(TEXT("Flow"))->GetContentDir();

// Setup icon sizes
static const FVector2D Icon16 = FVector2D(16.0f, 16.0f);
static const FVector2D Icon64 = FVector2D(64.0f, 64.0f);

// Preprocessor macro to make defining flow icons simple...

// CLASS_NAME - name of the class to make the icon for
// ICON_NAME - base-name of the icon to use. Not necessarily based off class name
#define SET_FLOW_ICON(CLASS_NAME, ICON_NAME) \
		FlowStyleSet->Set( *FString::Printf(TEXT("ClassIcon.%s"), TEXT(#CLASS_NAME)), new FSlateImageBrush(FlowContentDir / FString::Printf(TEXT("Icons/AssetIcons/%s_16x.png"), TEXT(#ICON_NAME)), Icon16)); \
		FlowStyleSet->Set( *FString::Printf(TEXT("ClassThumbnail.%s"), TEXT(#CLASS_NAME)), new FSlateImageBrush(FlowContentDir / FString::Printf(TEXT("Icons/AssetIcons/%s_64x.png"), TEXT(#ICON_NAME)), Icon64)); 

// Simpler version of SET_FLOW_ICON, assumes same name of icon png and class name
#define SET_FLOW_ICON_SIMPLE(CLASS_NAME) SET_FLOW_ICON(CLASS_NAME, CLASS_NAME)

#define LOCTEXT_NAMESPACE "FlowEditor"

void FFlowEditorModule::StartupModule()
{
	// register assets
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FlowAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Flow")), LOCTEXT("FlowAssetCategory", "Flow"));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FlowAssetActions));

	// register visual utilities
	FEdGraphUtilities::RegisterVisualNodeFactory(MakeShareable(new FFlowGraphPanelNodeFactory()));
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MakeShareable(new FFlowGraphConnectionDrawingPolicyFactory));

	// menu extensibility
	FlowAssetExtensibility.Init();

	// create style set
	FlowStyleSet = MakeShareable(new FSlateStyleSet("FlowStyleSet"));
	FlowStyleSet->SetContentRoot(FlowContentDir);

	SetupIcons();
}

void FFlowEditorModule::ShutdownModule()
{
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

void FFlowEditorModule::SetupIcons()
{
	SET_FLOW_ICON_SIMPLE(FlowAsset);

	FSlateStyleRegistry::RegisterSlateStyle(*FlowStyleSet.Get());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFlowEditorModule, FlowEditor);
DEFINE_LOG_CATEGORY(LogFlowEditor);
