#include "FlowEditorModule.h"

#include "FlowEditorStyle.h"
#include "Graph/Customizations/FlowNode_CustomEventCustomization.h"
#include "Graph/Customizations/FlowNode_CustomOutputCustomization.h"
#include "Graph/FlowAssetActions.h"
#include "Graph/FlowAssetEditor.h"
#include "Graph/FlowGraphConnectionDrawingPolicy.h"
#include "LevelEditor/SLevelEditorFlow.h"

#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_CustomEvent.h"
#include "Nodes/Route/FlowNode_CustomOutput.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FlowEditor"

EAssetTypeCategories::Type FFlowEditorModule::FlowAssetCategory = static_cast<EAssetTypeCategories::Type>(0);

void FFlowEditorModule::StartupModule()
{
	FFlowEditorStyle::Initialize();

	// register assets
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FlowAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Flow")), LOCTEXT("FlowAssetCategory", "Flow"));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FlowAssetActions));

	// register visual utilities
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MakeShareable(new FFlowGraphConnectionDrawingPolicyFactory));

	// init menu extensibility
	FlowAssetExtensibility.Init();

	// add Flow Toolbar
	if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddToolBarExtension("Game", EExtensionHook::After, nullptr, FToolBarExtensionDelegate::CreateRaw(this, &FFlowEditorModule::CreateFlowToolbar));
		LevelEditorModule->GetToolBarExtensibilityManager()->AddExtender(MenuExtender);
	}

	// register detail customizations
	RegisterCustomClassLayout(UFlowNode_CustomEvent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_CustomEventCustomization::MakeInstance));
	RegisterCustomClassLayout(UFlowNode_CustomOutput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_CustomOutputCustomization::MakeInstance));

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FFlowEditorModule::ShutdownModule()
{
	FFlowEditorStyle::Shutdown();

	// unregister visual utilities
	if (FlowGraphConnectionFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinConnectionFactory(FlowGraphConnectionFactory);
	}

	// reset menu extensibility
	FlowAssetExtensibility.Reset();

	// unregister details customizations
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		
		for (auto It = CustomClassLayouts.CreateConstIterator(); It; ++It)
		{
			if (It->IsValid())
			{
				PropertyModule.UnregisterCustomClassLayout(*It);
			}
		}
	}
}

void FFlowEditorModule::RegisterCustomClassLayout(const TSubclassOf<UObject> Class, const FOnGetDetailCustomizationInstance DetailLayout)
{
	if (Class)
	{
		CustomClassLayouts.Add(Class->GetFName());

		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(Class->GetFName(), DetailLayout);
	}
}

TSharedPtr<FExtensibilityManager> FFlowEditorModule::GetFlowAssetMenuExtensibilityManager() const
{
	return FlowAssetExtensibility.MenuExtensibilityManager;
}

TSharedPtr<FExtensibilityManager> FFlowEditorModule::GetFlowAssetToolBarExtensibilityManager() const
{
	return FlowAssetExtensibility.ToolBarExtensibilityManager;
}

void FFlowEditorModule::CreateFlowToolbar(FToolBarBuilder& ToolbarBuilder) const
{
	ToolbarBuilder.BeginSection("Flow");
	{
		ToolbarBuilder.AddWidget(SNew(SLevelEditorFlow));
	}
	ToolbarBuilder.EndSection();
}

TSharedRef<FFlowAssetEditor> FFlowEditorModule::CreateFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UFlowAsset* FlowAsset)
{
	TSharedRef<FFlowAssetEditor> NewFlowAssetEditor(new FFlowAssetEditor());
	NewFlowAssetEditor->InitFlowAssetEditor(Mode, InitToolkitHost, FlowAsset);
	return NewFlowAssetEditor;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFlowEditorModule, FlowEditor)
DEFINE_LOG_CATEGORY(LogFlowEditor);