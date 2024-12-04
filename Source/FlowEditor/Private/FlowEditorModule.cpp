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
#include "Pins/SFlowInputPinHandle.h"
#include "Pins/SFlowOutputPinHandle.h"

#include "DetailCustomizations/FlowAssetDetails.h"
#include "DetailCustomizations/FlowNode_Details.h"
#include "DetailCustomizations/FlowNode_ComponentObserverDetails.h"
#include "DetailCustomizations/FlowNode_CustomInputDetails.h"
#include "DetailCustomizations/FlowNode_CustomOutputDetails.h"
#include "DetailCustomizations/FlowNode_PlayLevelSequenceDetails.h"
#include "DetailCustomizations/FlowNode_SubGraphDetails.h"
#include "DetailCustomizations/FlowNodeAddOn_Details.h"
#include "DetailCustomizations/FlowOwnerFunctionRefCustomization.h"
#include "DetailCustomizations/FlowActorOwnerComponentRefCustomization.h"
#include "DetailCustomizations/FlowDataPinPropertyCustomizations.h"
#include "DetailCustomizations/FlowDataPinProperty_ClassCustomization.h"
#include "DetailCustomizations/FlowDataPinProperty_EnumCustomization.h"
#include "DetailCustomizations/FlowDataPinProperty_ObjectCustomization.h"
#include "DetailCustomizations/FlowPinCustomization.h"
#include "DetailCustomizations/FlowNamedDataPinOutputPropertyCustomization.h"

#include "FlowAsset.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/Route/FlowNode_CustomInput.h"
#include "Nodes/Route/FlowNode_CustomOutput.h"
#include "Nodes/Route/FlowNode_SubGraph.h"
#include "Nodes/World/FlowNode_ComponentObserver.h"
#include "Nodes/World/FlowNode_PlayLevelSequence.h"

#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "FlowModule.h"
#include "IAssetSearchModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISequencerChannelInterface.h" // ignore Rider's false "unused include" warning
#include "ISequencerModule.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"

static FName AssetSearchModuleName = TEXT("AssetSearch");

#define LOCTEXT_NAMESPACE "FlowEditorModule"

EAssetTypeCategories::Type FFlowEditorModule::FlowAssetCategory = static_cast<EAssetTypeCategories::Type>(0);
FAssetCategoryPath FFLowAssetCategoryPaths::Flow(LOCTEXT("Flow", "Flow"));

void FFlowEditorModule::StartupModule()
{
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

	// run one-time asserts that cannot be asserted statically
	UFlowK2SchemaSubclassForAccess::AssertPinCategoryNames();
}

void FFlowEditorModule::ShutdownModule()
{
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
		RegisterCustomClassLayout(UFlowNode::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_Details::MakeInstance));
		RegisterCustomClassLayout(UFlowNodeAddOn::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNodeAddOn_Details::MakeInstance));
		RegisterCustomClassLayout(UFlowNode_ComponentObserver::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_ComponentObserverDetails::MakeInstance));
		RegisterCustomClassLayout(UFlowNode_CustomInput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_CustomInputDetails::MakeInstance));
		RegisterCustomClassLayout(UFlowNode_CustomOutput::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_CustomOutputDetails::MakeInstance));
		RegisterCustomClassLayout(UFlowNode_PlayLevelSequence::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_PlayLevelSequenceDetails::MakeInstance));
	    RegisterCustomClassLayout(UFlowNode_SubGraph::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FFlowNode_SubGraphDetails::MakeInstance));
		RegisterCustomStructLayout(*FFlowOwnerFunctionRef::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowOwnerFunctionRefCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowActorOwnerComponentRef::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowActorOwnerComponentRefCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowPin::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowPinCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowNamedDataPinOutputProperty::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowNamedDataPinOutputPropertyCustomization::MakeInstance));

		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_Bool::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_BoolCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_Int64::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_Int64Customization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_Int32::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_Int32Customization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_Double::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_DoubleCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_Float::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_FloatCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_Name::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_NameCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_String::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_StringCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_Text::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_TextCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_Enum::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_EnumCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_Class::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_ClassCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinOutputProperty_Object::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinOutputProperty_ObjectCustomization::MakeInstance));

		RegisterCustomStructLayout(*FFlowDataPinInputProperty_Bool::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_BoolCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinInputProperty_Int64::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_Int64Customization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinInputProperty_Int32::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_Int32Customization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinInputProperty_Double::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_DoubleCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinInputProperty_Float::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_FloatCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinInputProperty_Name::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_NameCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinInputProperty_String::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_StringCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinInputProperty_Text::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_TextCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinInputProperty_Enum::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_EnumCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinInputProperty_Class::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_ClassCustomization::MakeInstance));
		RegisterCustomStructLayout(*FFlowDataPinInputProperty_Object::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFlowDataPinInputProperty_ObjectCustomization::MakeInstance));

		// Consider implementing details customizations... for every EFlowPinType
		FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

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
