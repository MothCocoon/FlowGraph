// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetToolbar.h"

#include "Graph/FlowGraphUtils.h"
#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowAssetEditorContext.h"
#include "Asset/SAssetRevisionMenu.h"
#include "FlowEditorCommands.h"

#include "FlowAsset.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"
#include "FlowUserSettings.h"

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
FText SFlowAssetInstanceList::AllWorldsText = LOCTEXT("AllWorlds", "All Worlds");

void SFlowAssetInstanceList::Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset)
{
	TemplateAsset = InTemplateAsset;

	DebugWorldsComboBox = SNew(SComboBox<TSharedPtr<FFlowDebugWorld>>)
		.OptionsSource(&DebugWorlds)
		.Visibility_Static(&SFlowAssetInstanceList::GetWorldComboVisibility)
		.OnComboBoxOpening(this, &SFlowAssetInstanceList::GenerateDebugWorldNames)
		.OnGenerateWidget(this, &SFlowAssetInstanceList::GenerateWorldItemWidget)
		.OnSelectionChanged(this, &SFlowAssetInstanceList::DebugWorldSelectionChanged)
		.ContentPadding(FMargin(0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SFlowAssetInstanceList::GetSelectedWorldName)
		];

	DebugInstancesComboBox = SNew(SComboBox<TSharedPtr<FFlowDebugInstance>>)
		.OptionsSource(&DebugInstances)
		.OnComboBoxOpening(this, &SFlowAssetInstanceList::GenerateDebugInstances)
		.OnGenerateWidget(this, &SFlowAssetInstanceList::GenerateInstanceItemWidget)
		.OnSelectionChanged(this, &SFlowAssetInstanceList::DebugInstanceSelectionChanged)
		.ContentPadding(FMargin(0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SFlowAssetInstanceList::GetSelectedInstanceName)
		];

	ChildSlot
	[
		SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				DebugWorldsComboBox.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.Padding(8.0, 0.f, 4.f, 0.f)
			.AutoWidth()
			[
				DebugInstancesComboBox.ToSharedRef()
			]
	];

	if (TemplateAsset.IsValid())
	{
		TemplateAsset->OnDebuggerRefresh().AddSP(this, &SFlowAssetInstanceList::GenerateDebugWorldNames);
		TemplateAsset->OnDebuggerRefresh().AddSP(this, &SFlowAssetInstanceList::GenerateDebugInstances);
		GenerateDebugWorldNames();
		GenerateDebugInstances();
	}
}

SFlowAssetInstanceList::~SFlowAssetInstanceList()
{
	if (TemplateAsset.IsValid())
	{
		TemplateAsset->OnDebuggerRefresh().RemoveAll(this);
	}
}

EVisibility SFlowAssetInstanceList::GetWorldComboVisibility()
{
	if (GEditor->PlayWorld != nullptr)
	{
		auto GetNumLocalWorlds = []()
		{
			int32 LocalWorldCount = 0;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::PIE && Context.World() != nullptr)
				{
					++LocalWorldCount;
				}
			}
			return LocalWorldCount;
		};
		
		if (GetNumLocalWorlds() > 1)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

void SFlowAssetInstanceList::GenerateDebugWorldNames()
{
	DebugWorlds.Empty();
	DebugWorlds.Add(MakeShareable(new FFlowDebugWorld(nullptr, AllWorldsText.ToString())));

	for (const FWorldContext& PieContext : GEngine->GetWorldContexts())
	{
		UWorld* PlayWorld = PieContext.World();
		if (PlayWorld && PlayWorld->IsGameWorld())
		{
			FString WorldName = GetDebugStringForWorld(PlayWorld);
			DebugWorlds.Add(MakeShareable(new FFlowDebugWorld(PlayWorld, WorldName)));
		}
	}
	
	TSharedPtr<FFlowDebugWorld> LastSelection = GetDebugWorld();
	DebugWorldsComboBox->SetSelectedItem(LastSelection);
}

TSharedRef<SWidget> SFlowAssetInstanceList::GenerateWorldItemWidget(TSharedPtr<FFlowDebugWorld> Item) const
{
	return SNew(STextBlock)
		.Text(FText::FromString(Item->WorldLabel));
}

void SFlowAssetInstanceList::DebugWorldSelectionChanged(TSharedPtr<FFlowDebugWorld> SelectedItem, ESelectInfo::Type SelectionType)
{
	check(TemplateAsset.IsValid());
	if (SelectionType != ESelectInfo::Direct)
	{
		check(SelectedItem.IsValid());
		TemplateAsset->SetWorldBeingDebugged(SelectedItem->WorldPtr);
		TemplateAsset->SetInspectedInstance(nullptr);
	}
}

FText SFlowAssetInstanceList::GetSelectedWorldName() const
{
	return FText::FromString(DebugWorldsComboBox->GetSelectedItem()->WorldLabel);
}

TSharedPtr<FFlowDebugWorld> SFlowAssetInstanceList::GetDebugWorld() const
{
	check(TemplateAsset.IsValid());
	TWeakObjectPtr<const UWorld> World = TemplateAsset->GetWorldBeingDebugged();
	if (!World.IsExplicitlyNull())
	{
		for (const TSharedPtr<FFlowDebugWorld>& DebugWorld : DebugWorlds)
		{
			if (ensure(DebugWorld.IsValid()) && DebugWorld->WorldPtr == World)
			{
				return DebugWorld;
			}
		}
	}

	check(DebugWorlds.Num() > 0);
	return DebugWorlds[0];
}

void SFlowAssetInstanceList::GenerateDebugInstances()
{
	check(TemplateAsset.IsValid());

	TSharedPtr<FFlowDebugInstance> LastSelection;
	if (UFlowUserSettings::Get()->bKeepLastInspectedInstance)
	{
		LastSelection = GetDebugInstance();
	}
	
	DebugInstances.Empty();
	DebugInstances.Add(MakeShareable(new FFlowDebugInstance(nullptr, *NoInstanceSelectedText.ToString())));

	TWeakObjectPtr<const UWorld> DebugWorld = DebugWorldsComboBox->GetSelectedItem()->WorldPtr;
	
	// collect active instances of this Flow Asset
	for (const UFlowAsset* ActiveInstance: TemplateAsset->GetActiveInstances())
	{
		if (DebugWorld.IsValid() && DebugWorld.Get() != ActiveInstance->GetWorld())
		{
			continue;
		}
		
		TSharedPtr<FFlowDebugInstance> NewInstance = MakeShareable(new FFlowDebugInstance(ActiveInstance, ActiveInstance->GetDebugName()));
		DebugInstances.Add(NewInstance);
	}
	
	TSharedPtr<FFlowDebugInstance> Selection = GetDebugInstance();
	if (Selection.IsValid() && !Selection->IsEmptyObject())
	{
		// If our new selection matches the actual debug instance, set it
		if (LastSelection.IsValid() && LastSelection->InstanceLabel == Selection->InstanceLabel)
		{
			// new selection is the same as our selected instance from previous PIE session, set it as inspected
			TemplateAsset->SetInspectedInstance(Selection->InstancePtr);
		}
		DebugInstancesComboBox->SetSelectedItem(Selection);
	}
	else if (LastSelection.IsValid() && !LastSelection->IsEmptyObject())
	{
		// Re-add the desired runtime instance, even though it is currently null
		DebugInstances.Add(LastSelection);
		DebugInstancesComboBox->SetSelectedItem(LastSelection);
	}
	
	// Finally ensure we have a valid selection, this will set to all objects as a backup
	TSharedPtr<FFlowDebugInstance> CurrentSelection = DebugInstancesComboBox->GetSelectedItem();
	if (DebugInstances.Find(CurrentSelection) == INDEX_NONE)
	{
		check(DebugInstances.Num() > 0);
		DebugInstancesComboBox->SetSelectedItem(DebugInstances[0]);
	}
}

TSharedRef<SWidget> SFlowAssetInstanceList::GenerateInstanceItemWidget(const TSharedPtr<FFlowDebugInstance> Item) const
{
	return SNew(STextBlock)
		.Text(FText::FromString(Item->InstanceLabel));
}

void SFlowAssetInstanceList::DebugInstanceSelectionChanged(const TSharedPtr<FFlowDebugInstance> SelectedItem, const ESelectInfo::Type SelectionType)
{
	check(TemplateAsset.IsValid());
	if (SelectionType != ESelectInfo::Direct)
	{
		check(SelectedItem.IsValid());
		TWeakObjectPtr<const UFlowAsset> Instance = SelectedItem->InstancePtr;
		TemplateAsset->SetInspectedInstance(Instance);
	}
}

FText SFlowAssetInstanceList::GetSelectedInstanceName() const
{
	return FText::FromString(DebugInstancesComboBox->GetSelectedItem()->InstanceLabel);
}

TSharedPtr<FFlowDebugInstance> SFlowAssetInstanceList::GetDebugInstance() const
{
	check(TemplateAsset.IsValid());
	const FStringView DebugName = TemplateAsset->GetLastInspectedInstanceName();
	if (!DebugName.IsEmpty())
	{
		for (int32 ObjectIndex = 0; ObjectIndex < DebugInstances.Num(); ++ObjectIndex)
		{
			if (ensure(DebugInstances[ObjectIndex].IsValid()) && DebugName.Equals(DebugInstances[ObjectIndex]->InstanceLabel))
			{
				return DebugInstances[ObjectIndex];
			}
		}
	}
	
	if (DebugInstances.Num() > 0)
	{
		return DebugInstances[0];
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
// Flow Asset Breadcrumb

void SFlowAssetBreadcrumb::Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset)
{
	TemplateAsset = InTemplateAsset;

	// create breadcrumb
	SAssignNew(BreadcrumbTrail, SBreadcrumbTrail<FFlowBreadcrumb>)
		.OnCrumbClicked(this, &SFlowAssetBreadcrumb::OnCrumbClicked)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.TextStyle(FAppStyle::Get(), "NormalText")
		.ButtonContentPadding( FMargin(2.f, 4.f) )
		.DelimiterImage( FAppStyle::GetBrush("Icons.ChevronRight") )
		.ShowLeadingDelimiter(true)
		.PersistentBreadcrumbs(true);

	ChildSlot
	[
		SNew(SBorder)
		.Visibility(this, &SFlowAssetBreadcrumb::GetBreadcrumbVisibility)
		.BorderImage(new FSlateRoundedBoxBrush(FStyleColors::Transparent, 4, FStyleColors::InputOutline, 1))
		[
			SNew(SBox)
			.MaxDesiredWidth(500.f)
			[
				BreadcrumbTrail.ToSharedRef()
			]
		]
	];

	check(TemplateAsset.IsValid());
	TemplateAsset->OnDebuggerRefresh().AddSP(this, &SFlowAssetBreadcrumb::FillBreadcrumb);
	FillBreadcrumb();
}

EVisibility SFlowAssetBreadcrumb::GetBreadcrumbVisibility() const
{
	return GEditor->PlayWorld && TemplateAsset->GetInspectedInstance() ? EVisibility::Visible : EVisibility::Collapsed;
}

void SFlowAssetBreadcrumb::FillBreadcrumb()
{
	BreadcrumbTrail->ClearCrumbs();
	if (const UFlowAsset* InspectedInstance = TemplateAsset->GetInspectedInstance())
	{
		TArray<TWeakObjectPtr<const UFlowAsset>> InstancesFromRoot = {InspectedInstance};

		const UFlowAsset* CheckedInstance = InspectedInstance;
		while (UFlowAsset* ParentInstance = CheckedInstance->GetParentInstance())
		{
			InstancesFromRoot.Insert(ParentInstance, 0);
			CheckedInstance = ParentInstance;
		}

		for (int32 Index = 0; Index < InstancesFromRoot.Num(); Index++)
		{
			TWeakObjectPtr<const UFlowAsset> Instance = InstancesFromRoot[Index];
			TWeakObjectPtr<const UFlowAsset> ChildInstance = Index < InstancesFromRoot.Num() - 1 ? InstancesFromRoot[Index + 1] : nullptr;
				
			BreadcrumbTrail->PushCrumb(FText::FromName(Instance->GetDisplayName()), FFlowBreadcrumb(Instance, ChildInstance));
		}
	}
}

void SFlowAssetBreadcrumb::OnCrumbClicked(const FFlowBreadcrumb& Item) const
{
	const UFlowAsset* InspectedInstance = TemplateAsset->GetInspectedInstance();
	if (InspectedInstance == nullptr || Item.CurrentInstance != TemplateAsset)
	{
		const TWeakObjectPtr<const UFlowAsset> ClickedInstance = Item.CurrentInstance;
		UFlowAsset* ClickedTemplateAsset = ClickedInstance->GetTemplateAsset();
		
		if (GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(ClickedTemplateAsset))
		{
			ClickedTemplateAsset->SetInspectedInstance(ClickedInstance);
			if (const TSharedPtr<FFlowAssetEditor> FlowAssetEditor = FFlowGraphUtils::GetFlowAssetEditor(ClickedTemplateAsset))
			{
				FlowAssetEditor->JumpToNode(Item.ChildInstance->GetNodeOwningThisAssetInstance()->GetGraphNode());
			}
		}
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
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(FFlowEditorCommands::Get().RefreshAsset));
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(FFlowEditorCommands::Get().ValidateAsset));
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(FFlowEditorCommands::Get().EditAssetDefaults));
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
			FFlowEditorCommands::Get().SearchInAsset,
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

			InSection.AddSeparator(NAME_None).StyleNameOverride = FName("Toolbar.BackplateRight");

			InSection.AddEntry(FToolMenuEntry::InitWidget("AssetBreadcrumb", SNew(SFlowAssetBreadcrumb, Context->GetFlowAsset()), FText(), true));
		}
	}));
}

#undef LOCTEXT_NAMESPACE
