// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetToolbar.h"

#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowAssetEditorContext.h"
#include "Asset/SAssetRevisionMenu.h"
#include "FlowEditorCommands.h"

#include "FlowAsset.h"

#include "Kismet2/DebuggerCommands.h"
#include "Misc/Attribute.h"
#include "Misc/MessageDialog.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "AssetToolsModule.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlHelpers.h"

#define LOCTEXT_NAMESPACE "FlowDebuggerToolbar"

//////////////////////////////////////////////////////////////////////////
// Flow Asset Instance List

FText SFlowAssetInstanceList::NoInstanceSelectedText = LOCTEXT("NoInstanceSelected", "No instance selected");

void SFlowAssetInstanceList::Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset)
{
	TemplateAsset = InTemplateAsset;
	if (TemplateAsset.IsValid())
	{
		TemplateAsset->OnDebuggerRefresh().AddSP(this, &SFlowAssetInstanceList::RefreshInstances);
		RefreshInstances();
	}

	// create dropdown
	SAssignNew(Dropdown, SComboBox<TSharedPtr<FName>>)
		.OptionsSource(&InstanceNames)
		.Visibility_Static(&SFlowAssetInstanceList::GetDebuggerVisibility)
		.OnGenerateWidget(this, &SFlowAssetInstanceList::OnGenerateWidget)
		.OnSelectionChanged(this, &SFlowAssetInstanceList::OnSelectionChanged)
		[
			SNew(STextBlock)
			.Text(this, &SFlowAssetInstanceList::GetSelectedInstanceName)
		];

	ChildSlot
	[
		Dropdown.ToSharedRef()
	];
}

SFlowAssetInstanceList::~SFlowAssetInstanceList()
{
	if (TemplateAsset.IsValid())
	{
		TemplateAsset->OnDebuggerRefresh().RemoveAll(this);
	}
}

void SFlowAssetInstanceList::RefreshInstances()
{
	// collect instance names of this Flow Asset
	InstanceNames = {MakeShareable(new FName(*NoInstanceSelectedText.ToString()))};
	TemplateAsset->GetInstanceDisplayNames(InstanceNames);

	// select instance
	if (const UFlowAsset* InspectedInstance = TemplateAsset->GetInspectedInstance())
	{
		const FName& InspectedInstanceName = InspectedInstance->GetDisplayName();
		for (const TSharedPtr<FName>& Instance : InstanceNames)
		{
			if (*Instance == InspectedInstanceName)
			{
				SelectedInstance = Instance;
				break;
			}
		}
	}
	else
	{
		// default object is always available
		SelectedInstance = InstanceNames[0];
	}
}

EVisibility SFlowAssetInstanceList::GetDebuggerVisibility()
{
	return GEditor->PlayWorld ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<SWidget> SFlowAssetInstanceList::OnGenerateWidget(const TSharedPtr<FName> Item) const
{
	return SNew(STextBlock).Text(FText::FromName(*Item.Get()));
}

void SFlowAssetInstanceList::OnSelectionChanged(const TSharedPtr<FName> SelectedItem, const ESelectInfo::Type SelectionType)
{
	if (SelectionType != ESelectInfo::Direct)
	{
		SelectedInstance = SelectedItem;

		if (TemplateAsset.IsValid())
		{
			const FName NewSelectedInstanceName = (SelectedInstance.IsValid() && *SelectedInstance != *InstanceNames[0]) ? *SelectedInstance : NAME_None;
			TemplateAsset->SetInspectedInstance(NewSelectedInstanceName);
		}
	}
}

FText SFlowAssetInstanceList::GetSelectedInstanceName() const
{
	return SelectedInstance.IsValid() ? FText::FromName(*SelectedInstance) : NoInstanceSelectedText;
}

//////////////////////////////////////////////////////////////////////////
// Flow Asset Breadcrumb

void SFlowAssetBreadcrumb::Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset)
{
	TemplateAsset = InTemplateAsset;

	// create breadcrumb
	SAssignNew(BreadcrumbTrail, SBreadcrumbTrail<FFlowBreadcrumb>)
		.OnCrumbClicked(this, &SFlowAssetBreadcrumb::OnCrumbClicked)
		.Visibility_Static(&SFlowAssetInstanceList::GetDebuggerVisibility)
		.ButtonStyle(FAppStyle::Get(), "FlatButton")
		.DelimiterImage(FAppStyle::GetBrush("Sequencer.BreadcrumbIcon"))
		.PersistentBreadcrumbs(true)
		.TextStyle(FAppStyle::Get(), "Sequencer.BreadcrumbText");

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .HAlign(HAlign_Right)
		  .VAlign(VAlign_Center)
		  .AutoHeight()
		  .Padding(25.0f, 10.0f)
		[
			BreadcrumbTrail.ToSharedRef()
		]
	];

	// fill breadcrumb
	BreadcrumbTrail->ClearCrumbs();
	if (UFlowAsset* InspectedInstance = TemplateAsset->GetInspectedInstance())
	{
		TArray<TWeakObjectPtr<UFlowAsset>> InstancesFromRoot = {InspectedInstance};

		const UFlowAsset* CheckedInstance = InspectedInstance;
		while (UFlowAsset* ParentInstance = CheckedInstance->GetParentInstance())
		{
			InstancesFromRoot.Insert(ParentInstance, 0);
			CheckedInstance = ParentInstance;
		}

		for (TWeakObjectPtr<UFlowAsset> Instance : InstancesFromRoot)
		{
			if (Instance.IsValid())
			{
				const FFlowBreadcrumb NewBreadcrumb = FFlowBreadcrumb(Instance);
				BreadcrumbTrail->PushCrumb(FText::FromName(NewBreadcrumb.InstanceName), FFlowBreadcrumb(Instance));
			}
		}
	}
}

void SFlowAssetBreadcrumb::OnCrumbClicked(const FFlowBreadcrumb& Item) const
{
	const UFlowAsset* InspectedInstance = TemplateAsset->GetInspectedInstance();
	if (InspectedInstance == nullptr || Item.InstanceName != InspectedInstance->GetDisplayName())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Item.AssetPathName);
	}
}

//////////////////////////////////////////////////////////////////////////
// Flow Asset Toolbar

FFlowAssetToolbar::FFlowAssetToolbar(const TSharedPtr<FFlowAssetEditor> InAssetEditor, UToolMenu* ToolbarMenu)
	: FlowAssetEditor(InAssetEditor)
{
	BuildAssetToolbar(ToolbarMenu);
	BuildDebuggerToolbar(ToolbarMenu);
}

void FFlowAssetToolbar::BuildAssetToolbar(UToolMenu* ToolbarMenu) const
{
	{
		FToolMenuSection& Section = ToolbarMenu->AddSection("FlowAsset");
		Section.InsertPosition = FToolMenuInsert("Asset", EToolMenuInsertType::After);

		// add buttons
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(FFlowToolbarCommands::Get().RefreshAsset));
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(FFlowToolbarCommands::Get().ValidateAsset));
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(FFlowToolbarCommands::Get().EditAssetDefaults));
	}
	
	{
		FToolMenuSection& Section = ToolbarMenu->AddSection("View");
		Section.InsertPosition = FToolMenuInsert("FlowAsset", EToolMenuInsertType::After);

		// Visual Diff: menu to choose asset revision compared with the current one 
		Section.AddDynamicEntry("SourceControlCommands", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
		{
			const UFlowAssetEditorContext* Context = InSection.FindContext<UFlowAssetEditorContext>();
			if (Context && Context->FlowAssetEditor.IsValid())
			{
				InSection.InsertPosition = FToolMenuInsert();
				FToolMenuEntry DiffEntry = FToolMenuEntry::InitComboButton(
					"Diff",
					FUIAction(),
					FOnGetContent::CreateStatic(&FFlowAssetToolbar::MakeDiffMenu, Context),
					LOCTEXT("Diff", "Diff"),
					LOCTEXT("FlowAssetEditorDiffToolTip", "Diff against previous revisions"),
					FSlateIcon(FAppStyle::Get().GetStyleSetName(), "BlueprintDiff.ToolbarIcon")
				);
				DiffEntry.StyleNameOverride = "CalloutToolbar";
				InSection.AddEntry(DiffEntry);
			}
		}));
		
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(
			FFlowToolbarCommands::Get().SearchInAsset,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.FindResults")
		));
	}
}

/** Delegate called to diff a specific revision with the current */
// Copy from FBlueprintEditorToolbar::OnDiffRevisionPicked
static void OnDiffRevisionPicked(FRevisionInfo const& RevisionInfo, const FString& Filename, TWeakObjectPtr<UObject> CurrentAsset)
{
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

	// Get the SCC state
	const FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Filename, EStateCacheUsage::Use);
	if (SourceControlState.IsValid())
	{
		for (int32 HistoryIndex = 0; HistoryIndex < SourceControlState->GetHistorySize(); HistoryIndex++)
		{
			TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(HistoryIndex);
			check(Revision.IsValid());
			if (Revision->GetRevision() == RevisionInfo.Revision)
			{
				// Get the revision of this package from source control
				FString PreviousTempPkgName;
				if (Revision->Get(PreviousTempPkgName))
				{
					// Try and load that package
					UPackage* PreviousTempPkg = LoadPackage(nullptr, *PreviousTempPkgName, LOAD_ForDiff | LOAD_DisableCompileOnLoad);
					if (PreviousTempPkg)
					{
						const FString PreviousAssetName = FPaths::GetBaseFilename(Filename, true);
						UObject* PreviousAsset = FindObject<UObject>(PreviousTempPkg, *PreviousAssetName);
						if (PreviousAsset)
						{
							const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
							const FRevisionInfo OldRevision = {Revision->GetRevision(), Revision->GetCheckInIdentifier(), Revision->GetDate()};
							const FRevisionInfo CurrentRevision = {TEXT(""), Revision->GetCheckInIdentifier(), Revision->GetDate()};
							AssetToolsModule.Get().DiffAssets(PreviousAsset, CurrentAsset.Get(), OldRevision, CurrentRevision);
						}
					}
					else
					{
						FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UnableToLoadAssets", "Unable to load assets to diff. Content may no longer be supported?"));
					}
				}
				break;
			}
		}
	}
}

// Variant of FBlueprintEditorToolbar::MakeDiffMenu
TSharedRef<SWidget> FFlowAssetToolbar::MakeDiffMenu(const UFlowAssetEditorContext* Context)
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		UFlowAsset* FlowAsset = Context ? Context->FlowAssetEditor.Pin()->GetFlowAsset() : nullptr;
		if (FlowAsset)
		{
			FString Filename = SourceControlHelpers::PackageFilename(FlowAsset->GetPathName());
			TWeakObjectPtr<UObject> AssetPtr = FlowAsset;

			// Add our async SCC task widget
			return SNew(SAssetRevisionMenu, Filename)
				.OnRevisionSelected_Static(&OnDiffRevisionPicked, AssetPtr);
		}
		else
		{
			// if asset is null then this means that multiple assets are selected
			FMenuBuilder MenuBuilder(true, nullptr);
			MenuBuilder.AddMenuEntry(LOCTEXT("NoRevisionsForMultipleFlowAssets", "Multiple Flow Assets selected"), FText(), FSlateIcon(), FUIAction());
			return MenuBuilder.MakeWidget();
		}
	}

	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.AddMenuEntry(LOCTEXT("SourceControlDisabled", "Source control is disabled"), FText(), FSlateIcon(), FUIAction());
	return MenuBuilder.MakeWidget();
}

void FFlowAssetToolbar::BuildDebuggerToolbar(UToolMenu* ToolbarMenu) const
{
	FToolMenuSection& Section = ToolbarMenu->AddSection("Debug");
	Section.InsertPosition = FToolMenuInsert("View", EToolMenuInsertType::After);

	Section.AddDynamicEntry("DebuggingCommands", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
	{
		const UFlowAssetEditorContext* Context = InSection.FindContext<UFlowAssetEditorContext>();
		if (Context && Context->GetFlowAsset())
		{
			FPlayWorldCommands::BuildToolbar(InSection);

			InSection.AddEntry(FToolMenuEntry::InitWidget("AssetInstances", SNew(SFlowAssetInstanceList, Context->GetFlowAsset()), FText(), true));

			InSection.AddEntry(FToolMenuEntry::InitToolBarButton(FFlowToolbarCommands::Get().GoToParentInstance));
			InSection.AddEntry(FToolMenuEntry::InitWidget("AssetBreadcrumb", SNew(SFlowAssetBreadcrumb, Context->GetFlowAsset()), FText(), true));
		}
	}));
}

#undef LOCTEXT_NAMESPACE
