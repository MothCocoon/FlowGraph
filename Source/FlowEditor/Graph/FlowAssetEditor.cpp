#include "FlowAssetEditor.h"
#include "FlowAssetGraph.h"
#include "FlowGraphActions.h"
#include "FlowGraphSchema.h"
#include "Nodes/FlowGraphNode.h"
#include "Nodes/FlowGraphNode_In.h"
#include "Nodes/FlowGraphNode_Out.h"
#include "Widgets/SFlowPalette.h"
#include "../FlowEditorModule.h"

#include "Flow/Graph/FlowAsset.h"
#include "Flow/Graph//Nodes/FlowNode.h"

#include "EdGraphUtilities.h"
#include "EdGraph/EdGraphNode.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformApplicationMisc.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "IDetailsView.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "SNodePanel.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FlowEditor"

const FName FFlowAssetEditor::GraphCanvasTabId(TEXT("FlowEditor_GraphCanvas"));
const FName FFlowAssetEditor::PropertiesTabId(TEXT("FlowEditor_Properties"));
const FName FFlowAssetEditor::PaletteTabId(TEXT("FlowEditor_Palette"));

FFlowAssetEditor::FFlowAssetEditor()
	: FlowAsset(nullptr)
{
}

void FFlowAssetEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_FlowAssetEditor", "Flow Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(GraphCanvasTabId, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_GraphCanvas))
		.SetDisplayName(LOCTEXT("GraphCanvasTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(PropertiesTabId, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_Properties))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(PaletteTabId, FOnSpawnTab::CreateSP(this, &FFlowAssetEditor::SpawnTab_Palette))
		.SetDisplayName(LOCTEXT("PaletteTab", "Palette"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "Kismet.Tabs.Palette"));
}

void FFlowAssetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(GraphCanvasTabId);
	InTabManager->UnregisterTabSpawner(PropertiesTabId);
	InTabManager->UnregisterTabSpawner(PaletteTabId);
}

FFlowAssetEditor::~FFlowAssetEditor()
{
	GEditor->UnregisterForUndo(this);
}

void FFlowAssetEditor::InitFlowAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit)
{
	FlowAsset = CastChecked<UFlowAsset>(ObjectToEdit);

	// Support undo/redo
	FlowAsset->SetFlags(RF_Transactional);

	GEditor->RegisterForUndo(this);

	FGraphEditorCommands::Register();
	FFlowGraphCommands::Register();
	FFlowGraphSpawnCommands::Register();

	CreateInternalWidgets();

	BindGraphCommands();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("FlowAssetEditor_Layout")
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
					->AddTab(PropertiesTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.65f)
					->AddTab(GraphCanvasTabId, ETabState::OpenedTab)->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.125f)
					->AddTab(PaletteTabId, ETabState::OpenedTab)
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
		SpawnToolkitTab(GraphCanvasTabId, FString(), EToolkitTabSpot::Viewport);
		SpawnToolkitTab(PropertiesTabId, FString(), EToolkitTabSpot::Details);
	}*/
}

UFlowAsset* FFlowAssetEditor::GetFlowAsset() const
{
	return FlowAsset;
}

void FFlowAssetEditor::SetSelection(TArray<UObject*> SelectedObjects)
{
	if (FlowAssetProperties.IsValid())
	{
		FlowAssetProperties->SetObjects(SelectedObjects);
	}
}

bool FFlowAssetEditor::GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding)
{
	return FlowGraphEditor->GetBoundsForSelectedNodes(Rect, Padding);
}

int32 FFlowAssetEditor::GetNumberOfSelectedNodes() const
{
	return FlowGraphEditor->GetSelectedNodes().Num();
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

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_GraphCanvas(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == GraphCanvasTabId);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("FlowGraphCanvasTitle", "Viewport"));

	if (FlowGraphEditor.IsValid())
	{
		SpawnedTab->SetContent(FlowGraphEditor.ToSharedRef());
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_Properties(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == PropertiesTabId);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("FlowAssetDetailsTitle", "Details"))
		[
			FlowAssetProperties.ToSharedRef()
		];
}

TSharedRef<SDockTab> FFlowAssetEditor::SpawnTab_Palette(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == PaletteTabId);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("Kismet.Tabs.Palette"))
		.Label(LOCTEXT("FlowAssetPaletteTitle", "Palette"))
		[
			Palette.ToSharedRef()
		];
}

void FFlowAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(FlowAsset);
}

void FFlowAssetEditor::PostUndo(bool bSuccess)
{
	if (FlowGraphEditor.IsValid())
	{
		FlowGraphEditor->ClearSelectionSet();
		FlowGraphEditor->NotifyGraphChanged();
	}
}

void FFlowAssetEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, class UProperty* PropertyThatChanged)
{
	if (FlowGraphEditor.IsValid() && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		FlowGraphEditor->NotifyGraphChanged();
	}
}

void FFlowAssetEditor::CreateInternalWidgets()
{
	FlowGraphEditor = CreateGraphEditorWidget();

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FlowAssetProperties = PropertyModule.CreateDetailView(Args);
	FlowAssetProperties->SetObject(FlowAsset);

	Palette = SNew(SFlowPalette, SharedThis(this));
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
	const FFlowGraphCommands& Commands = FFlowGraphCommands::Get();

	ToolkitCommands->MapAction(
		FGenericCommands::Get().Undo,
		FExecuteAction::CreateSP(this, &FFlowAssetEditor::UndoGraphAction));

	ToolkitCommands->MapAction(
		FGenericCommands::Get().Redo,
		FExecuteAction::CreateSP(this, &FFlowAssetEditor::RedoGraphAction));
}

void FFlowAssetEditor::AddInput()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	// Iterator used but should only contain one node
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt);

		if (SelectedNode)
		{
			SelectedNode->AddInputPin();
			break;
		}
	}
}

bool FFlowAssetEditor::CanAddInput() const
{
	return GetSelectedNodes().Num() == 1;
}

void FFlowAssetEditor::DeleteInput()
{
	if (FlowGraphEditor.IsValid())
	{
		UEdGraphPin* SelectedPin = FlowGraphEditor->GetGraphPinForMenu();
		if (ensure(SelectedPin))
		{
			UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(SelectedPin->GetOwningNode());

			if (SelectedNode && SelectedNode == SelectedPin->GetOwningNode())
			{
				SelectedNode->RemoveInputPin(SelectedPin);
			}
		}
	}
}

bool FFlowAssetEditor::CanDeleteInput() const
{
	return true;
}

void FFlowAssetEditor::AddOutput()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	// Iterator used but should only contain one node
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt);

		if (SelectedNode)
		{
			SelectedNode->AddOutputPin();
			break;
		}
	}
}

bool FFlowAssetEditor::CanAddOutput() const
{
	return GetSelectedNodes().Num() == 1;
}

void FFlowAssetEditor::DeleteOutput()
{
	if (FlowGraphEditor.IsValid())
	{
		UEdGraphPin* SelectedPin = FlowGraphEditor->GetGraphPinForMenu();
		if (ensure(SelectedPin))
		{
			UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(SelectedPin->GetOwningNode());

			if (SelectedNode && SelectedNode == SelectedPin->GetOwningNode())
			{
				SelectedNode->RemoveOutputPin(SelectedPin);
			}
		}
	}
}

bool FFlowAssetEditor::CanDeleteOutput() const
{
	return true;
}

void FFlowAssetEditor::FocusViewport()
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

void FFlowAssetEditor::OnCreateComment()
{
	FFlowGraphSchemaAction_NewComment CommentAction;
	CommentAction.PerformAction(FlowAsset->GetGraph(), nullptr, FlowGraphEditor->GetPasteLocation());
}

TSharedRef<SGraphEditor> FFlowAssetEditor::CreateGraphEditorWidget()
{
	if (!GraphEditorCommands.IsValid())
	{
		GraphEditorCommands = MakeShareable(new FUICommandList);

		// Node Commands
		GraphEditorCommands->MapAction(FFlowGraphCommands::Get().AddInput,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::AddInput),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanAddInput));

		GraphEditorCommands->MapAction(FFlowGraphCommands::Get().DeleteInput,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::DeleteInput),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDeleteInput));

		GraphEditorCommands->MapAction(FFlowGraphCommands::Get().AddOutput,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::AddOutput),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanAddOutput));

		GraphEditorCommands->MapAction(FFlowGraphCommands::Get().DeleteOutput,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::DeleteOutput),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDeleteOutput));

		GraphEditorCommands->MapAction(FFlowGraphCommands::Get().FocusViewport,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::FocusViewport),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanFocusViewport));

		// Graph Editor Commands
		GraphEditorCommands->MapAction(FGraphEditorCommands::Get().CreateComment,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::OnCreateComment)
		);

		// Editing commands
		GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::SelectAllNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanSelectAllNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::DeleteSelectedNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDeleteNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::CopySelectedNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanCopyNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::CutSelectedNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanCutNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::PasteNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanPasteNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
			FExecuteAction::CreateSP(this, &FFlowAssetEditor::DuplicateNodes),
			FCanExecuteAction::CreateSP(this, &FFlowAssetEditor::CanDuplicateNodes)
		);
	}

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_FlowAsset", "FLOW");

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FFlowAssetEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FFlowAssetEditor::OnNodeDoubleClicked);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FFlowAssetEditor::OnNodeTitleCommitted);
	InEvents.OnSpawnNodeByShortcut = SGraphEditor::FOnSpawnNodeByShortcut::CreateSP(this, &FFlowAssetEditor::OnSpawnGraphNodeByShortcut, static_cast<UEdGraph*>(FlowAsset->GetGraph()));

	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
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

	TSharedPtr<FEdGraphSchemaAction> Action = FFlowGraphSpawnCommands::Get().GetGraphActionByChord(InChord, InGraph);

	if (Action.IsValid())
	{
		TArray<UEdGraphPin*> DummyPins;
		Action->PerformAction(Graph, DummyPins, InPosition);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void FFlowAssetEditor::OnNodeDoubleClicked(class UEdGraphNode* Node)
{
	UFlowNode* FlowNode = Cast<UFlowGraphNode>(Node)->GetFlowNode();

	if (UObject* AssetToOpen = FlowNode->GetAssetToOpen())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetToOpen);
		return;
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

FGraphPanelSelectionSet FFlowAssetEditor::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	if (FlowGraphEditor.IsValid())
	{
		CurrentSelection = FlowGraphEditor->GetSelectedNodes();
	}
	return CurrentSelection;
}

void FFlowAssetEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	TArray<UObject*> Selection;

	if (NewSelection.Num())
	{
		for (TSet<class UObject*>::TConstIterator SetIt(NewSelection); SetIt; ++SetIt)
		{
			if (Cast<UFlowGraphNode_In>(*SetIt) || Cast<UFlowGraphNode_Out>(*SetIt))
			{
				Selection.Add(GetFlowAsset());
			}
			else if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(*SetIt))
			{
				Selection.Add(Cast<UObject>(GraphNode->GetFlowNode()));
			}
			else
			{
				Selection.Add(*SetIt);
			}
		}
	}
	else
	{
		Selection.Add(GetFlowAsset());
	}

	SetSelection(Selection);
}

void FFlowAssetEditor::SelectAllNodes()
{
	FlowGraphEditor->SelectAllNodes();
}

bool FFlowAssetEditor::CanSelectAllNodes() const
{
	return true;
}

void FFlowAssetEditor::DeleteSelectedNodes()
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "FlowEditorDeleteSelectedNode", "Delete Selected Flow Node"));

	FlowGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FlowGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = CastChecked<UEdGraphNode>(*NodeIt);

		if (Node->CanUserDeleteNode())
		{
			if (UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
			{
				const FGuid NodeGuid = FlowGraphNode->GetFlowNode()->GetGuid();
				FBlueprintEditorUtils::RemoveNode(nullptr, FlowGraphNode, true);
				FlowAsset->UnregisterNode(NodeGuid);
			}
		}
	}
}

void FFlowAssetEditor::DeleteSelectedDuplicatableNodes()
{
	// Cache off the old selection
	const FGraphPanelSelectionSet OldSelectedNodes = GetSelectedNodes();

	// Clear the selection and only select the nodes that can be duplicated
	FGraphPanelSelectionSet RemainingNodes;
	FlowGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			if (Node->CanDuplicateNode())
			{
				FlowGraphEditor->SetNodeSelection(Node, true);
			}
			else
			{
				RemainingNodes.Add(Node);
			}
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	// Reselect whatever's left from the original selection after the deletion
	FlowGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(RemainingNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			FlowGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

bool FFlowAssetEditor::CanDeleteNodes() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	if (SelectedNodes.Num() == 1)
	{
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (Cast<UFlowGraphNode_In>(*NodeIt))
			{
				// Return false if only root node is selected, as it can't be deleted
				return false;
			}
		}
	}

	return SelectedNodes.Num() > 0;
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

void FFlowAssetEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UFlowGraphNode* Node = Cast<UFlowGraphNode>(*SelectedIter))
		{
			Node->PrepareForCopying();
		}
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, /*out*/ ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	// Make sure FlowAsset remains the owner of the copied nodes
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
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}
	return false;
}

void FFlowAssetEditor::PasteNodes()
{
	PasteNodesHere(FlowGraphEditor->GetPasteLocation());
}

void FFlowAssetEditor::PasteNodesHere(const FVector2D& Location)
{
	// Undo/Redo support
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "FlowEditorPaste", "Paste Flow Node"));
	FlowAsset->GetGraph()->Modify();
	FlowAsset->Modify();

	// Clear the selection set (newly pasted stuff will be selected)
	FlowGraphEditor->ClearSelectionSet();

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
		float InvNumNodes = 1.0f / float(PastedNodes.Num());
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
		FlowGraphEditor->SetNodeSelection(Node, true);

		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

		Node->SnapToGrid(SNodePanel::GetSnapGridSize());
	}

	// Force new pasted FlowNodes to have same connections as graph nodes
	FlowAsset->CompileNodeConnections();

	// Update UI
	FlowGraphEditor->NotifyGraphChanged();

	FlowAsset->PostEditChange();
	FlowAsset->MarkPackageDirty();
}

bool FFlowAssetEditor::CanPasteNodes() const
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(FlowAsset->GetGraph(), ClipboardContent);
}

void FFlowAssetEditor::DuplicateNodes()
{
	// Copy and paste current selection
	CopySelectedNodes();
	PasteNodes();
}

bool FFlowAssetEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void FFlowAssetEditor::UndoGraphAction()
{
	GEditor->UndoTransaction();
}

void FFlowAssetEditor::RedoGraphAction()
{
	// Clear selection, to avoid holding refs to nodes that go away
	FlowGraphEditor->ClearSelectionSet();

	GEditor->RedoTransaction();
}

#undef LOCTEXT_NAMESPACE
