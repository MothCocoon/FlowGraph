// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowEditorModule.h"
#include "FlowEditorStyle.h"

#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowAssetIndexer.h"
#include "Graph/FlowGraphConnectionDrawingPolicy.h"
#include "Graph/FlowGraphPinFactory.h"
#include "Graph/FlowGraphSettings.h"
#include "Utils/SLevelEditorFlow.h"
#include "MovieScene/FlowTrackEditor.h"
#include "Nodes/AssetTypeActions_FlowNodeBlueprint.h"
#include "Nodes/AssetTypeActions_FlowNodeAddOnBlueprint.h"
#include "Pins/SFlowInputPinHandle.h"
#include "Pins/SFlowOutputPinHandle.h"

#include "FlowModule.h"

#include "DetailCustomizations/FlowAssetDetails.h"
#include "DetailCustomizations/FlowNode_Details.h"
#include "DetailCustomizations/FlowNode_ComponentObserverDetails.h"
#include "DetailCustomizations/FlowNode_CustomInputDetails.h"
#include "DetailCustomizations/FlowNode_CustomOutputDetails.h"
#include "DetailCustomizations/FlowNode_PlayLevelSequenceDetails.h"
#include "DetailCustomizations/FlowNode_SubGraphDetails.h"
#include "DetailCustomizations/FlowNodeAddOn_Details.h"
#include "DetailCustomizations/FlowActorOwnerComponentRefCustomization.h"
#include "DetailCustomizations/FlowPinCustomization.h"
#include "DetailCustomizations/FlowNamedDataPinPropertyCustomization.h"
#include "DetailCustomizations/FlowAssetParamsPtrCustomization.h"
#include "DetailCustomizations/FlowDataPinValueOwnerCustomizations.h"
#include "DetailCustomizations/FlowDataPinValueStandardCustomizations.h"

#include "FlowAsset.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Asset/FlowAssetParamsTypes.h"
#include "Nodes/Actor/FlowNode_ComponentObserver.h"
#include "Nodes/Actor/FlowNode_PlayLevelSequence.h"
#include "Nodes/Graph/FlowNode_CustomInput.h"
#include "Nodes/Graph/FlowNode_CustomOutput.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"
#include "Types/FlowNamedDataPinProperty.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "IAssetSearchModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISequencerChannelInterface.h" // ignore Rider's false "unused include" warning
#include "ISequencerModule.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"

static FName AssetSearchModuleName = TEXT("AssetSearch");

#define LOCTEXT_NAMESPACE "FlowEditorModule"

EAssetTypeCategories::Type FFlowEditorModule::FlowAssetCategory = static_cast<EAssetTypeCategories::Type>(0);
FAssetCategoryPath FFlowAssetCategoryPaths::Flow(LOCTEXT("Flow", "Flow"));

void FFlowEditorModule::StartupModule()
{
	MenuExtensibilityManager = MakeShared<FExtensibilityManager>();
	ToolBarExtensibilityManager = MakeShared<FExtensibilityManager>();
	
	FFlowEditorStyle::Initialize();

	TrySetFlowNodeDisplayStyleDefaults();

	RegisterAssets();

	// register visual utilities
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MakeShareable(new FFlowGraphConnectionDrawingPolicyFactory));
	FEdGraphUtilities::RegisterVisualPinFactory(MakeShareable(new FFlowGraphPinFactory()));
	FEdGraphUtilities::RegisterVisualPinFactory(MakeShareable(new FFlowInputPinHandleFactory()));
	FEdGraphUtilities::RegisterVisualPinFactory(MakeShareable(new FFlowOutputPinHandleFactory()));

	// add Flow Toolbar
	if (UFlowGraphSettings::Get()->bShowAssetToolbarAboveLevelEditor)
	{
		if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
		{
			const TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
			MenuExtender->AddToolBarExtension("Play", EExtensionHook::After, nullptr, FToolBarExtensionDelegate::CreateRaw(this, &FFlowEditorModule::CreateFlowToolbar));
			LevelEditorModule->GetToolBarExtensibilityManager()->AddExtender(MenuExtender);
		}
	}

	// register Flow sequence track
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	FlowTrackCreateEditorHandle = SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FFlowTrackEditor::CreateTrackEditor));

	RegisterDetailCustomizations();

	// register asset indexers
	if (FModuleManager::Get().IsModuleLoaded(AssetSearchModuleName))
	{
		RegisterAssetIndexers();
	}
	ModulesChangedHandle = FModuleManager::Get().OnModulesChanged().AddRaw(this, &FFlowEditorModule::ModulesChangesCallback);
}

void FFlowEditorModule::ShutdownModule()
{
	MenuExtensibilityManager.Reset();
	ToolBarExtensibilityManager.Reset();
	
	FFlowEditorStyle::Shutdown();

	UnregisterDetailCustomizations();

	UnregisterAssets();

	// unregister track editors
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	SequencerModule.UnRegisterTrackEditor(FlowTrackCreateEditorHandle);

	FModuleManager::Get().OnModulesChanged().Remove(ModulesChangedHandle);
}

void FFlowEditorModule::TrySetFlowNodeDisplayStyleDefaults() const
{
	// Force the Flow module to be loaded before we try to access the Settings
	FModuleManager::LoadModuleChecked<FFlowModule>("Flow");

	UFlowGraphSettings& Settings = *UFlowGraphSettings::Get();
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::Node, FLinearColor(0.0f, 0.581f, 1.0f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::Condition, FLinearColor(1.0f, 0.62f, 0.016f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::Deprecated, FLinearColor(1.0f, 1.0f, 0.0f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::Developer, FLinearColor(0.7f, 0.2f, 1.0f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::InOut, FLinearColor(1.0f, 0.0f, 0.008f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::Latent, FLinearColor(0.0f, 0.770f, 0.375f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::Logic, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::SubGraph, FLinearColor(1.0f, 0.128f, 0.0f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::Terminal, FLinearColor(1.0f, 0.0f, 0.008f, 1.0f)));

	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::AddOn, FLinearColor(0.0f, 0.581f, 1.0f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::AddOn_PerSpawnedActor, FLinearColor(0.3f, 0.3f, 1.0f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::AddOn_Predicate, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	(void) Settings.TryAddDefaultNodeDisplayStyle(FFlowNodeDisplayStyleConfig(FlowNodeStyle::AddOn_Predicate_Composite, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
}

void FFlowEditorModule::RegisterAssets()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// try to merge asset category with a built-in one
	{
		const FText AssetCategoryText = UFlowGraphSettings::Get()->FlowAssetCategoryName;

		// Find matching built-in category
		if (!AssetCategoryText.IsEmpty())
		{
			TArray<FAdvancedAssetCategory> AllCategories;
			AssetTools.GetAllAdvancedAssetCategories(AllCategories);
			for (const FAdvancedAssetCategory& ExistingCategory : AllCategories)
			{
				if (ExistingCategory.CategoryName.EqualTo(AssetCategoryText))
				{
					FlowAssetCategory = ExistingCategory.CategoryType;
					break;
				}
			}
		}

		if (FlowAssetCategory == EAssetTypeCategories::None)
		{
			FlowAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Flow")), AssetCategoryText);
		}
	}

	const TSharedRef<IAssetTypeActions> FlowNodeActions = MakeShareable(new FAssetTypeActions_FlowNodeBlueprint());
	RegisteredAssetActions.Add(FlowNodeActions);
	AssetTools.RegisterAssetTypeActions(FlowNodeActions);

	const TSharedRef<IAssetTypeActions> FlowNodeAddOnActions = MakeShareable(new FAssetTypeActions_FlowNodeAddOnBlueprint());
	RegisteredAssetActions.Add(FlowNodeAddOnActions);
	AssetTools.RegisterAssetTypeActions(FlowNodeAddOnActions);
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
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(Class->GetFName(), DetailLayout);

		CustomClassLayouts.Add(Class->GetFName());
	}
}

void FFlowEditorModule::RegisterCustomStructLayout(const UScriptStruct& Struct, const FOnGetPropertyTypeCustomizationInstance DetailLayout)
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyTypeLayout(Struct.GetFName(), DetailLayout);

		CustomStructLayouts.Add(Struct.GetFName());
	}
}

void FFlowEditorModule::RegisterDetailCustomizations()
{
	// register detail customizations
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		RegisterCustomClassLayout(UFlowAsset::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowAssetDetails::MakeInstance));
		RegisterCustomClassLayout(UFlowAssetParams::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowAssetParamsCustomization::MakeInstance));
		RegisterCustomClassLayout(UFlowExecutableActorComponent::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowExecutableActorComponentCustomization::MakeInstance));
		RegisterCustomClassLayout(UFlowNodeBase::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNodeBaseCustomization::MakeInstance));
		RegisterCustomClassLayout(UFlowNode::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_Details::MakeInstance));
		RegisterCustomClassLayout(UFlowNodeAddOn::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNodeAddOn_Details::MakeInstance));
		RegisterCustomClassLayout(UFlowNode_ComponentObserver::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_ComponentObserverDetails::MakeInstance));
		RegisterCustomClassLayout(UFlowNode_CustomInput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_CustomInputDetails::MakeInstance));
		RegisterCustomClassLayout(UFlowNode_CustomOutput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_CustomOutputDetails::MakeInstance));
		RegisterCustomClassLayout(UFlowNode_PlayLevelSequence::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_PlayLevelSequenceDetails::MakeInstance));
	    RegisterCustomClassLayout(UFlowNode_SubGraph::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_SubGraphDetails::MakeInstance));
		RegisterCustomStructLayout(*FFlowActorOwnerComponentRef::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowActorOwnerComponentRefCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowPin::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowPinCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowNamedDataPinProperty::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowNamedDataPinPropertyCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowAssetParamsPtr::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowAssetParamsPtrCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Bool::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Bool::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Int::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Int::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Int64::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Int64::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Float::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Float::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Double::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Double::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Name::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Name::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_String::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_String::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Text::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Text::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Enum::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Enum::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Vector::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Vector::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Rotator::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Rotator::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Transform::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Transform::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_GameplayTag::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_GameplayTag::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_GameplayTagContainer::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_GameplayTagContainer::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_InstancedStruct::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_InstancedStruct::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Class::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Class::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinValue_Object::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinValueCustomization_Object::MakeInstance));

		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FFlowEditorModule::UnregisterDetailCustomizations()
{
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

		for (auto It = CustomStructLayouts.CreateConstIterator(); It; ++It)
		{
			if (It->IsValid())
			{
				PropertyModule.UnregisterCustomPropertyTypeLayout(*It);
			}
		}

		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FFlowEditorModule::ModulesChangesCallback(const FName ModuleName, const EModuleChangeReason ReasonForChange) const
{
	if (ReasonForChange == EModuleChangeReason::ModuleLoaded && ModuleName == AssetSearchModuleName)
	{
		RegisterAssetIndexers();
	}
}

void FFlowEditorModule::RegisterAssetIndexers() const
{
	IAssetSearchModule::Get().RegisterAssetIndexer(UFlowAsset::StaticClass(), MakeUnique<FFlowAssetIndexer>());
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