// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetToolbar.h"

#include "Graph/FlowGraphUtils.h"
#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowAssetEditorContext.h"
#include "Asset/SAssetRevisionMenu.h"
#include "FlowEditorCommands.h"

#include "FlowAsset.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

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
FText SFlowAssetInstanceList::AllContextsText = LOCTEXT("All", "All");

void SFlowAssetInstanceList::Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset)
{
	TemplateAsset = InTemplateAsset;

	if (TemplateAsset.IsValid())
	{
		TemplateAsset->OnDebuggerRefresh().AddSP(this, &SFlowAssetInstanceList::RefreshInstances);
		RefreshInstances();
	}

	ContextComboBox = SNew(SComboBox<TSharedPtr<FObjectKey>>)
		.OptionsSource(&Contexts)
		.Visibility(this, &SFlowAssetInstanceList::GetContextVisibility)
		.OnGenerateWidget(this, &SFlowAssetInstanceList::OnGenerateContextWidget)
		.OnSelectionChanged(this, &SFlowAssetInstanceList::OnContextSelectionChanged)
		.ContentPadding(FMargin(0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SFlowAssetInstanceList::GetSelectedContextName)
		];

	InstanceComboBox = SNew(SComboBox<TSharedPtr<FObjectKey>>)
		.OptionsSource(&Instances)
		.OnGenerateWidget(this, &SFlowAssetInstanceList::OnGenerateInstanceWidget)
		.OnSelectionChanged(this, &SFlowAssetInstanceList::OnInstanceSelectionChanged)
		.ContentPadding(FMargin(0.f, 2.f))
		[
			SNew(STextBlock)
			.Text(this, &SFlowAssetInstanceList::GetSelectedInstanceName)
		];

	ChildSlot
	[
		SNew(SHorizontalBox)
			.Visibility_Static(&SFlowAssetInstanceList::GetDebuggerVisibility)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				ContextComboBox.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.Padding(8.0, 0.f, 4.f, 0.f)
			.AutoWidth()
			[
				InstanceComboBox.ToSharedRef()
			]
	];
}

SFlowAssetInstanceList::~SFlowAssetInstanceList()
{
	if (TemplateAsset.IsValid())
	{
		TemplateAsset->OnDebuggerRefresh().RemoveAll(this);
	}
}

EVisibility SFlowAssetInstanceList::GetDebuggerVisibility()
{
	return GEditor->PlayWorld ? EVisibility::Visible : EVisibility::Collapsed;
}

void SFlowAssetInstanceList::RefreshInstances()
{
	if (GEditor->ShouldEndPlayMap())
	{
		Contexts.Empty();
		Instances.Empty();
		InstancesPerContext.Empty();
		return;
	}
	
	InstancesPerContext.Empty();

	// gather contexts
	{
		NoContext = MakeShareable(new FObjectKey(nullptr));
		Contexts.Add(NoContext);
		
		// support World context in case of online multiplayer
		// todo: support Local Player context in case of split-screen
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			const UWorld* PlayWorld = WorldContext.World();
			if (PlayWorld && PlayWorld->IsGameWorld())
			{
				FText WorldName = FText::FromString(GetDebugStringForWorld(PlayWorld));
				InstancesPerContext.Add(PlayWorld, FFlowAssetInstanceContext(WorldName));
			}
		}

		if (!SelectedContext.IsValid() || !InstancesPerContext.Contains(*SelectedContext.Get()))
		{
			SelectedContext = NoContext;
		}
	}

	// gather all instances of given UFlowAsset
	{
		Instances.Empty();
		Instances.Add(MakeShareable(new FObjectKey(nullptr)));

		for (const UFlowAsset* ActiveInstance : TemplateAsset->GetActiveInstances())
		{
			const FObjectKey WeakPtrKey = ActiveInstance->GetWorld();
			if (FFlowAssetInstanceContext* FoundContext = InstancesPerContext.Find(WeakPtrKey))
			{
				FoundContext->AssetInstances.Add(ActiveInstance);
			}

			// if no context is selected (we list all instances),
			// or give Flow Asset instance belongs to selected context
			if (SelectedContext == NoContext || *SelectedContext.Get() == WeakPtrKey)
			{
				Instances.Add(MakeShareable(new FObjectKey(ActiveInstance)));
			}
		}
	}
}

EVisibility SFlowAssetInstanceList::GetContextVisibility() const
{
	// switching makes sense only if we have more than 1 specific context
	return InstancesPerContext.Num() > 1 ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<SWidget> SFlowAssetInstanceList::OnGenerateContextWidget(TSharedPtr<FObjectKey> Item)
{
	const FFlowAssetInstanceContext* Context = InstancesPerContext.Find(Item->ResolveObjectPtr());
	return SNew(STextBlock).Text(Context ? Context->DisplayText : AllContextsText);
}

void SFlowAssetInstanceList::OnContextSelectionChanged(TSharedPtr<FObjectKey> SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType != ESelectInfo::Direct)
	{
		SelectedContext = SelectedItem;

		if (TemplateAsset.IsValid())
		{
			TemplateAsset->SetInspectedInstance(nullptr);
		}
	}
}

FText SFlowAssetInstanceList::GetSelectedContextName() const
{
	const UObject* Context = SelectedInstance->ResolveObjectPtr();
	return InstancesPerContext.Find(Context) ? InstancesPerContext.Find(Context)->DisplayText : AllContextsText;
}

TSharedRef<SWidget> SFlowAssetInstanceList::OnGenerateInstanceWidget(const TSharedPtr<FObjectKey> Item) const
{
	const UFlowAsset* Instance = Cast<UFlowAsset>(Item->ResolveObjectPtr());
	const FText Result = Instance ? FText::FromString(Instance->GetDebugName()) : NoInstanceSelectedText;
	return SNew(STextBlock).Text(Result);
}

void SFlowAssetInstanceList::OnInstanceSelectionChanged(const TSharedPtr<FObjectKey> SelectedItem, const ESelectInfo::Type SelectionType)
{
	if (SelectionType != ESelectInfo::Direct)
	{
		SelectedInstance = SelectedItem;

		const UFlowAsset* Instance = Cast<UFlowAsset>(SelectedInstance->ResolveObjectPtr());
		if (TemplateAsset.IsValid() && Instance)
		{
			TemplateAsset->SetInspectedInstance(Instance);
		}
	}
}

FText SFlowAssetInstanceList::GetSelectedInstanceName() const
{
	if (SelectedInstance.IsValid())
	{
		if (const UFlowAsset* Instance = Cast<UFlowAsset>(SelectedInstance->ResolveObjectPtr()))
		{
			return FText::FromString(Instance->GetDebugName());
		}
	}

	return NoInstanceSelectedText;
}

//////////////////////////////////////////////////////////////////////////
// Flow Asset Breadcrumb

void SFlowAssetBreadcrumb::Construct(const FArguments& InArgs, const TWeakObjectPtr<UFlowAsset> InTemplateAsset)
{
	TemplateAsset = InTemplateAsset;

	// create breadcrumb
	SAssignNew(BreadcrumbTrail, SBreadcrumbTrail<FFlowBreadcrumb>)
		.Visibility_Static(&SFlowAssetInstanceList::GetDebuggerVisibility)
		.OnCrumbClicked(this, &SFlowAssetBreadcrumb::OnCrumbClicked)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.TextStyle(FAppStyle::Get(), "NormalText")
		.ButtonContentPadding(FMargin(2.f, 4.f))
		.DelimiterImage(FAppStyle::GetBrush("Icons.ChevronRight"))
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
				if (!Item.ChildInstance.IsExplicitlyNull())
				{
					FlowAssetEditor->JumpToNode(Item.ChildInstance->GetNodeOwningThisAssetInstance()->GetGraphNode());
				}
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
