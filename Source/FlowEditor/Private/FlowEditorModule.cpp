#include "FlowEditorModule.h"
#include "FlowEditorStyle.h"

#include "Asset/AssetTypeActions_FlowAsset.h"
#include "Asset/FlowAssetDetails.h"
#include "Asset/FlowAssetEditor.h"
#include "Graph/Customizations/FlowNode_Details.h"
#include "Graph/Customizations/FlowNode_ComponentObserverDetails.h"
#include "Graph/Customizations/FlowNode_CustomInputDetails.h"
#include "Graph/Customizations/FlowNode_CustomOutputDetails.h"
#include "Graph/Customizations/FlowNode_PlayLevelSequenceDetails.h"
#include "Graph/FlowGraphConnectionDrawingPolicy.h"
#include "LevelEditor/SLevelEditorFlow.h"
#include "MovieScene/FlowTrackEditor.h"
#include "Nodes/AssetTypeActions_FlowNodeBlueprint.h"

#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_CustomInput.h"
#include "Nodes/Route/FlowNode_CustomOutput.h"
#include "Nodes/World/FlowNode_ComponentObserver.h"
#include "Nodes/World/FlowNode_PlayLevelSequence.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "Framework/Multibox/MultiBoxBuilder.h"
#include "ISequencerChannelInterface.h"
#include "ISequencerModule.h"
#include "LevelEditor.h"
#include "MultiBoxBuilder.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FlowEditor"

EAssetTypeCategories::Type FFlowEditorModule::FlowAssetCategory = static_cast<EAssetTypeCategories::Type>(0);

void FFlowEditorModule::StartupModule()
{
	FFlowEditorStyle::Initialize();

	RegisterAssets();

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

	// register Flow sequence track
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	FlowTrackCreateEditorHandle = SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FFlowTrackEditor::CreateTrackEditor));

	// register detail customizations
	RegisterCustomClassLayout(UFlowAsset::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowAssetDetails::MakeInstance));
	RegisterCustomClassLayout(UFlowNode::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_Details::MakeInstance));
	RegisterCustomClassLayout(UFlowNode_ComponentObserver::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_ComponentObserverDetails::MakeInstance));
	RegisterCustomClassLayout(UFlowNode_CustomInput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_CustomInputDetails::MakeInstance));
	RegisterCustomClassLayout(UFlowNode_CustomOutput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_CustomOutputDetails::MakeInstance));
	RegisterCustomClassLayout(UFlowNode_PlayLevelSequence::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_PlayLevelSequenceDetails::MakeInstance));

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FFlowEditorModule::ShutdownModule()
{
	FFlowEditorStyle::Shutdown();

	UnregisterAssets();

	// unregister visual utilities
	if (FlowGraphConnectionFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinConnectionFactory(FlowGraphConnectionFactory);
	}

	// reset menu extensibility
	FlowAssetExtensibility.Reset();

	// unregister track editors
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	SequencerModule.UnRegisterTrackEditor(FlowTrackCreateEditorHandle);

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

void FFlowEditorModule::RegisterAssets()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FlowAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Flow")), LOCTEXT("FlowAssetCategory", "Flow"));

	const TSharedRef<IAssetTypeActions> FlowAssetActions = MakeShareable(new FAssetTypeActions_FlowAsset());
	RegisteredAssetActions.Add(FlowAssetActions);
	AssetTools.RegisterAssetTypeActions(FlowAssetActions);

	const TSharedRef<IAssetTypeActions> FlowNodeActions = MakeShareable(new FAssetTypeActions_FlowNodeBlueprint());
	RegisteredAssetActions.Add(FlowNodeActions);
	AssetTools.RegisterAssetTypeActions(FlowNodeActions);
}

void FFlowEditorModule::UnregisterAssets()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (const TSharedRef<IAssetTypeActions>& TypeAction : RegisteredAssetActions)
		{
			AssetTools.UnregisterAssetTypeActions(TypeAction);
		}
	}

	RegisteredAssetActions.Empty();
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

TSharedRef<FFlowAssetEditor> FFlowEditorModule::CreateFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UFlowAsset* FlowAsset)
{
	TSharedRef<FFlowAssetEditor> NewFlowAssetEditor(new FFlowAssetEditor());
	NewFlowAssetEditor->InitFlowAssetEditor(Mode, InitToolkitHost, FlowAsset);
	return NewFlowAssetEditor;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFlowEditorModule, FlowEditor)
DEFINE_LOG_CATEGORY(LogFlowEditor);
