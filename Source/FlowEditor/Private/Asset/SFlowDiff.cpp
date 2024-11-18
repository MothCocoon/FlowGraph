// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/SFlowDiff.h"
#include "Asset/FlowDiffControl.h"

#include "FlowAsset.h"

#include "EdGraphUtilities.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "GraphDiffControl.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Internationalization/Text.h"
#include "PropertyEditorModule.h"
#include "SBlueprintDiff.h"
#include "SDetailsSplitter.h"
#include "SlateOptMacros.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "SFlowDiff"

static const FName DetailsMode = FName(TEXT("DetailsMode"));
static const FName GraphMode = FName(TEXT("GraphMode"));

FFlowDiffPanel::FFlowDiffPanel()
	: FlowAsset(nullptr)
	, bShowAssetName(false)
{
}

static int32 GetCurrentIndex(SListView<TSharedPtr<FDiffSingleResult>> const& ListView, const TArray<TSharedPtr<FDiffSingleResult>>& ListViewSource)
{
	const TArray<TSharedPtr<FDiffSingleResult>>& Selected = ListView.GetSelectedItems();
	if (Selected.Num() == 1)
	{
		for (const TSharedPtr<FDiffSingleResult>& Diff : ListViewSource)
		{
			if (Diff == Selected[0])
			{
				return 0;
			}
		}
	}
	return -1;
}

void FlowDiffUtils::SelectNextRow(SListView<TSharedPtr<FDiffSingleResult>>& ListView, const TArray<TSharedPtr<FDiffSingleResult>>& ListViewSource)
{
	const int32 CurrentIndex = GetCurrentIndex(ListView, ListViewSource);
	if (CurrentIndex == ListViewSource.Num() - 1)
	{
		return;
	}

	ListView.SetSelection(ListViewSource[CurrentIndex + 1]);
}

void FlowDiffUtils::SelectPrevRow(SListView<TSharedPtr<FDiffSingleResult>>& ListView, const TArray<TSharedPtr<FDiffSingleResult>>& ListViewSource)
{
	const int32 CurrentIndex = GetCurrentIndex(ListView, ListViewSource);
	if (CurrentIndex == 0)
	{
		return;
	}

	ListView.SetSelection(ListViewSource[CurrentIndex - 1]);
}

bool FlowDiffUtils::HasNextDifference(const SListView<TSharedPtr<FDiffSingleResult>>& ListView, const TArray<TSharedPtr<FDiffSingleResult>>& ListViewSource)
{
	const int32 CurrentIndex = GetCurrentIndex(ListView, ListViewSource);
	return ListViewSource.IsValidIndex(CurrentIndex + 1);
}

bool FlowDiffUtils::HasPrevDifference(const SListView<TSharedPtr<FDiffSingleResult>>& ListView, const TArray<TSharedPtr<FDiffSingleResult>>& ListViewSource)
{
	const int32 CurrentIndex = GetCurrentIndex(ListView, ListViewSource);
	return ListViewSource.IsValidIndex(CurrentIndex - 1);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFlowDiff::Construct(const FArguments& InArgs)
{
	check(InArgs._OldFlow && InArgs._NewFlow);
	PanelOld.FlowAsset = InArgs._OldFlow;
	PanelNew.FlowAsset = InArgs._NewFlow;
	PanelOld.RevisionInfo = InArgs._OldRevision;
	PanelNew.RevisionInfo = InArgs._NewRevision;

	// sometimes we want to clearly identify the assets being diffed (when it's
	// not the same asset in each panel)
	PanelOld.bShowAssetName = InArgs._ShowAssetNames;
	PanelNew.bShowAssetName = InArgs._ShowAssetNames;

	bLockViews = true;

	if (InArgs._ParentWindow.IsValid())
	{
		WeakParentWindow = InArgs._ParentWindow;

		AssetEditorCloseDelegate = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetEditorRequestClose().AddSP(this, &SFlowDiff::OnCloseAssetEditor);
	}

	FToolBarBuilder NavToolBarBuilder(TSharedPtr<const FUICommandList>(), FMultiBoxCustomization::None);
	NavToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SFlowDiff::PrevDiff),
			FCanExecuteAction::CreateSP(this, &SFlowDiff::HasPrevDiff)
		)
		, NAME_None
		, LOCTEXT("PrevDiffLabel", "Prev")
		, LOCTEXT("PrevDiffTooltip", "Go to previous difference")
		, FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlueprintDif.PrevDiff")
	);
	NavToolBarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SFlowDiff::NextDiff),
			FCanExecuteAction::CreateSP(this, &SFlowDiff::HasNextDiff)
		)
		, NAME_None
		, LOCTEXT("NextDiffLabel", "Next")
		, LOCTEXT("NextDiffTooltip", "Go to next difference")
		, FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlueprintDif.NextDiff")
	);

	FToolBarBuilder GraphToolbarBuilder(TSharedPtr<const FUICommandList>(), FMultiBoxCustomization::None);
	GraphToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SFlowDiff::OnToggleLockView))
		, NAME_None
		, LOCTEXT("LockGraphsLabel", "Lock/Unlock")
		, LOCTEXT("LockGraphsTooltip", "Force all graph views to change together, or allow independent scrolling/zooming")
		, TAttribute<FSlateIcon>(this, &SFlowDiff::GetLockViewImage)
	);
	GraphToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SFlowDiff::OnToggleSplitViewMode))
		, NAME_None
		, LOCTEXT("SplitGraphsModeLabel", "Vertical/Horizontal")
		, LOCTEXT("SplitGraphsModeLabelTooltip", "Toggles the split view of graphs between vertical and horizontal")
		, TAttribute<FSlateIcon>(this, &SFlowDiff::GetSplitViewModeImage)
	);

	DifferencesTreeView = DiffTreeView::CreateTreeView(&PrimaryDifferencesList);

	GenerateDifferencesList();

	const auto TextBlock = [](FText Text) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
		.Padding(FMargin(4.0f, 10.0f))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Visibility(EVisibility::HitTestInvisible)
			.TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
			.Text(Text)
		];
	};

	TopRevisionInfoWidget =
		SNew(SSplitter)
		.Visibility(EVisibility::HitTestInvisible)
		+ SSplitter::Slot()
		.Value(.2f)
		[
			SNew(SBox)
		]
		+ SSplitter::Slot()
		.Value(.8f)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(10.0f)
			+ SSplitter::Slot()
			.Value(.5f)
			[
				TextBlock(DiffViewUtils::GetPanelLabel(PanelOld.FlowAsset, PanelOld.RevisionInfo, FText()))
			]
			+ SSplitter::Slot()
			.Value(.5f)
			[
				TextBlock(DiffViewUtils::GetPanelLabel(PanelNew.FlowAsset, PanelNew.RevisionInfo, FText()))
			]
		];

	GraphToolBarWidget =
		SNew(SSplitter)
		.Visibility(EVisibility::HitTestInvisible)
		+ SSplitter::Slot()
		.Value(.2f)
		[
			SNew(SBox)
		]
		+ SSplitter::Slot()
		.Value(.8f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				GraphToolbarBuilder.MakeWidget()
			]
		];

	this->ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Docking.Tab", ".ContentAreaBrush"))
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.VAlign(VAlign_Top)
			[
				TopRevisionInfoWidget.ToSharedRef()
			]
			+ SOverlay::Slot()
			.VAlign(VAlign_Top)
			.Padding(0.0f, 6.0f, 0.0f, 4.0f)
			[
				GraphToolBarWidget.ToSharedRef()
			]
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f, 0.0f, 2.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(4.f)
					.AutoWidth()
					[
						NavToolBarBuilder.MakeWidget()
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpacer)
					]
				]
				+ SVerticalBox::Slot()
				[
					SNew(SSplitter)
					+ SSplitter::Slot()
					.Value(.2f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							DifferencesTreeView.ToSharedRef()
						]
					]
					+ SSplitter::Slot()
					.Value(.8f)
					[
						SAssignNew(ModeContents, SBox)
					]
				]
			]
		]
	];

	SetCurrentMode(DetailsMode);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SFlowDiff::~SFlowDiff()
{
	if (AssetEditorCloseDelegate.IsValid())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetEditorRequestClose().Remove(AssetEditorCloseDelegate);
	}
}

void SFlowDiff::OnCloseAssetEditor(UObject* Asset, const EAssetEditorCloseReason CloseReason)
{
	if (PanelOld.FlowAsset == Asset || PanelNew.FlowAsset == Asset || CloseReason == EAssetEditorCloseReason::CloseAllAssetEditors)
	{
		// Tell our window to close and set our selves to collapsed to try and stop it from ticking
		SetVisibility(EVisibility::Collapsed);

		if (AssetEditorCloseDelegate.IsValid())
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetEditorRequestClose().Remove(AssetEditorCloseDelegate);
		}

		if (WeakParentWindow.IsValid())
		{
			WeakParentWindow.Pin()->RequestDestroyWindow();
		}
	}
}

void SFlowDiff::OnGraphSelectionChanged(const TSharedPtr<FFlowGraphToDiff> Item, ESelectInfo::Type SelectionType)
{
	if (!Item.IsValid())
	{
		return;
	}

	FocusOnGraphRevisions(Item.Get());
}

void SFlowDiff::OnGraphChanged(const FFlowGraphToDiff* Diff)
{
	if (PanelNew.GraphEditor.IsValid() && PanelNew.GraphEditor.Pin()->GetCurrentGraph() == Diff->GetGraphNew())
	{
		FocusOnGraphRevisions(Diff);
	}
}

TSharedRef<SWidget> SFlowDiff::DefaultEmptyPanel()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BlueprintDifGraphsToolTip", "Select Graph to Diff"))
		];
}

TSharedPtr<SWindow> SFlowDiff::CreateDiffWindow(const FText WindowTitle, const UFlowAsset* OldFlow, const UFlowAsset* NewFlow, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision)
{
	// sometimes we're comparing different revisions of one single asset (other 
	// times we're comparing two completely separate assets altogether)
	const bool bIsSingleAsset = (NewFlow->GetName() == OldFlow->GetName());

	TSharedPtr<SWindow> Window = SNew(SWindow)
		.Title(WindowTitle)
		.ClientSize(FVector2D(1000, 800));

	Window->SetContent(SNew(SFlowDiff)
		.OldFlow(OldFlow)
		.NewFlow(NewFlow)
		.OldRevision(OldRevision)
		.NewRevision(NewRevision)
		.ShowAssetNames(!bIsSingleAsset)
		.ParentWindow(Window));

	// Make this window a child of the modal window if we've been spawned while one is active.
	const TSharedPtr<SWindow> ActiveModal = FSlateApplication::Get().GetActiveModalWindow();
	if (ActiveModal.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(Window.ToSharedRef(), ActiveModal.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window.ToSharedRef());
	}

	return Window;
}

void SFlowDiff::NextDiff() const
{
	DiffTreeView::HighlightNextDifference(DifferencesTreeView.ToSharedRef(), RealDifferences, PrimaryDifferencesList);
}

void SFlowDiff::PrevDiff() const
{
	DiffTreeView::HighlightPrevDifference(DifferencesTreeView.ToSharedRef(), RealDifferences, PrimaryDifferencesList);
}

bool SFlowDiff::HasNextDiff() const
{
	return DiffTreeView::HasNextDifference(DifferencesTreeView.ToSharedRef(), RealDifferences);
}

bool SFlowDiff::HasPrevDiff() const
{
	return DiffTreeView::HasPrevDifference(DifferencesTreeView.ToSharedRef(), RealDifferences);
}

FFlowGraphToDiff* SFlowDiff::FindGraphToDiffEntry(const FString& GraphPath) const
{
	const FString SearchGraphPath = GraphToDiff->GetGraphOld() ? FGraphDiffControl::GetGraphPath(GraphToDiff->GetGraphOld()) : FGraphDiffControl::GetGraphPath(GraphToDiff->GetGraphNew());
	if (SearchGraphPath.Equals(GraphPath, ESearchCase::CaseSensitive))
	{
		return GraphToDiff.Get();
	}

	return nullptr;
}

void SFlowDiff::FocusOnGraphRevisions(const FFlowGraphToDiff* Diff)
{
	UEdGraph* Graph = Diff->GetGraphOld() ? Diff->GetGraphOld() : Diff->GetGraphNew();

	const FString GraphPath = FGraphDiffControl::GetGraphPath(Graph);
	HandleGraphChanged(GraphPath);

	ResetGraphEditors();
}

void SFlowDiff::OnDiffListSelectionChanged(TSharedPtr<FFlowObjectDiffArgs> FlowObjectDiffArgs)
{
	const TSharedPtr<FFlowObjectDiff> FlowObjectDiff = FlowObjectDiffArgs->FlowNodeDiff.Pin();
	if (!ensure(FlowObjectDiff.IsValid()))
	{
		return;
	}

	check(!FlowObjectDiff->DiffResult->Result.OwningObjectPath.IsEmpty());
	FocusOnGraphRevisions(FindGraphToDiffEntry(FlowObjectDiff->DiffResult->Result.OwningObjectPath));

	const TSharedPtr<FFlowObjectDiff> ParentFlowNodeDiff = FlowObjectDiff->ParentNodeDiff.Pin();
	const FDiffSingleResult& Result = ParentFlowNodeDiff.IsValid() ? ParentFlowNodeDiff->DiffResult->Result : FlowObjectDiff->DiffResult->Result;

	const auto SafeClearSelection = [](TWeakPtr<SGraphEditor> GraphEditor)
	{
		const TSharedPtr<SGraphEditor> GraphEditorPtr = GraphEditor.Pin();
		if (GraphEditorPtr.IsValid())
		{
			GraphEditorPtr->ClearSelectionSet();
		}
	};

	SafeClearSelection(PanelNew.GraphEditor);
	SafeClearSelection(PanelOld.GraphEditor);

	//Select the details panel to display below the graphs.
	//Show an empty details panel if there is no generated details panel.
	const TSharedPtr<SWidget> OldDetailsPanel = FlowObjectDiff->OldDetailsView.IsValid() ?
		FlowObjectDiff->OldDetailsView->DetailsWidget() : PanelOld.EmptyDetailsView.ToSharedRef();
	const TSharedPtr<SWidget> NewDetailsPanel = FlowObjectDiff->NewDetailsView.IsValid() ?
		FlowObjectDiff->NewDetailsView->DetailsWidget() : PanelNew.EmptyDetailsView.ToSharedRef();

	GraphDiffSplitter->SetBottomLeftContent(OldDetailsPanel.ToSharedRef());
	GraphDiffSplitter->SetBottomRightContent(NewDetailsPanel.ToSharedRef());

	if (Result.Pin1)
	{
		GetDiffPanelForNode(*Result.Pin1->GetOwningNode()).FocusDiff(*Result.Pin1);
		if (Result.Pin2)
		{
			GetDiffPanelForNode(*Result.Pin2->GetOwningNode()).FocusDiff(*Result.Pin2);
		}
	}
	else if (Result.Node1)
	{
		FlowObjectDiff->OnSelectDiff(FlowObjectDiffArgs->PropertyDiff);

		GetDiffPanelForNode(*Result.Node1).FocusDiff(*Result.Node1);
		if (Result.Node2)
		{
			GetDiffPanelForNode(*Result.Node2).FocusDiff(*Result.Node2);
		}
	}
}

void SFlowDiff::OnToggleLockView()
{
	bLockViews = !bLockViews;
	ResetGraphEditors();
}

void SFlowDiff::OnToggleSplitViewMode()
{
	bVerticalSplitGraphMode = !bVerticalSplitGraphMode;

	if (SSplitter* DiffGraphSplitterPtr = DiffGraphSplitter.Get())
	{
		DiffGraphSplitterPtr->SetOrientation(bVerticalSplitGraphMode ? Orient_Horizontal : Orient_Vertical);
	}
}

FSlateIcon SFlowDiff::GetLockViewImage() const
{
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), bLockViews ? "Icons.Lock" : "Icons.Unlock");
}

FSlateIcon SFlowDiff::GetSplitViewModeImage() const
{
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), bVerticalSplitGraphMode ? "BlueprintDif.VerticalDiff.Small" : "BlueprintDif.HorizontalDiff.Small");
}

void SFlowDiff::ResetGraphEditors() const
{
	if (PanelOld.GraphEditor.IsValid() && PanelNew.GraphEditor.IsValid())
	{
		if (bLockViews)
		{
			PanelOld.GraphEditor.Pin()->LockToGraphEditor(PanelNew.GraphEditor);
			PanelNew.GraphEditor.Pin()->LockToGraphEditor(PanelOld.GraphEditor);
		}
		else
		{
			PanelOld.GraphEditor.Pin()->UnlockFromGraphEditor(PanelNew.GraphEditor);
			PanelNew.GraphEditor.Pin()->UnlockFromGraphEditor(PanelOld.GraphEditor);
		}
	}
}

void FFlowDiffPanel::GeneratePanel(UEdGraph* NewGraph, UEdGraph* OldGraph)
{
	const TSharedPtr<TArray<FDiffSingleResult>> Diff = MakeShared<TArray<FDiffSingleResult>>();
	FGraphDiffControl::DiffGraphs(OldGraph, NewGraph, *Diff);
	GeneratePanel(NewGraph, Diff, {});
}

void FFlowDiffPanel::GeneratePanel(UEdGraph* Graph, TSharedPtr<TArray<FDiffSingleResult>> DiffResults, TAttribute<int32> FocusedDiffResult)
{
	if (GraphEditor.IsValid() && GraphEditor.Pin()->GetCurrentGraph() == Graph)
	{
		return;
	}

	TSharedPtr<SWidget> Widget = SNew(SBorder)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
	[
		SNew(STextBlock).Text(LOCTEXT("FlowDiffPanelNoGraphTip", "Graph does not exist in this revision"))
	];

	if (Graph)
	{
		SGraphEditor::FGraphEditorEvents InEvents;
		{
			const auto ContextMenuHandler = [](UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FMenuBuilder* MenuBuilder, bool bIsDebugging)
			{
				MenuBuilder->AddMenuEntry(FGenericCommands::Get().Copy);
				return FActionMenuContent(MenuBuilder->MakeWidget());
			};

			InEvents.OnCreateNodeOrPinMenu = SGraphEditor::FOnCreateNodeOrPinMenu::CreateStatic(ContextMenuHandler);
		}

		if (!GraphEditorCommands.IsValid())
		{
			GraphEditorCommands = MakeShared<FUICommandList>();

			GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
											FExecuteAction::CreateRaw(this, &FFlowDiffPanel::CopySelectedNodes),
											FCanExecuteAction::CreateRaw(this, &FFlowDiffPanel::CanCopyNodes)
			);
		}

		const TSharedRef<SGraphEditor> Editor = SNew(SGraphEditor)
			.AdditionalCommands(GraphEditorCommands)
			.GraphToEdit(Graph)
			.GraphToDiff(nullptr)
			.DiffResults(DiffResults)
			.FocusedDiffResult(FocusedDiffResult)
			.IsEditable(false)
			.GraphEvents(InEvents);

		GraphEditor = Editor;
		Widget = Editor;
	}

	GraphEditorBox->SetContent(Widget.ToSharedRef());
}

FGraphPanelSelectionSet FFlowDiffPanel::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	const TSharedPtr<SGraphEditor> FocusedGraphEd = GraphEditor.Pin();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}
	return CurrentSelection;
}

void FFlowDiffPanel::CopySelectedNodes() const
{
	// Export the selected nodes and place the text on the clipboard
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(SelectedNodes, /*out*/ ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FFlowDiffPanel::CanCopyNodes() const
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		const UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if ((Node != nullptr) && Node->CanDuplicateNode())
		{
			return true;
		}
	}
	return false;
}

void FFlowDiffPanel::FocusDiff(const UEdGraphPin& Pin) const
{
	GraphEditor.Pin()->JumpToPin(&Pin);
}

void FFlowDiffPanel::FocusDiff(const UEdGraphNode& Node) const
{
	if (GraphEditor.IsValid())
	{
		GraphEditor.Pin()->JumpToNode(&Node, false);
	}
}

FFlowDiffPanel& SFlowDiff::GetDiffPanelForNode(const UEdGraphNode& Node)
{
	const ENodeDiffType NodeDiffType = GraphToDiff->GetNodeDiffType(Node);

	if (NodeDiffType == ENodeDiffType::Old)
	{
		return PanelOld;
	}
	if (NodeDiffType == ENodeDiffType::New)
	{
		return PanelNew;
	}

	ensureMsgf(false, TEXT("Looking for node %s but it cannot be found in provided panels"), *Node.GetName());
	static FFlowDiffPanel Default;
	return Default;
}

void SFlowDiff::HandleGraphChanged(const FString& GraphPath)
{
	SetCurrentMode(GraphMode);

	UEdGraph* GraphOld = nullptr;
	UEdGraph* GraphNew = nullptr;
	TSharedPtr<TArray<FDiffSingleResult>> DiffResults;
	int32 RealDifferencesStartIndex = INDEX_NONE;
	{
		UEdGraph* NewGraph = GraphToDiff->GetGraphNew();
		UEdGraph* OldGraph = GraphToDiff->GetGraphOld();
		const FString OtherGraphPath = NewGraph ? FGraphDiffControl::GetGraphPath(NewGraph) : FGraphDiffControl::GetGraphPath(OldGraph);
		if (GraphPath.Equals(OtherGraphPath))
		{
			GraphNew = NewGraph;
			GraphOld = OldGraph;
			DiffResults = GraphToDiff->FoundDiffs;
			RealDifferencesStartIndex = GraphToDiff->RealDifferencesStartIndex;
		}
	}

	const TAttribute<int32> FocusedDiffResult = TAttribute<int32>::CreateLambda(
		[this, RealDifferencesStartIndex]()
		{
			int32 FocusedDiffResult = INDEX_NONE;
			if (RealDifferencesStartIndex != INDEX_NONE)
			{
				FocusedDiffResult = DiffTreeView::CurrentDifference(DifferencesTreeView.ToSharedRef(), RealDifferences) - RealDifferencesStartIndex;
			}

			// find selected index in all the graphs, and subtract the index of the first entry in this graph
			return FocusedDiffResult;
		});

	// only regenerate PanelOld if the old graph has changed
	if (!PanelOld.GraphEditor.IsValid() || GraphOld != PanelOld.GraphEditor.Pin()->GetCurrentGraph())
	{
		PanelOld.GeneratePanel(GraphOld, DiffResults, FocusedDiffResult);
	}

	// only regenerate PanelNew if the old graph has changed
	if (!PanelNew.GraphEditor.IsValid() || GraphNew != PanelNew.GraphEditor.Pin()->GetCurrentGraph())
	{
		PanelNew.GeneratePanel(GraphNew, DiffResults, FocusedDiffResult);
	}
}

void SFlowDiff::GenerateDifferencesList()
{
	PrimaryDifferencesList.Empty();
	RealDifferences.Empty();
	ModePanels.Empty();

	const auto CreateInspector = [](const UObject* Object)
	{
		FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FNotifyHook* NotifyHook = nullptr;

		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.NotifyHook = NotifyHook;
		DetailsViewArgs.ViewIdentifier = FName("ObjectInspector");
		TSharedRef<IDetailsView> DetailsView = EditModule.CreateDetailView(DetailsViewArgs);
		DetailsView->SetObject(const_cast<UObject*>(Object));

		return DetailsView;
	};

	PanelOld.EmptyDetailsView = CreateInspector(nullptr);
	PanelNew.EmptyDetailsView = CreateInspector(nullptr);

	// Now that we have done the diffs, create the panel widgets
	ModePanels.Add(DetailsMode, GenerateDetailsPanel());
	ModePanels.Add(GraphMode, GenerateGraphPanel());

	DifferencesTreeView->RebuildList();
}

SFlowDiff::FDiffControl SFlowDiff::GenerateDetailsPanel()
{
	const TSharedPtr<FFlowAssetDiffControl> NewDiffControl = MakeShared<FFlowAssetDiffControl>(PanelOld.FlowAsset, PanelNew.FlowAsset, FOnDiffEntryFocused::CreateRaw(this, &SFlowDiff::SetCurrentMode, DetailsMode));
	NewDiffControl->GenerateTreeEntries(PrimaryDifferencesList, RealDifferences);

	FDiffControl Ret;
	Ret.DiffControl = NewDiffControl;

	const TSharedRef<SDetailsSplitter> Splitter = SNew(SDetailsSplitter);
	if (PanelOld.FlowAsset)
	{
		Splitter->AddSlot(
			SDetailsSplitter::Slot()
			.Value(0.5f)
			.DetailsView(NewDiffControl->GetDetailsWidget(PanelOld.FlowAsset))
			.DifferencesWithRightPanel(NewDiffControl.ToSharedRef(), &FFlowAssetDiffControl::GetDifferencesWithRight, Cast<UObject>(PanelOld.FlowAsset))
		);
	}
	if (PanelNew.FlowAsset)
	{
		Splitter->AddSlot(
			SDetailsSplitter::Slot()
			.Value(0.5f)
			.DetailsView(NewDiffControl->GetDetailsWidget(PanelNew.FlowAsset))
			.DifferencesWithRightPanel(NewDiffControl.ToSharedRef(), &FFlowAssetDiffControl::GetDifferencesWithRight, Cast<UObject>(PanelOld.FlowAsset))
		);
	}
	Ret.Widget = Splitter;

	return Ret;
}

SFlowDiff::FDiffControl SFlowDiff::GenerateGraphPanel()
{
	// We only have a single permanent graph in Flow Asset
	GraphToDiff = MakeShared<FFlowGraphToDiff>(this, PanelOld.FlowAsset->GetGraph(), PanelNew.FlowAsset->GetGraph(), PanelOld.RevisionInfo, PanelNew.RevisionInfo);
	GraphToDiff->GenerateTreeEntries(PrimaryDifferencesList, RealDifferences);

	SAssignNew(GraphDiffSplitter,SSplitter2x2)
		.TopLeft()[ GenerateGraphWidgetForPanel(PanelOld) ]
		.TopRight()[ GenerateGraphWidgetForPanel(PanelNew) ]

		.BottomLeft()[ PanelOld.EmptyDetailsView.ToSharedRef() ]
		.BottomRight()[ PanelNew.EmptyDetailsView.ToSharedRef() ];

	static const FVector2D GraphPercentage = {.5f, .7f};
	static const FVector2D DetailsViewPercentage = {.5f, .3f};
	static FVector2D Percentages[] = {GraphPercentage, DetailsViewPercentage, GraphPercentage, DetailsViewPercentage};
	GraphDiffSplitter->SetSplitterPercentages(MakeArrayView<FVector2D>(Percentages, UE_ARRAY_COUNT(Percentages)));
	
	FDiffControl Ret;
	Ret.Widget = GraphDiffSplitter;

	return Ret;
}

TSharedRef<SOverlay> SFlowDiff::GenerateGraphWidgetForPanel(FFlowDiffPanel& OutDiffPanel) const
{
	return SNew(SOverlay)
		+ SOverlay::Slot() // Graph slot
		[
			SAssignNew(OutDiffPanel.GraphEditorBox, SBox)
			.HAlign(HAlign_Fill)
			[
				DefaultEmptyPanel()
			]
		]
		+ SOverlay::Slot() // Revision info slot
		.VAlign(VAlign_Bottom)
		.HAlign(HAlign_Right)
		.Padding(FMargin(20.0f, 10.0f))
		[
			GenerateRevisionInfoWidgetForPanel(OutDiffPanel.OverlayGraphRevisionInfo, DiffViewUtils::GetPanelLabel(OutDiffPanel.FlowAsset, OutDiffPanel.RevisionInfo, FText()))
		];
}

TSharedRef<SBox> SFlowDiff::GenerateRevisionInfoWidgetForPanel(TSharedPtr<SWidget>& OutGeneratedWidget, const FText& InRevisionText) const
{
	return SAssignNew(OutGeneratedWidget, SBox)
		.Padding(FMargin(4.0f, 10.0f))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
	[
		SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "DetailsView.CategoryTextStyle")
						.Text(InRevisionText)
						.ShadowColorAndOpacity(FColor::Black)
						.ShadowOffset(FVector2D(1.4, 1.4))
	];
}

void SFlowDiff::SetCurrentMode(FName NewMode)
{
	if (CurrentMode == NewMode)
	{
		return;
	}

	CurrentMode = NewMode;

	const FDiffControl* FoundControl = ModePanels.Find(NewMode);

	if (FoundControl)
	{
		ModeContents->SetContent(FoundControl->Widget.ToSharedRef());
	}
	else
	{
		ensureMsgf(false, TEXT("Diff panel does not support mode %s"), *NewMode.ToString());
	}

	OnModeChanged(NewMode);
}

void SFlowDiff::UpdateTopSectionVisibility(const FName& InNewViewMode) const
{
	SSplitter* GraphToolBarPtr = GraphToolBarWidget.Get();
	SSplitter* TopRevisionInfoWidgetPtr = TopRevisionInfoWidget.Get();

	if (!GraphToolBarPtr || !TopRevisionInfoWidgetPtr)
	{
		return;
	}

	if (InNewViewMode == GraphMode)
	{
		GraphToolBarPtr->SetVisibility(EVisibility::Visible);
		TopRevisionInfoWidgetPtr->SetVisibility(EVisibility::Collapsed);
	}
	else
	{
		GraphToolBarPtr->SetVisibility(EVisibility::Collapsed);
		TopRevisionInfoWidgetPtr->SetVisibility(EVisibility::HitTestInvisible);
	}
}

void SFlowDiff::OnModeChanged(const FName& InNewViewMode) const
{
	UpdateTopSectionVisibility(InNewViewMode);
}

#undef LOCTEXT_NAMESPACE
