// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowEditorModule.h"
#include "FlowEditorStyle.h"

#include "Asset/AssetTypeActions_FlowAsset.h"
#include "Asset/FlowAssetDetails.h"
#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowAssetIndexer.h"
#include "Graph/FlowGraphConnectionDrawingPolicy.h"
#include "Graph/FlowGraphSettings.h"
#include "LevelEditor/SLevelEditorFlow.h"
#include "MovieScene/FlowTrackEditor.h"
#include "Nodes/AssetTypeActions_FlowNodeBlueprint.h"
#include "Nodes/Customizations/FlowNode_Details.h"
#include "Nodes/Customizations/FlowNode_ComponentObserverDetails.h"
#include "Nodes/Customizations/FlowNode_CustomInputDetails.h"
#include "Nodes/Customizations/FlowNode_CustomOutputDetails.h"
#include "Nodes/Customizations/FlowNode_PlayLevelSequenceDetails.h"

#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_CustomInput.h"
#include "Nodes/Route/FlowNode_CustomOutput.h"
#include "Nodes/World/FlowNode_ComponentObserver.h"
#include "Nodes/World/FlowNode_PlayLevelSequence.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "IAssetSearchModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISequencerChannelInterface.h"
#include "ISequencerModule.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"

static FName AssetSearchModuleName = TEXT("AssetSearch");

#define LOCTEXT_NAMESPACE "FlowEditorModule"

EAssetTypeCategories::Type FFlowEditorModule::FlowAssetCategory = static_cast<EAssetTypeCategories::Type>(0);

void FFlowEditorModule::StartupModule()
{
	FFlowEditorStyle::Initialize();

	RegisterAssets();

	// register visual utilities
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MakeShareable(new FFlowGraphConnectionDrawingPolicyFactory));

	// add Flow Toolbar
	if (UFlowGraphSettings::Get()->bShowAssetToolbarAboveLevelEditor)
	{
		if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
		{
			const TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
			MenuExtender->AddToolBarExtension("Game", EExtensionHook::After, nullptr, FToolBarExtensionDelegate::CreateRaw(this, &FFlowEditorModule::CreateFlowToolbar));
			LevelEditorModule->GetToolBarExtensibilityManager()->AddExtender(MenuExtender);
		}
	}

	// register Flow sequence track
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	FlowTrackCreateEditorHandle = SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FFlowTrackEditor::CreateTrackEditor));

	RegisterPropertyCustomizations();

	// register detail customizations
	RegisterCustomClassLayout(UFlowAsset::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowAssetDetails::MakeInstance));
	RegisterCustomClassLayout(UFlowNode::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_Details::MakeInstance));
	RegisterCustomClassLayout(UFlowNode_ComponentObserver::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_ComponentObserverDetails::MakeInstance));
	RegisterCustomClassLayout(UFlowNode_CustomInput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_CustomInputDetails::MakeInstance));
	RegisterCustomClassLayout(UFlowNode_CustomOutput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_CustomOutputDetails::MakeInstance));
	RegisterCustomClassLayout(UFlowNode_PlayLevelSequence::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_PlayLevelSequenceDetails::MakeInstance));

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.NotifyCustomizationModuleChanged();

	// register asset indexers
	if (FModuleManager::Get().IsModuleLoaded(AssetSearchModuleName))
	{
		RegisterAssetIndexers();
	}
	ModulesChangedHandle = FModuleManager::Get().OnModulesChanged().AddRaw(this, &FFlowEditorModule::ModulesChangesCallback);
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
	
	FModuleManager::Get().OnModulesChanged().Remove(ModulesChangedHandle);
}

void FFlowEditorModule::RegisterAssets()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FlowAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Flow")), UFlowGraphSettings::Get()->FlowAssetCategoryName);

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

void FFlowEditorModule::RegisterPropertyCustomizations() const
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// notify on customization change
	PropertyModule.NotifyCustomizationModuleChanged();
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

void FFlowEditorModule::ModulesChangesCallback(FName ModuleName, EModuleChangeReason ReasonForChange)
{
	if (ReasonForChange == EModuleChangeReason::ModuleLoaded && ModuleName == AssetSearchModuleName)
	{
		RegisterAssetIndexers();
	}
}

void FFlowEditorModule::RegisterAssetIndexers() const
{
	/**
	 * Documentation: https://github.com/MothCocoon/FlowGraph/wiki/Asset-Search
	 * Uncomment line below, if you made these changes to the engine: https://github.com/EpicGames/UnrealEngine/pull/9070
	 */
	//IAssetSearchModule::Get().RegisterAssetIndexer(UFlowAsset::StaticClass(), MakeUnique<FFlowAssetIndexer>());
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
