#include "Graph/FlowAssetEditor.h"

#include "FlowEditorModule.h"
#include "Graph/FlowAssetGraph.h"
#include "Graph/FlowGraphCommands.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Graph/Widgets/SFlowPalette.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

#include "EdGraphUtilities.h"
#include "EdGraph/EdGraphNode.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "SNodePanel.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FlowEditor"

const FName FFlowAssetEditor::DetailsTab(TEXT("Details"));
const FName FFlowAssetEditor::GraphTab(TEXT("Graph"));
const FName FFlowAssetEditor::PaletteTab(TEXT("Palette"));

FFlowAssetEditor::FFlowAssetEditor()
	: FlowAsset(nullptr)
{
}

FFlowAssetEditor::~FFlowAssetEditor()
{
	GEditor->UnregisterForUndo(this);
}

void FFlowAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(FlowAsset);
}

void FFlowAssetEditor::PostUndo(bool bSuccess)
{
	HandleUndoTransaction();
}

void FFlowAssetEditor::PostRedo(bool bSuccess)
{
	HandleUndoTransaction();
}

void FFlowAssetEditor::HandleUndoTransaction()
{
	SetUISelectionState(NAME_None);
	FocusedGraphEditor->NotifyGraphChanged();
	FSlateApplication::Get().DismissAllMenus();
}

void FFlowAssetEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		FocusedGraphEditor->NotifyGraphChanged();
	}
}

FName FFlowAssetEditor::GetToolkitFName() const
{
	return FName("FlowEditor");
}

FText FFlowAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "FlowAsset Editor");
}

FString FFlowAssetEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "FlowAsset").ToString();
}

FLinearColor FFlowAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

void FFlowAssetEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_FlowAssetEditor", "Flow Editor"));
	const auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(GraphTab, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_GraphCanvas))
		.SetDisplayName(LOCTEXT("GraphTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(DetailsTab, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(PaletteTab, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_Palette))
		.SetDisplayName(LOCTEXT("PaletteTab", "Palette"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "Kismet.Tabs.Palette"));
}

void FFlowAssetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(GraphTab);
	InTabManager->UnregisterTabSpawner(DetailsTab);
	InTabManager->UnregisterTabSpawner(PaletteTab);
}

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_Details(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == DetailsTab);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("FlowDetailsTitle", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_GraphCanvas(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == GraphTab);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("FlowGraphTitle", "Graph"));

	if (FocusedGraphEditor.IsValid())
	{
		SpawnedTab->SetContent(FocusedGraphEditor.ToSharedRef());
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_Palette(const FSpawnTabArgs& Args) const
{
	check(Args.GetTabId() == PaletteTab);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("Kismet.Tabs.Palette"))
		.Label(LOCTEXT("FlowPaletteTitle", "Palette"))
		[
			Palette.ToSharedRef()
		];
}

void FFlowAssetEditor::InitFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit)
{
	FlowAsset = CastChecked<UFlowAsset>(ObjectToEdit);

	// Support undo/redo
	FlowAsset->SetFlags(RF_Transactional);
	GEditor->RegisterForUndo(this);

	FGraphEditorCommands::Register();
	FFlowGraphCommands::Register();
	FFlowSpawnNodeCommands::Register();

	CreateWidgets();
	BindGraphCommands();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("FlowAssetEditor_Layout_v2")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)->SetSizeCoefficient(0.9f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.225f)
					->AddTab(DetailsTab, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.65f)
					->AddTab(GraphTab, ETabState::OpenedTab)->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.125f)
					->AddTab(PaletteTab, ETabState::OpenedTab)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, TEXT("FlowEditorApp"), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectToEdit, false);

	FFlowEditorModule* FlowEditorModule = &FModuleManager::LoadModuleChecked<FFlowEditorModule>("FlowEditor");
	AddMenuExtender(FlowEditorModule->GetFlowAssetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	ExtendToolbar();
	RegenerateMenusAndToolbars();

	// @todo toolkit world centric editing
	/*if(IsWorldCentricAssetEditor())
	{
		SpawnToolkitTab(GetToolbarTabId(), FString(), EToolkitTabSpot::ToolBar);
		SpawnToolkitTab(GraphTab, FString(), EToolkitTabSpot::Viewport);
		SpawnToolkitTab(PropertiesTabId, FString(), EToolkitTabSpot::Details);
	}*/
}

void FFlowAssetEditor::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("Toolbar");
			{
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar)
	);

	AddToolbarExtender(ToolbarExtender);

	FFlowEditorModule* FlowEditorModule = &FModuleManager::LoadModuleChecked<FFlowEditorModule>("FlowEditor");
	AddToolbarExtender(FlowEditorModule->GetFlowAssetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

void FFlowAssetEditor::BindGraphCommands()
{
	const FGenericCommands& GenericCommands = FGenericCommands::Get();

	ToolkitCommands->MapAction(GenericCommands.Undo,
		FExecuteAction::CreateSP(this, &FFlowAssetEditor::UndoGraphAction),
		FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanEdit));

	ToolkitCommands->MapAction(GenericCommands.Redo,
		FExecuteAction::CreateSP(this, &FFlowAssetEditor::RedoGraphAction),
		FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanEdit));
}

void FFlowAssetEditor::UndoGraphAction()
{
	GEditor->UndoTransaction();
}

void FFlowAssetEditor::RedoGraphAction()
{
	GEditor->RedoTransaction();
}

void FFlowAssetEditor::CreateWidgets()
{
	FocusedGraphEditor = CreateGraphEditorWidget();

	FDetailsViewArgs Args;
	Args.bAllowSearch = false;
	Args.bHideSelectionTip = true;
	Args.bShowOptions = false;
	Args.bShowPropertyMatrixButton = false;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	DetailsView = PropertyModule.CreateDetailView(Args);
	DetailsView->SetObject(FlowAsset);

	Palette = SNew(SFlowPalette, SharedThis(this));
}

TSharedRef<SGraphEditor> FFlowAssetEditor::CreateGraphEditorWidget()
{
	if (!Commands.IsValid())
	{
		Commands = MakeShareable(new FUICommandList);

		const FGenericCommands& GenericCommands = FGenericCommands::Get();
		const FGraphEditorCommandsImpl& GraphCommands = FGraphEditorCommands::Get();
		const FFlowGraphCommands& FlowGraphCommands = FFlowGraphCommands::Get();

		// Graph commands
		Commands->MapAction(GraphCommands.CreateComment,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnCreateComment),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanEdit));

		Commands->MapAction(GraphCommands.StraightenConnections,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnStraightenConnections));

		// Generic Node commands
		Commands->MapAction(GenericCommands.SelectAll,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::SelectAllNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanSelectAllNodes));

		Commands->MapAction(GenericCommands.Delete,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::DeleteSelectedNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDeleteNodes));

		Commands->MapAction(GenericCommands.Copy,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::CopySelectedNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanCopyNodes));

		Commands->MapAction(GenericCommands.Cut,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::CutSelectedNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanCutNodes));

		Commands->MapAction(GenericCommands.Paste,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::PasteNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanPasteNodes));

		Commands->MapAction(GenericCommands.Duplicate,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::DuplicateNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDuplicateNodes));

		// Pin commands
		Commands->MapAction(FlowGraphCommands.RefreshContextPins,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::RefreshContextPins),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanRefreshContextPins));

		Commands->MapAction(FlowGraphCommands.AddInput,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::AddInput),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanAddInput));

		Commands->MapAction(FlowGraphCommands.AddOutput,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::AddOutput),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanAddOutput));

		Commands->MapAction(FlowGraphCommands.RemovePin,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::RemovePin),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanRemovePin));

		// Breakpoint commands
		Commands->MapAction(GraphCommands.AddBreakpoint,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnAddBreakpoint),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanAddBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanAddBreakpoint)
		);

		Commands->MapAction(GraphCommands.RemoveBreakpoint,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnRemoveBreakpoint),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanRemoveBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanRemoveBreakpoint)
		);

		Commands->MapAction(GraphCommands.EnableBreakpoint,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnEnableBreakpoint),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanEnableBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanEnableBreakpoint)
		);

		Commands->MapAction(GraphCommands.DisableBreakpoint,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnDisableBreakpoint),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDisableBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanDisableBreakpoint)
		);

		Commands->MapAction(GraphCommands.ToggleBreakpoint,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnToggleBreakpoint),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanToggleBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanToggleBreakpoint)
		);

		// Pin Breakpoint commands
		Commands->MapAction(FlowGraphCommands.AddPinBreakpoint,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnAddPinBreakpoint),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanAddPinBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanAddPinBreakpoint)
		);

		Commands->MapAction(FlowGraphCommands.RemovePinBreakpoint,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnRemovePinBreakpoint),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanRemovePinBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanRemovePinBreakpoint)
		);

		Commands->MapAction(FlowGraphCommands.EnablePinBreakpoint,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnEnablePinBreakpoint),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanEnablePinBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanEnablePinBreakpoint)
		);

		Commands->MapAction(FlowGraphCommands.DisablePinBreakpoint,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnDisablePinBreakpoint),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDisablePinBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanDisablePinBreakpoint)
		);

		Commands->MapAction(FlowGraphCommands.TogglePinBreakpoint,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnTogglePinBreakpoint),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanTogglePinBreakpoint),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FFlowAssetEditor::CanTogglePinBreakpoint)
		);

		// Extra debug commands
		Commands->MapAction(FlowGraphCommands.FocusViewport,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::FocusViewport),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanFocusViewport));
	}

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_FlowAsset", "FLOW");

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FFlowAssetEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FFlowAssetEditor::OnNodeDoubleClicked);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FFlowAssetEditor::OnNodeTitleCommitted);
	InEvents.OnSpawnNodeByShortcut = SGraphEditor::FOnSpawnNodeByShortcut::CreateSP(this, &FFlowAssetEditor::OnSpawnGraphNodeByShortcut, static_cast<UEdGraph*>(FlowAsset->GetGraph()));

	return SNew(SGraphEditor)
		.AdditionalCommands(Commands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(FlowAsset->GetGraph())
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(false);
}

FReply FFlowAssetEditor::OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph)
{
	UEdGraph* Graph = InGraph;

	TSharedPtr<FEdGraphSchemaAction> Action = FFlowSpawnNodeCommands::Get().GetActionByChord(InChord);

	if (Action.IsValid())
	{
		TArray<UEdGraphPin*> DummyPins;
		Action->PerformAction(Graph, DummyPins, InPosition);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void FFlowAssetEditor::SetUISelectionState(const FName SelectionOwner)
{
	if (SelectionOwner != CurrentUISelection)
	{
		ClearSelectionStateFor(CurrentUISelection);
		CurrentUISelection = SelectionOwner;
	}
}

void FFlowAssetEditor::ClearSelectionStateFor(const FName SelectionOwner)
{
	if (SelectionOwner == GraphTab)
	{
		FocusedGraphEditor->ClearSelectionSet();
	}
	else if (SelectionOwner == PaletteTab)
	{
		if (Palette.IsValid())
		{
			Palette->ClearGraphActionMenuSelection();
		}
	}
}

void FFlowAssetEditor::OnCreateComment() const
{
	FFlowGraphSchemaAction_NewComment CommentAction;
	CommentAction.PerformAction(FlowAsset->GetGraph(), nullptr, FocusedGraphEditor->GetPasteLocation());
}

void FFlowAssetEditor::OnStraightenConnections() const
{
	FocusedGraphEditor->OnStraightenConnections();
}

bool FFlowAssetEditor::CanEdit() const
{
	return GEditor->PlayWorld == nullptr;
}

FGraphPanelSelectionSet FFlowAssetEditor::GetSelectedNodes() const
{
	return FocusedGraphEditor->GetSelectedNodes();
}

int32 FFlowAssetEditor::GetNumberOfSelectedNodes() const
{
	return FocusedGraphEditor->GetSelectedNodes().Num();
}

bool FFlowAssetEditor::GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding) const
{
	return FocusedGraphEditor->GetBoundsForSelectedNodes(Rect, Padding);
}

void FFlowAssetEditor::OnSelectedNodesChanged(const TSet<UObject*>& Nodes)
{
	TArray<UObject*> SelectedObjects;

	if (Nodes.Num() > 0)
	{
		SetUISelectionState(GraphTab);

		for (TSet<UObject*>::TConstIterator SetIt(Nodes); SetIt; ++SetIt)
		{
			if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(*SetIt))
			{
				SelectedObjects.Add(Cast<UObject>(GraphNode->GetFlowNode()));
			}
			else
			{
				SelectedObjects.Add(*SetIt);
			}
		}
	}
	else
	{
		SetUISelectionState(NAME_None);
		SelectedObjects.Add(GetFlowAsset());
	}

	if (DetailsView.IsValid())
	{
		DetailsView->SetObjects(SelectedObjects);
	}
}

void FFlowAssetEditor::SelectSingleNode(UEdGraphNode* Node) const
{
	FocusedGraphEditor->SetNodeSelection(Node, true);
}

void FFlowAssetEditor::SelectAllNodes() const
{
	FocusedGraphEditor->SelectAllNodes();
}

bool FFlowAssetEditor::CanSelectAllNodes() const
{
	return true;
}

void FFlowAssetEditor::DeleteSelectedNodes()
{
	const FScopedTransaction Transaction(LOCTEXT("DeleteSelectedNode", "Delete Selected Node"));
	FocusedGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	SetUISelectionState(NAME_None);

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = CastChecked<UEdGraphNode>(*NodeIt);

		if (Node->CanUserDeleteNode())
		{
			if (UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
			{
				if (FlowGraphNode->GetFlowNode())
				{
					const FGuid NodeGuid = FlowGraphNode->GetFlowNode()->GetGuid();
					FBlueprintEditorUtils::RemoveNode(nullptr, Node, true);
					FlowAsset->UnregisterNode(NodeGuid);
					continue;
				}
			}

			FBlueprintEditorUtils::RemoveNode(nullptr, Node, true);
		}
	}
}

void FFlowAssetEditor::DeleteSelectedDuplicatableNodes()
{
	// Cache off the old selection
	const FGraphPanelSelectionSet OldSelectedNodes = GetSelectedNodes();

	// Clear the selection and only select the nodes that can be duplicated
	FGraphPanelSelectionSet RemainingNodes;
	FocusedGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			if (Node->CanDuplicateNode())
			{
				FocusedGraphEditor->SetNodeSelection(Node, true);
			}
			else
			{
				RemainingNodes.Add(Node);
			}
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(RemainingNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			FocusedGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

bool FFlowAssetEditor::CanDeleteNodes() const
{
	if (CanEdit())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (const UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
			{
				if (!Node->CanUserDeleteNode())
				{
					return false;
				}
			}
		}

		return SelectedNodes.Num() > 0;
	}

	return false;
}

void FFlowAssetEditor::CutSelectedNodes()
{
	CopySelectedNodes();

	// Cut should only delete nodes that can be duplicated
	DeleteSelectedDuplicatableNodes();
}

bool FFlowAssetEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FFlowAssetEditor::CopySelectedNodes() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UFlowGraphNode* Node = Cast<UFlowGraphNode>(*SelectedIter))
		{
			Node->PrepareForCopying();
		}
	}

	// Export the selected nodes and place the text on the clipboard
	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(SelectedNodes, /*out*/ ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UFlowGraphNode* Node = Cast<UFlowGraphNode>(*SelectedIter))
		{
			Node->PostCopyNode();
		}
	}
}

bool FFlowAssetEditor::CanCopyNodes() const
{
	if (CanEdit())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
		{
			UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
			if (Node && Node->CanDuplicateNode())
			{
				return true;
			}
		}
	}

	return false;
}

void FFlowAssetEditor::PasteNodes()
{
	PasteNodesHere(FocusedGraphEditor->GetPasteLocation());
}

void FFlowAssetEditor::PasteNodesHere(const FVector2D& Location)
{
	SetUISelectionState(NAME_None);

	// Undo/Redo support
	const FScopedTransaction Transaction(LOCTEXT("PasteNode", "Paste Node"));
	FlowAsset->GetGraph()->Modify();
	FlowAsset->Modify();

	// Clear the selection set (newly pasted stuff will be selected)
	FocusedGraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(FlowAsset->GetGraph(), TextToImport, /*out*/ PastedNodes);

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}

	if (PastedNodes.Num() > 0)
	{
		const float InvNumNodes = 1.0f / static_cast<float>(PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;
	}

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;

		// Give new node a different Guid from the old one
		Node->CreateNewGuid();

		if (UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
		{
			FlowAsset->RegisterNode(Node->NodeGuid, FlowGraphNode->GetFlowNode());
		}

		// Select the newly pasted stuff
		FocusedGraphEditor->SetNodeSelection(Node, true);

		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

		Node->SnapToGrid(SNodePanel::GetSnapGridSize());
	}

	// Force new pasted FlowNodes to have same connections as graph nodes
	FlowAsset->CompileNodeConnections();

	// Update UI
	FocusedGraphEditor->NotifyGraphChanged();

	FlowAsset->PostEditChange();
	FlowAsset->MarkPackageDirty();
}

bool FFlowAssetEditor::CanPasteNodes() const
{
	if (CanEdit())
	{
		FString ClipboardContent;
		FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

		return FEdGraphUtilities::CanImportNodesFromText(FlowAsset->GetGraph(), ClipboardContent);
	}

	return false;
}

void FFlowAssetEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FFlowAssetEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void FFlowAssetEditor::OnNodeDoubleClicked(class UEdGraphNode* Node) const
{
	UFlowNode* FlowNode = Cast<UFlowGraphNode>(Node)->GetFlowNode();

	if (UObject* AssetToOpen = FlowNode->GetAssetToOpen())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetToOpen);
	}

	/*if (ObjectsToView.Num() > 0)
	{
		GEditor->SyncBrowserToObjects(ObjectsToView);
	}*/
}

void FFlowAssetEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		const FScopedTransaction Transaction(LOCTEXT("RenameNode", "Rename Node"));
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

void FFlowAssetEditor::RefreshContextPins() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			SelectedNode->RefreshContextPins();
		}
	}
}

bool FFlowAssetEditor::CanRefreshContextPins() const
{
	if (CanEdit() && GetSelectedNodes().Num() == 1)
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
			{
				return SelectedNode->SupportsContextPins();
			}
		}
	}

	return false;
}

void FFlowAssetEditor::AddInput() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			SelectedNode->AddUserInput();
			break;
		}
	}
}

bool FFlowAssetEditor::CanAddInput() const
{
	if (CanEdit() && GetSelectedNodes().Num() == 1)
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
			{
				return SelectedNode->CanUserAddInput();
			}
		}
	}

	return false;
}

void FFlowAssetEditor::AddOutput() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			SelectedNode->AddUserOutput();
			break;
		}
	}
}

bool FFlowAssetEditor::CanAddOutput() const
{
	if (CanEdit() && GetSelectedNodes().Num() == 1)
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
			{
				return SelectedNode->CanUserAddOutput();
			}
		}
	}

	return false;
}

void FFlowAssetEditor::RemovePin() const
{
	if (UEdGraphPin* SelectedPin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(SelectedPin->GetOwningNode()))
		{
			SelectedNode->RemoveInstancePin(SelectedPin);
		}
	}
}

bool FFlowAssetEditor::CanRemovePin() const
{
	if (CanEdit() && GetSelectedNodes().Num() == 1)
	{
		if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
		{
			if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
			{
				if (Pin->Direction == EGPD_Input)
				{
					return GraphNode->CanUserRemoveInput(Pin);
				}
				else
				{
					return GraphNode->CanUserRemoveOutput(Pin);
				}
			}
		}
	}

	return false;
}

void FFlowAssetEditor::OnAddBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			SelectedNode->NodeBreakpoint.AddBreakpoint();
		}
	}
}

void FFlowAssetEditor::OnAddPinBreakpoint() const
{
	if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			if (Pin->Direction == EGPD_Input)
			{
				FFlowBreakpoint& NewBreakpoint = GraphNode->InputBreakpoints.Add(GraphNode->InputPins.Find(Pin), FFlowBreakpoint());
				NewBreakpoint.AddBreakpoint();
			}
			else
			{
				FFlowBreakpoint& NewBreakpoint = GraphNode->OutputBreakpoints.Add(GraphNode->OutputPins.Find(Pin), FFlowBreakpoint());
				NewBreakpoint.AddBreakpoint();
			}
		}
	}
}

bool FFlowAssetEditor::CanAddBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			return !SelectedNode->NodeBreakpoint.HasBreakpoint();
		}
	}

	return false;
}

bool FFlowAssetEditor::CanAddPinBreakpoint() const
{
	if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			if (Pin->Direction == EGPD_Input)
			{
				const int32 PinIndex = GraphNode->InputPins.Find(Pin);
				return !GraphNode->InputBreakpoints.Contains(PinIndex) || !GraphNode->InputBreakpoints[PinIndex].HasBreakpoint();
			}
			else
			{
				const int32 PinIndex = GraphNode->OutputPins.Find(Pin);
				return !GraphNode->OutputBreakpoints.Contains(PinIndex) || !GraphNode->OutputBreakpoints[PinIndex].HasBreakpoint();
			}
		}
	}

	return false;
}

void FFlowAssetEditor::OnRemoveBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			SelectedNode->NodeBreakpoint.RemoveBreakpoint();
		}
	}
}

void FFlowAssetEditor::OnRemovePinBreakpoint() const
{
	if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			if (Pin->Direction == EGPD_Input)
			{
				GraphNode->InputBreakpoints.Remove(GraphNode->InputPins.Find(Pin));
			}
			else
			{
				GraphNode->OutputBreakpoints.Remove(GraphNode->OutputPins.Find(Pin));
			}
		}
	}
}

bool FFlowAssetEditor::CanRemoveBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			return SelectedNode->NodeBreakpoint.HasBreakpoint();
		}
	}

	return false;
}

bool FFlowAssetEditor::CanRemovePinBreakpoint() const
{
	if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			if (Pin->Direction == EGPD_Input)
			{
				return GraphNode->InputBreakpoints.Contains(GraphNode->InputPins.Find(Pin));
			}
			else
			{
				return GraphNode->OutputBreakpoints.Contains(GraphNode->OutputPins.Find(Pin));
			}
		}
	}

	return false;
}

void FFlowAssetEditor::OnEnableBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			SelectedNode->NodeBreakpoint.EnableBreakpoint();
		}
	}
}

void FFlowAssetEditor::OnEnablePinBreakpoint() const
{
	if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			if (Pin->Direction == EGPD_Input)
			{
				GraphNode->InputBreakpoints[GraphNode->InputPins.Find(Pin)].EnableBreakpoint();
			}
			else
			{
				GraphNode->OutputBreakpoints[GraphNode->OutputPins.Find(Pin)].EnableBreakpoint();
			}
		}
	}
}

bool FFlowAssetEditor::CanEnableBreakpoint() const
{
	if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			if (Pin->Direction == EGPD_Input)
			{
				const int32 PinIndex = GraphNode->InputPins.IndexOfByKey(Pin);
				return GraphNode->InputBreakpoints.Contains(PinIndex);
			}
			else
			{
				const int32 PinIndex = GraphNode->OutputPins.IndexOfByKey(Pin);
				return GraphNode->OutputBreakpoints.Contains(PinIndex);
			}
		}
	}

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			return SelectedNode->NodeBreakpoint.CanEnableBreakpoint();
		}
	}

	return false;
}

bool FFlowAssetEditor::CanEnablePinBreakpoint() const
{
	if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			if (Pin->Direction == EGPD_Input)
			{
				const int32 PinIndex = GraphNode->InputPins.Find(Pin);
				return GraphNode->InputBreakpoints.Contains(PinIndex) && GraphNode->InputBreakpoints[PinIndex].CanEnableBreakpoint();
			}
			else
			{
				const int32 PinIndex = GraphNode->OutputPins.Find(Pin);
				return GraphNode->OutputBreakpoints.Contains(PinIndex) && GraphNode->OutputBreakpoints[PinIndex].CanEnableBreakpoint();
			}
		}
	}

	return false;
}

void FFlowAssetEditor::OnDisableBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			SelectedNode->NodeBreakpoint.DisableBreakpoint();
		}
	}
}

void FFlowAssetEditor::OnDisablePinBreakpoint() const
{
	if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			if (Pin->Direction == EGPD_Input)
			{
				GraphNode->InputBreakpoints[GraphNode->InputPins.Find(Pin)].DisableBreakpoint();
			}
			else
			{
				GraphNode->OutputBreakpoints[GraphNode->OutputPins.Find(Pin)].DisableBreakpoint();
			}
		}
	}
}

bool FFlowAssetEditor::CanDisableBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			return SelectedNode->NodeBreakpoint.IsBreakpointEnabled();
		}
	}

	return false;
}

bool FFlowAssetEditor::CanDisablePinBreakpoint() const
{
	if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			if (Pin->Direction == EGPD_Input)
			{
				const int32 PinIndex = GraphNode->InputPins.Find(Pin);
				return GraphNode->InputBreakpoints.Contains(PinIndex) && GraphNode->InputBreakpoints[PinIndex].IsBreakpointEnabled();
			}
			else
			{
				const int32 PinIndex = GraphNode->OutputPins.Find(Pin);
				return GraphNode->OutputBreakpoints.Contains(PinIndex) && GraphNode->OutputBreakpoints[PinIndex].IsBreakpointEnabled();
			}
		}
	}

	return false;
}

void FFlowAssetEditor::OnToggleBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			SelectedNode->NodeBreakpoint.ToggleBreakpoint();
		}
	}
}

void FFlowAssetEditor::OnTogglePinBreakpoint() const
{
	if (UEdGraphPin* Pin = FocusedGraphEditor->GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			if (Pin->Direction == EGPD_Input)
			{
				FFlowBreakpoint& NewBreakpoint = GraphNode->InputBreakpoints.Add(GraphNode->InputPins.Find(Pin), FFlowBreakpoint());
				NewBreakpoint.ToggleBreakpoint();
			}
			else
			{
				FFlowBreakpoint& NewBreakpoint = GraphNode->OutputBreakpoints.Add(GraphNode->OutputPins.Find(Pin), FFlowBreakpoint());
				NewBreakpoint.ToggleBreakpoint();
			}
		}
	}
}

bool FFlowAssetEditor::CanToggleBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		return Cast<UFlowGraphNode>(*NodeIt) != nullptr;
	}

	return false;
}

bool FFlowAssetEditor::CanTogglePinBreakpoint() const
{
	return FocusedGraphEditor->GetGraphPinForMenu() != nullptr;
}

void FFlowAssetEditor::FocusViewport() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	// Iterator used but should only contain one node
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			UFlowNode* FlowNode = Cast<UFlowGraphNode>(SelectedNode)->GetFlowNode();
			if (UFlowNode* NodeInstance = FlowNode->GetInspectedInstance())
			{
				if (AActor* ActorToFocus = NodeInstance->GetActorToFocus())
				{
					GEditor->SelectNone(false, false, false);
					GEditor->SelectActor(ActorToFocus, true, true, true);
					GEditor->NoteSelectionChange();

					GEditor->MoveViewportCamerasToActor(*ActorToFocus, false);

					FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
					TSharedPtr<SDockTab> LevelEditorTab = LevelEditorModule.GetLevelEditorInstanceTab().Pin();
					if (LevelEditorTab.IsValid())
					{
						LevelEditorTab->DrawAttention();
					}
				}
			}

			return;
		}
	}
}

bool FFlowAssetEditor::CanFocusViewport() const
{
	return GetSelectedNodes().Num() == 1;
}

#undef LOCTEXT_NAMESPACE