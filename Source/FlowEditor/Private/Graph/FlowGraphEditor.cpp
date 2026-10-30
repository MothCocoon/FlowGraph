// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Graph/FlowGraphEditor.h"

#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowAssetEditorContext.h"
#include "FlowEditorCommands.h"
#include "Graph/Extraction/FlowSubGraphExtractionTool.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "Debugger/FlowDebuggerSubsystem.h"

#include "EdGraphUtilities.h"
#include "Editor/UnrealEdEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "LevelEditor.h"
#include "Misc/ScopeExit.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "ToolMenu.h"
#include "ToolMenuDelegates.h"
#include "ToolMenus.h"
#include "UnrealEdGlobals.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FlowGraphEditor"

void SFlowGraphEditor::Construct(const FArguments& InArgs, const TSharedPtr<FFlowAssetEditor> InAssetEditor)
{
	FlowAssetEditor = InAssetEditor;
	UFlowAsset* FlowAssetPtr =InAssetEditor->GetFlowAsset();
	FlowAsset = FlowAssetPtr;
	DetailsView = InArgs._DetailsView;
	bCanEditInPIE = InArgs._CanEditInPIE;

	DebuggerSubsystem = GEngine->GetEngineSubsystem<UFlowDebuggerSubsystem>();

	BindGraphCommands();
	CreateDebugMenu();

	SGraphEditor::FArguments Arguments;
	Arguments._AdditionalCommands = CommandList;
	Arguments._Appearance = TAttribute<FGraphAppearanceInfo>::CreateSP(this, &SFlowGraphEditor::GetGraphAppearanceInfo);
	Arguments._GraphToEdit = FlowAssetPtr->GetGraph();
	Arguments._GraphEvents = InArgs._GraphEvents;
	Arguments._AutoExpandActionMenu = true;
	Arguments._GraphEvents.OnSelectionChanged = FOnSelectionChanged::CreateSP(this, &SFlowGraphEditor::OnSelectedNodesChanged);
	Arguments._GraphEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &SFlowGraphEditor::OnNodeDoubleClicked);
	Arguments._GraphEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &SFlowGraphEditor::OnNodeTitleCommitted);
	Arguments._GraphEvents.OnSpawnNodeByShortcutAtLocation = FOnSpawnNodeByShortcutAtLocation::CreateStatic(&SFlowGraphEditor::OnSpawnGraphNodeByShortcut, FlowAssetPtr->GetGraph());

	SGraphEditor::Construct(Arguments);
}

void SFlowGraphEditor::CreateDebugMenu()
{
	// The main menu is shared by every open Flow Asset Editor, so commands are resolved from the editor the menu is opened in
	const FNewToolMenuDelegate AddDebugSection = FNewToolMenuDelegate::CreateLambda([](UToolMenu* InMenu)
	{
		const UFlowAssetEditorContext* Context = InMenu->FindContext<UFlowAssetEditorContext>();
		const TSharedPtr<FFlowAssetEditor> AssetEditor = Context ? Context->FlowAssetEditor.Pin() : nullptr;
		const TSharedPtr<SFlowGraphEditor> GraphEditor = AssetEditor.IsValid() ? AssetEditor->GetFlowGraph() : nullptr;

		if (GraphEditor.IsValid())
		{
			FToolMenuSection& Section = InMenu->AddSection("Breakpoints", LOCTEXT("Breakpoints", "Breakpoints"));
			const FFlowGraphCommands& FlowGraphCommands = FFlowGraphCommands::Get();

			Section.AddMenuEntryWithCommandList(FlowGraphCommands.EnableAllBreakpoints, GraphEditor->CommandList);
			Section.AddMenuEntryWithCommandList(FlowGraphCommands.DisableAllBreakpoints, GraphEditor->CommandList);
			Section.AddMenuEntryWithCommandList(FlowGraphCommands.RemoveAllBreakpoints, GraphEditor->CommandList);
		}
	});

	FToolMenuSection& MainSection = UToolMenus::Get()->ExtendMenu("AssetEditor.FlowEditor.MainMenu")->FindOrAddSection(NAME_None);
	FToolMenuEntry& DebugEntry = MainSection.AddSubMenu(
		"Debug",
		LOCTEXT("Debug", "Debug"),
		LOCTEXT("DebugTooltip", "Open the debug menu"),
		AddDebugSection
	);

	DebugEntry.InsertPosition = FToolMenuInsert("Edit", EToolMenuInsertType::After);
}

void SFlowGraphEditor::BindGraphCommands()
{
	FGraphEditorCommands::Register();
	FFlowGraphCommands::Register();
	FFlowSpawnNodeCommands::Register();

	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	const FGraphEditorCommandsImpl& GraphEditorCommands = FGraphEditorCommands::Get();
	const FFlowGraphCommands& FlowGraphCommands = FFlowGraphCommands::Get();

	CommandList = MakeShareable(new FUICommandList);

	// Graph commands
	CommandList->MapAction(GraphEditorCommands.CreateComment,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnCreateComment),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanEdit));

	CommandList->MapAction(GraphEditorCommands.StraightenConnections,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnStraightenConnections));

	CommandList->MapAction(GraphEditorCommands.DeleteAndReconnectNodes,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::DeleteSelectedNodes),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanDeleteNodes));

	// Generic Node commands
	CommandList->MapAction(GenericCommands.Undo,
	                       FExecuteAction::CreateStatic(&SFlowGraphEditor::UndoGraphAction),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanEdit));

	CommandList->MapAction(GenericCommands.Redo,
	                       FExecuteAction::CreateStatic(&SFlowGraphEditor::RedoGraphAction),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanEdit));

	CommandList->MapAction(GenericCommands.SelectAll,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::SelectAllNodes),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanSelectAllNodes));

	CommandList->MapAction(GenericCommands.Delete,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::DeleteSelectedNodes),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanDeleteNodes));

	CommandList->MapAction(GenericCommands.Copy,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::CopySelectedNodes),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanCopyNodes));

	CommandList->MapAction(GenericCommands.Cut,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::CutSelectedNodes),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanCutNodes));

	CommandList->MapAction(GenericCommands.Paste,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::PasteNodes),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanPasteNodes));

	CommandList->MapAction(GenericCommands.Duplicate,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::DuplicateNodes),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanDuplicateNodes));

	// Pin commands
	CommandList->MapAction(FlowGraphCommands.ReconstructNode,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::ReconstructNode),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanReconstructNode));

	CommandList->MapAction(FlowGraphCommands.AddInput,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::AddInput),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanAddInput));

	CommandList->MapAction(FlowGraphCommands.AddOutput,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::AddOutput),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanAddOutput));

	CommandList->MapAction(FlowGraphCommands.RemovePin,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::RemovePin),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanRemovePin));

	// Organization commands
	CommandList->MapAction(GraphEditorCommands.AlignNodesTop,
						   FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnAlignTop));

	CommandList->MapAction(GraphEditorCommands.AlignNodesMiddle,
						   FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnAlignMiddle));

	CommandList->MapAction(GraphEditorCommands.AlignNodesBottom,
						   FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnAlignBottom));

	CommandList->MapAction(GraphEditorCommands.AlignNodesLeft,
						   FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnAlignLeft));

	CommandList->MapAction(GraphEditorCommands.AlignNodesCenter,
						   FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnAlignCenter));

	CommandList->MapAction(GraphEditorCommands.AlignNodesRight,
						   FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnAlignRight));

	CommandList->MapAction(GraphEditorCommands.StraightenConnections,
						   FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnStraightenConnections));
	
	CommandList->MapAction(FlowGraphCommands.ExtractToSubGraph,
					   FExecuteAction::CreateSP(this, &SFlowGraphEditor::ExtractToSubGraph),
					   FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanExtractToSubGraph));

	// Breakpoint commands
	CommandList->MapAction(GraphEditorCommands.AddBreakpoint,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnAddBreakpoint),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanAddBreakpoint),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanAddBreakpoint)
	);

	CommandList->MapAction(GraphEditorCommands.RemoveBreakpoint,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnRemoveBreakpoint),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanRemoveBreakpoint),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanRemoveBreakpoint)
	);

	CommandList->MapAction(GraphEditorCommands.EnableBreakpoint,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnEnableBreakpoint),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanEnableBreakpoint),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanEnableBreakpoint)
	);

	CommandList->MapAction(GraphEditorCommands.DisableBreakpoint,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnDisableBreakpoint),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanDisableBreakpoint),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanDisableBreakpoint)
	);

	CommandList->MapAction(GraphEditorCommands.ToggleBreakpoint,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnToggleBreakpoint),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanToggleBreakpoint),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanToggleBreakpoint)
	);

	// Pin Breakpoint commands
	CommandList->MapAction(FlowGraphCommands.AddPinBreakpoint,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnAddPinBreakpoint),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanAddPinBreakpoint),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanAddPinBreakpoint)
	);

	CommandList->MapAction(FlowGraphCommands.RemovePinBreakpoint,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnRemovePinBreakpoint),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanRemovePinBreakpoint),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanRemovePinBreakpoint)
	);

	CommandList->MapAction(FlowGraphCommands.EnablePinBreakpoint,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnEnablePinBreakpoint),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanEnablePinBreakpoint),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanEnablePinBreakpoint)
	);

	CommandList->MapAction(FlowGraphCommands.DisablePinBreakpoint,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnDisablePinBreakpoint),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanDisablePinBreakpoint),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanDisablePinBreakpoint)
	);

	CommandList->MapAction(FlowGraphCommands.TogglePinBreakpoint,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnTogglePinBreakpoint),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanTogglePinBreakpoint),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanTogglePinBreakpoint)
	);

	CommandList->MapAction(FlowGraphCommands.EnableAllBreakpoints,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::EnableAllBreakpoints),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::HasAnyDisabledBreakpoints));

	CommandList->MapAction(FlowGraphCommands.DisableAllBreakpoints,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::DisableAllBreakpoints),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::HasAnyEnabledBreakpoints));

	CommandList->MapAction(FlowGraphCommands.RemoveAllBreakpoints,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::RemoveAllBreakpoints),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::HasAnyBreakpoints));

	// Execution Override commands
	CommandList->MapAction(FlowGraphCommands.EnableNode,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::SetSignalMode, EFlowSignalMode::Enabled),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanSetSignalMode, EFlowSignalMode::Enabled),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanSetSignalMode, EFlowSignalMode::Enabled)
	);

	CommandList->MapAction(FlowGraphCommands.DisableNode,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::SetSignalMode, EFlowSignalMode::Disabled),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanSetSignalMode, EFlowSignalMode::Disabled),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanSetSignalMode, EFlowSignalMode::Disabled)
	);

	CommandList->MapAction(FlowGraphCommands.SetPassThrough,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::SetSignalMode, EFlowSignalMode::PassThrough),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanSetSignalMode, EFlowSignalMode::PassThrough),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateSP(this, &SFlowGraphEditor::CanSetSignalMode, EFlowSignalMode::PassThrough)
	);

	CommandList->MapAction(FlowGraphCommands.ForcePinActivation,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnForcePinActivation),
	                       FCanExecuteAction::CreateStatic(&SFlowGraphEditor::IsPIE),
	                       FIsActionChecked(),
	                       FIsActionButtonVisible::CreateStatic(&SFlowGraphEditor::IsPIE)
	);

	// Jump commands
	CommandList->MapAction(FlowGraphCommands.FocusViewport,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::FocusViewport),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanFocusViewport));

	CommandList->MapAction(FlowGraphCommands.JumpToNodeDefinition,
	                       FExecuteAction::CreateSP(this, &SFlowGraphEditor::JumpToNodeDefinition),
	                       FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanJumpToNodeDefinition));
}

FGraphAppearanceInfo SFlowGraphEditor::GetGraphAppearanceInfo() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = GetCornerText();
	AppearanceInfo.PIENotifyText = GetPIENotifyText();
	return AppearanceInfo;
}

FText SFlowGraphEditor::GetCornerText() const
{
	return LOCTEXT("AppearanceCornerText_FlowAsset", "FLOW");
}

FText SFlowGraphEditor::GetPIENotifyText() const
{
	if (const UFlowAsset* InspectedInstance = FlowAsset->GetInspectedInstance())
	{
		if (const UWorld* InspectedWorld = InspectedInstance->GetWorld())
		{
			return FText::FromString(GetDebugStringForWorld(InspectedWorld));
		}
	}

	return LOCTEXT("PIENotifyText_FlowAsset", "Template");
}

void SFlowGraphEditor::UndoGraphAction()
{
	GEditor->UndoTransaction();
}

void SFlowGraphEditor::RedoGraphAction()
{
	GEditor->RedoTransaction();
}

FReply SFlowGraphEditor::OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2f& InPosition, UEdGraph* InGraph)
{
	UEdGraph* Graph = InGraph;

	if (FFlowSpawnNodeCommands::IsRegistered())
	{
		const TSharedPtr<FEdGraphSchemaAction> Action = FFlowSpawnNodeCommands::Get().GetActionByChord(InChord);
		if (Action.IsValid())
		{
			TArray<UEdGraphPin*> DummyPins;

			Action->PerformAction(Graph, DummyPins, InPosition);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SFlowGraphEditor::OnCreateComment() const
{
	FFlowGraphSchemaAction_NewComment CommentAction;
	CommentAction.PerformAction(FlowAsset->GetGraph(), nullptr, GetPasteLocation2f());
}

bool SFlowGraphEditor::IsTabFocused() const
{
	return FlowAssetEditor.Pin()->IsTabFocused(FFlowAssetEditor::GraphTab);
}

bool SFlowGraphEditor::CanEdit() const
{
	return GEditor->PlayWorld == nullptr || bCanEditInPIE;
}

bool SFlowGraphEditor::IsPIE()
{
	return GEditor->PlayWorld != nullptr;
}

bool SFlowGraphEditor::IsPlaySessionPaused()
{
	bool bPaused = true;

	for (const FWorldContext& PieContext : GUnrealEd->GetWorldContexts())
	{
		const UWorld* PlayWorld = PieContext.World();
		if (PlayWorld && PlayWorld->IsGameWorld())
		{
			bPaused = bPaused && PlayWorld->bDebugPauseExecution;
		}
	}

	return bPaused;
}

void SFlowGraphEditor::SelectSingleNode(UEdGraphNode* Node)
{
	ClearSelectionSet();
	SetNodeSelection(Node, true);
}

void SFlowGraphEditor::OnSelectedNodesChanged(const TSet<UObject*>& Nodes)
{
	TArray<UObject*> SelectedObjects;

	if (Nodes.Num() > 0)
	{
		FlowAssetEditor.Pin()->SetUISelectionState(FFlowAssetEditor::GraphTab);

		for (TSet<UObject*>::TConstIterator SetIt(Nodes); SetIt; ++SetIt)
		{
			if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(*SetIt))
			{
				SelectedObjects.Add(GraphNode->GetFlowNodeBase());
			}
			else
			{
				SelectedObjects.Add(*SetIt);
			}
		}
	}
	else
	{
		FlowAssetEditor.Pin()->SetUISelectionState(NAME_None);
		SelectedObjects.Add(FlowAsset.Get());
	}

	if (DetailsView.IsValid())
	{
		DetailsView->SetObjects(SelectedObjects);
	}

	OnSelectionChangedEvent.ExecuteIfBound(Nodes);
}

TArray<UFlowGraphNode*> SFlowGraphEditor::GetSelectedFlowNodes() const
{
	TArray<UFlowGraphNode*> Result;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			if (IsValid(SelectedNode))
			{
				Result.Emplace(SelectedNode);
			}
		}
	}

	return Result;
}

void SFlowGraphEditor::ReconnectExecPins(const UFlowGraphNode* Node)
{
	if (Node == nullptr)
	{
		return;
	}

	UEdGraphPin* InputPin = nullptr;
	UEdGraphPin* OutputPin = nullptr;

	for (UEdGraphPin* Pin : Node->InputPins)
	{
		if (Pin->HasAnyConnections())
		{
			if (InputPin)
			{
				// more than one connected input pins - do not reconnect anything
				return;
			}

			if (Pin)
			{
				InputPin = Pin;
			}
		}
		else if (InputPin == nullptr)
		{
			// first pin doesn't have any connections - do not reconnect anything, because we probably don't know expected result for user
			return;
		}
	}

	for (UEdGraphPin* Pin : Node->OutputPins)
	{
		if (Pin->HasAnyConnections())
		{
			if (OutputPin)
			{
				// more than one connected output pins - do not reconnect anything
				return;
			}

			if (Pin)
			{
				OutputPin = Pin;
			}
		}
		else if (OutputPin == nullptr)
		{
			// first pin doesn't have any connections - do not reconnect anything, because we probably don't know expected result for user
			return;
		}
	}

	if (InputPin && OutputPin)
	{
		// Make a connection from every incoming exec pin to every outgoing then pin
		for (UEdGraphPin* const IncomingConnectionPin : InputPin->LinkedTo)
		{
			if (IncomingConnectionPin)
			{
				for (UEdGraphPin* const ConnectedCompletePin : OutputPin->LinkedTo)
				{
					IncomingConnectionPin->MakeLinkTo(ConnectedCompletePin);
				}
			}
		}
	}
}

void SFlowGraphEditor::DeleteSelectedNodes()
{
	const FScopedTransaction Transaction(LOCTEXT("DeleteSelectedNode", "Delete Selected Node"));
	GetCurrentGraph()->Modify();
	UFlowAsset* FlowAssetPtr =FlowAsset.Get();
	FlowAssetPtr->Modify();

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	FlowAssetEditor.Pin()->SetUISelectionState(NAME_None);

	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = CastChecked<UEdGraphNode>(*NodeIt);
		if (Node->CanUserDeleteNode())
		{
			if (DebuggerSubsystemPtr)
			{
				DebuggerSubsystemPtr->RemoveAllBreakpoints(Node->NodeGuid);
			}

			if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
			{
				if (const UFlowNode* FlowNode = Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase()))
				{
					// If the user is pressing shift then try and reconnect the pins
					if (FSlateApplication::Get().GetModifierKeys().IsShiftDown())
					{
						ReconnectExecPins(FlowGraphNode);
					}

					GetCurrentGraph()->GetSchema()->BreakNodeLinks(*Node);
					Node->DestroyNode();

					FlowAssetPtr->UnregisterNode(FlowNode->GetGuid());
					continue;
				}
			}

			GetCurrentGraph()->GetSchema()->BreakNodeLinks(*Node);
			Node->DestroyNode();
		}
	}
}

void SFlowGraphEditor::DeleteSelectedDuplicableNodes()
{
	// Cache off the old selection
	const FGraphPanelSelectionSet OldSelectedNodes = GetSelectedNodes();

	// Clear the selection and only select the nodes that can be duplicated
	FGraphPanelSelectionSet RemainingNodes;
	ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIt(OldSelectedNodes); SelectedIt; ++SelectedIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIt))
		{
			if (Node->CanDuplicateNode())
			{
				SetNodeSelection(Node, true);
			}
			else
			{
				RemainingNodes.Add(Node);
			}
		}
	}

	// Delete the duplicable nodes
	DeleteSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIt(RemainingNodes); SelectedIt; ++SelectedIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIt))
		{
			SetNodeSelection(Node, true);
		}
	}
}

bool SFlowGraphEditor::CanDeleteNodes() const
{
	if (CanEdit() && IsTabFocused())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (const UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
			{
				if (Node->CanUserDeleteNode())
				{
					return true;
				}
			}
		}
	}

	return false;
}

void SFlowGraphEditor::CopySelectedNodes() const
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	FGraphPanelSelectionSet NewSelectedNodes;

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(*SelectedIter))
		{
			PrepareFlowGraphNodeForCopy(*FlowGraphNode, FGuid(), NewSelectedNodes);
		}
		else
		{
			NewSelectedNodes.Add(*SelectedIter);
		}
	}

	FString ExportedText;

	FEdGraphUtilities::ExportNodesToText(NewSelectedNodes, ExportedText);
	ClipboardCopy(ExportedText);

	for (FGraphPanelSelectionSet::TIterator SelectedIter(NewSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(*SelectedIter))
		{
			FlowGraphNode->PostCopyNode();
		}
	}
}

void SFlowGraphEditor::PrepareFlowGraphNodeForCopy(UFlowGraphNode& FlowGraphNode, const FGuid& ParentGuid, FGraphPanelSelectionSet& NewSelectedNodes)
{
	bool bAlreadyInSet = false;
	NewSelectedNodes.Add(&FlowGraphNode, &bAlreadyInSet);

	if (bAlreadyInSet)
	{
		// A subnode selected together with its parent is visited twice, in either order.
		// Only the visit through the parent carries a valid ParentGuid, so the other visit must not overwrite it.
		if (ParentGuid.IsValid())
		{
			FlowGraphNode.ParentGuidCopy = ParentGuid;
		}
	}
	else
	{
		// only valid for SubNodes
		FlowGraphNode.ParentGuidCopy = ParentGuid;

		FlowGraphNode.PrepareForCopying();

		// Append all subnodes for selection.
		for (UFlowGraphNode* SubNode : FlowGraphNode.SubNodes)
		{
			if (SubNode)
			{
				PrepareFlowGraphNodeForCopy(*SubNode, FlowGraphNode.NodeGuid, NewSelectedNodes);
			}
		}
	}
}

bool SFlowGraphEditor::CanCopyNodes() const
{
	if (CanEdit() && IsTabFocused())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator SelectedIt(SelectedNodes); SelectedIt; ++SelectedIt)
		{
			const UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIt);
			if (Node && Node->CanDuplicateNode())
			{
				return true;
			}
		}
	}

	return false;
}

void SFlowGraphEditor::CutSelectedNodes()
{
	CopySelectedNodes();

	// Cut should only delete nodes that can be duplicated
	DeleteSelectedDuplicableNodes();
}

bool SFlowGraphEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void SFlowGraphEditor::PasteNodes()
{
	PasteNodesHere(GetPasteLocation2f());
}

void SFlowGraphEditor::PasteNodesHere(const FVector2f& Location)
{
	// Start the transaction and protect the graph update lock.
	FScopedTransaction Transaction(LOCTEXT("PasteNode", "Paste Node"));
	UFlowAsset* FlowAssetPtr = FlowAsset.Get();
	UFlowGraph* FlowGraph = CastChecked<UFlowGraph>(FlowAssetPtr->GetGraph());
	FlowGraph->Modify();
	FlowAssetPtr->Modify();

	FlowGraph->LockUpdates();
	bool bUpdatesLocked = true;
	ON_SCOPE_EXIT
	{
		if (bUpdatesLocked)
		{
			FlowGraph->UnlockUpdates();
		}
	};

	const TArray<UFlowGraphNode*> SelectedNodes = GetSelectedFlowNodes();

	// Parent for AddOns copied without their parent node.
	// Unused when pasting base nodes or AddOns copied together with their parent.
	UFlowGraphNode* AddOnPasteTarget = SelectedNodes.Num() == 1 ? SelectedNodes[0] : nullptr;

	// Import clipboard nodes and reject incompatible nodes or AddOn trees.
	FString TextToImport;
	TMap<FGuid, UFlowGraphNode*> SourceGuidToPastedNode;
	const TSet<UEdGraphNode*> NodesToPaste = ImportAndValidatePastedNodes(
		TextToImport,
		SourceGuidToPastedNode,
		AddOnPasteTarget);

	if (NodesToPaste.IsEmpty())
	{
		// Nothing was pasted, so leave no empty step in the undo history.
		Transaction.Cancel();
		return;
	}

	// Position nodes, assign new identities, and register runtime nodes.
	PreparePastedNodesForPaste(NodesToPaste, Location);

	// Reattach copied AddOns to the selected target or their copied parent.
	AttachPastedSubNodes(NodesToPaste, SourceGuidToPastedNode, AddOnPasteTarget);

	// Finalize the graph mutation before refreshing the editor UI.
	FlowGraph->UpdateClassData();
	FlowGraph->OnNodesPasted(TextToImport);
	FlowGraph->UnlockUpdates();
	bUpdatesLocked = false;

	NotifyGraphChanged();

	if (UObject* GraphOwner = FlowGraph->GetOuter())
	{
		GraphOwner->PostEditChange();
		(void)GraphOwner->MarkPackageDirty();
	}
}

void SFlowGraphEditor::ClipboardCopy(const FString& Text) const
{
	FPlatformApplicationMisc::ClipboardCopy(*Text);
}

void SFlowGraphEditor::ClipboardPaste(FString& OutText) const
{
	FPlatformApplicationMisc::ClipboardPaste(OutText);
}

TSet<UEdGraphNode*> SFlowGraphEditor::ImportNodesToPasteFromClipboard(UFlowGraph& FlowGraph, FString& OutTextToImport) const
{
	ClipboardPaste(OutTextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> NodesToPaste;
	FEdGraphUtilities::ImportNodesFromText(&FlowGraph, OutTextToImport, /*out*/ NodesToPaste);

	return NodesToPaste;
}

TSet<UEdGraphNode*> SFlowGraphEditor::ImportAndValidatePastedNodes(FString& OutTextToImport, TMap<FGuid, UFlowGraphNode*>& OutSourceGuidToPastedNode, const UFlowGraphNode* AddOnPasteTarget) const
{
	UFlowGraph* FlowGraph = CastChecked<UFlowGraph>(FlowAsset->GetGraph());

	TSet<UEdGraphNode*> NodesToPaste = ImportNodesToPasteFromClipboard(*FlowGraph, OutTextToImport);
	if (NodesToPaste.IsEmpty())
	{
		return NodesToPaste;
	}

	// Map source Guids to pasted nodes before pasted nodes receive new Guids.
	TArray<UFlowGraphNode*> RejectedNodes;
	TSet<UEdGraphNode*> RootSubNodesToPaste;
	for (UEdGraphNode* Node : NodesToPaste)
	{
		if (UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
		{
			OutSourceGuidToPastedNode.Add(FlowGraphNode->NodeGuid, FlowGraphNode);

			// An AddOn copied without its parent needs the paste target.
			if (FlowGraphNode->IsSubNode() && !FlowGraphNode->ParentGuidCopy.IsValid())
			{
				RootSubNodesToPaste.Add(FlowGraphNode);
			}
		}
	}

	// Validate pasted sibling sets.
	const bool bCanPasteRootSubNodes = RootSubNodesToPaste.IsEmpty() || (AddOnPasteTarget && CanPasteNodesAsSubNodes(RootSubNodesToPaste, *AddOnPasteTarget));

	// Reject subnodes whose ancestor chain is broken or ends at a root AddOn that cannot be pasted.
	for (auto It = OutSourceGuidToPastedNode.CreateIterator(); It; ++It)
	{
		if (It.Value()->IsSubNode())
		{
			// Walk up to the topmost pasted ancestor: a base node, a root AddOn, or nullptr for a rejected ancestor.
			const UFlowGraphNode* Ancestor = It.Value();
			while (Ancestor && Ancestor->IsSubNode() && Ancestor->ParentGuidCopy.IsValid())
			{
				Ancestor = OutSourceGuidToPastedNode.FindRef(Ancestor->ParentGuidCopy);
			}

			const bool bAccepted = Ancestor && (!Ancestor->IsSubNode() || bCanPasteRootSubNodes);
			if (!bAccepted)
			{
				RejectedNodes.Add(It.Value());
				NodesToPaste.Remove(It.Value());
				It.RemoveCurrent();
			}
		}
	}

	// Destroy rejected imported objects.
	for (UFlowGraphNode* Node : RejectedNodes)
	{
		Node->DestroyNode();
		Node->Rename(nullptr, GetTransientPackage(), REN_NonTransactional | REN_DontCreateRedirectors);
		Node->MarkAsGarbage();
	}

	// AddOns of nodes dropped by the clipboard import are rejected silently, the import handles reporting of those nodes.
	if (!bCanPasteRootSubNodes)
	{
		FNotificationInfo Info(LOCTEXT("RejectedPastedAddOns", "Copied add-ons need exactly one selected node that accepts them and could not be pasted."));
		Info.ExpireDuration = 3.0f;
		Info.bUseLargeFont = false;
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	return NodesToPaste;
}

void SFlowGraphEditor::PreparePastedNodesForPaste(const TSet<UEdGraphNode*>& NodesToPaste, const FVector2f& Location)
{
	ClearSelectionSet();
	FlowAssetEditor.Pin()->SetUISelectionState(NAME_None);

	// Find the average position of top-level nodes to preserve their relative layout.
	FVector2D AverageNodePosition(0.0f, 0.0f);
	int32 AverageCount = 0;
	for (UEdGraphNode* EdNode : NodesToPaste)
	{
		const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
		if (EdNode && (FlowGraphNode == nullptr || !FlowGraphNode->IsSubNode()))
		{
			AverageNodePosition.X += EdNode->NodePosX;
			AverageNodePosition.Y += EdNode->NodePosY;
			++AverageCount;
		}
	}

	if (AverageCount > 0)
	{
		const float InverseNodeCount = 1.0f / static_cast<float>(AverageCount);
		AverageNodePosition.X *= InverseNodeCount;
		AverageNodePosition.Y *= InverseNodeCount;
	}

	UFlowAsset* FlowAssetPtr =FlowAsset.Get();
	for (UEdGraphNode* PastedNode : NodesToPaste)
	{
		UFlowGraphNode* PastedFlowGraphNode = Cast<UFlowGraphNode>(PastedNode);
		if (PastedNode && (PastedFlowGraphNode == nullptr || !PastedFlowGraphNode->IsSubNode()))
		{
			// Select the newly pasted stuff.
			constexpr bool bSelectNodes = true;
			SetNodeSelection(PastedNode, bSelectNodes);

			PastedNode->NodePosX = (PastedNode->NodePosX - AverageNodePosition.X) + Location.X;
			PastedNode->NodePosY = (PastedNode->NodePosY - AverageNodePosition.Y) + Location.Y;
			PastedNode->SnapToGrid(16);

			// Give the pasted node a different Guid from the original.
			PastedNode->CreateNewGuid();
		}

		if (PastedFlowGraphNode)
		{
			if (UFlowNode* FlowNode = Cast<UFlowNode>(PastedFlowGraphNode->GetFlowNodeBase()))
			{
				// Only full FlowNodes are registered with the Asset (for now, AddOns are not).
				FlowAssetPtr->RegisterNode(PastedNode->NodeGuid, FlowNode);
			}

			PastedFlowGraphNode->RemoveAllSubNodes();
		}
	}
}

void SFlowGraphEditor::AttachPastedSubNodes(const TSet<UEdGraphNode*>& NodesToPaste, const TMap<FGuid, UFlowGraphNode*>& SourceGuidToPastedNode, UFlowGraphNode* AddOnPasteTarget)
{
	for (UEdGraphNode* EdNode : NodesToPaste)
	{
		UFlowGraphNode* PasteNode = Cast<UFlowGraphNode>(EdNode);
		if (!PasteNode || !PasteNode->IsSubNode())
		{
			continue;
		}

		PasteNode->NodePosX = 0;
		PasteNode->NodePosY = 0;

		// Remove the subnode from the graph's node list, it was placed there temporarily during clipboard import.
		// This node will be referenced by its parent (FlowNode or AddOn).
		PasteNode->DestroyNode();

		if (!PasteNode->ParentGuidCopy.IsValid())
		{
			// A subnode copied without its parent attaches to the selected paste target.
			if (AddOnPasteTarget)
			{
				AddOnPasteTarget->AddSubNode(PasteNode);
			}
		}
		else if (UFlowGraphNode* PastedParentNode = SourceGuidToPastedNode.FindRef(PasteNode->ParentGuidCopy))
		{
			PastedParentNode->AddSubNode(PasteNode);
		}
	}
}

bool SFlowGraphEditor::CanPasteNodes() const
{
	if (!CanEdit() || !IsTabFocused())
	{
		return false;
	}

	FString ClipboardContent;
	ClipboardPaste(ClipboardContent);

	const UFlowGraph* FlowGraph = CastChecked<UFlowGraph>(FlowAsset->GetGraph());
	if (!ensure(IsValid(FlowGraph)))
	{
		// We expect to have a legal FlowGraph pointer at this point
		return false;
	}

	return FEdGraphUtilities::CanImportNodesFromText(FlowGraph, ClipboardContent);
}

bool SFlowGraphEditor::CanPasteNodesAsSubNodes(const TSet<UEdGraphNode*>& NodesToPaste, const UFlowGraphNode& PasteTargetNode)
{
	TSet<const UEdGraphNode*> AllRootSubNodesToPaste;
	for (TSet<UEdGraphNode*>::TConstIterator It(NodesToPaste); It; ++It)
	{
		const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(*It);
		if (!ensure(IsValid(FlowGraphNode)))
		{
			return false;
		}

		// Only SubNodes can be pasted onto other nodes.
		if (!FlowGraphNode->IsSubNode())
		{
			return false;
		}

		// Only AddOns copied without their parent attach to the paste target.
		if (!FlowGraphNode->ParentGuidCopy.IsValid())
		{
			AllRootSubNodesToPaste.Add(FlowGraphNode);
		}
	}

	for (TSet<const UEdGraphNode*>::TConstIterator It(AllRootSubNodesToPaste); It; ++It)
	{
		const UFlowGraphNode* NodeToPaste = Cast<UFlowGraphNode>(*It);

		if (!PasteTargetNode.CanAcceptSubNodeAsChild(*NodeToPaste, AllRootSubNodesToPaste))
		{
			// This node cannot accept the SubNode as a child
			return false;
		}
	}

	return true;
}

void SFlowGraphEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool SFlowGraphEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void SFlowGraphEditor::OnNodeDoubleClicked(class UEdGraphNode* Node) const
{
	if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
	{
		FlowGraphNode->OnNodeDoubleClicked();
	}
}

void SFlowGraphEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		const FScopedTransaction Transaction(LOCTEXT("RenameNode", "Rename Node"));
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

void SFlowGraphEditor::ReconstructNode() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->SupportsContextPins())
		{
			SelectedNode->ReconstructNode();
		}
	}
}

bool SFlowGraphEditor::CanReconstructNode() const
{
	if (CanEdit())
	{
		for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
		{
			if (SelectedNode->SupportsContextPins())
			{
				return true;
			}
		}
	}

	return false;
}

void SFlowGraphEditor::ExtractToSubGraph()
{
	FFlowSubGraphExtractionTool::Execute(*this);
}

bool SFlowGraphEditor::CanExtractToSubGraph() const
{
	return CanEdit() && !IsPIE() && IsTabFocused() && !GetSelectedFlowNodes().IsEmpty();
}

bool SFlowGraphEditor::GetValidExecBreakpointPinContext(const UEdGraphPin* Pin, FGuid& OutNodeGuid, FName& OutPinName)
{
	if (!Pin)
	{
		return false;
	}

	if (!FFlowPin::IsExecPinCategory(Pin->PinType.PinCategory))
	{
		return false;
	}

	// - If the owning node is not a UFlowGraphNode, allow it.
	// - If it is a UFlowGraphNode, require it to allow breakpoints.
	const UEdGraphNode* EdNode = Pin->GetOwningNode();
	if (!EdNode)
	{
		return false;
	}

	const UFlowGraphNode* FlowNode = Cast<const UFlowGraphNode>(EdNode);
	if (FlowNode && !FlowNode->CanPlaceBreakpoints())
	{
		return false;
	}

	OutNodeGuid = EdNode->NodeGuid;
	OutPinName = Pin->PinName;
	return true;
}

const FFlowBreakpoint* SFlowGraphEditor::FindPinBreakpoint(UFlowDebuggerSubsystem* InDebuggerSubsystem, const UEdGraphPin* Pin)
{
	if (!InDebuggerSubsystem)
	{
		return nullptr;
	}

	FGuid NodeGuid;
	FName PinName;
	if (!GetValidExecBreakpointPinContext(Pin, NodeGuid, PinName))
	{
		return nullptr;
	}

	return InDebuggerSubsystem->FindBreakpoint(NodeGuid, PinName);
}

bool SFlowGraphEditor::HasPinBreakpoint(UFlowDebuggerSubsystem* InDebuggerSubsystem, const UEdGraphPin* Pin)
{
	return FindPinBreakpoint(InDebuggerSubsystem, Pin) != nullptr;
}

bool SFlowGraphEditor::HasEnabledPinBreakpoint(UFlowDebuggerSubsystem* InDebuggerSubsystem, const UEdGraphPin* Pin)
{
	if (const FFlowBreakpoint* BP = FindPinBreakpoint(InDebuggerSubsystem, Pin))
	{
		return BP->IsEnabled();
	}

	return false;
}

void SFlowGraphEditor::AddInput() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanUserAddInput())
		{
			SelectedNode->AddUserInput();
		}
	}
}

bool SFlowGraphEditor::CanAddInput() const
{
	if (CanEdit())
	{
		for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
		{
			if (SelectedNode->CanUserAddInput())
			{
				return true;
			}
		}
	}

	return false;
}

void SFlowGraphEditor::AddOutput() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanUserAddOutput())
		{
			SelectedNode->AddUserOutput();
		}
	}
}

bool SFlowGraphEditor::CanAddOutput() const
{
	if (CanEdit())
	{
		for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
		{
			if (SelectedNode->CanUserAddOutput())
			{
				return true;
			}
		}
	}

	return false;
}

void SFlowGraphEditor::RemovePin()
{
	if (UEdGraphPin* SelectedPin = GetGraphPinForMenu())
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(SelectedPin->GetOwningNode()))
		{
			SelectedNode->RemoveInstancePin(SelectedPin);
		}
	}
}

bool SFlowGraphEditor::CanRemovePin()
{
	if (CanEdit() && GetSelectedFlowNodes().Num() == 1)
	{
		if (const UEdGraphPin* Pin = GetGraphPinForMenu())
		{
			if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
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

void SFlowGraphEditor::OnAddBreakpoint() const
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystemPtr->AddBreakpoint(SelectedNode->NodeGuid);
		}
	}
}

void SFlowGraphEditor::OnAddPinBreakpoint()
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		FGuid NodeGuid;
		FName PinName;
		if (!GetValidExecBreakpointPinContext(Pin, NodeGuid, PinName))
		{
			return;
		}

		DebuggerSubsystemPtr->AddBreakpoint(NodeGuid, PinName);
	}
}

bool SFlowGraphEditor::CanAddBreakpoint() const
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			if (DebuggerSubsystemPtr->FindBreakpoint(SelectedNode->NodeGuid) == nullptr)
			{
				return true;
			}
		}
	}

	return false;
}

bool SFlowGraphEditor::CanAddPinBreakpoint()
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		FGuid NodeGuid;
		FName PinName;
		if (!GetValidExecBreakpointPinContext(Pin, NodeGuid, PinName))
		{
			return false;
		}

		return DebuggerSubsystemPtr->FindBreakpoint(NodeGuid, PinName) == nullptr;
	}

	return false;
}

void SFlowGraphEditor::OnRemoveBreakpoint() const
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystemPtr->RemoveNodeBreakpoint(SelectedNode->NodeGuid);
		}
	}
}

void SFlowGraphEditor::OnRemovePinBreakpoint()
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		FGuid NodeGuid;
		FName PinName;
		if (!GetValidExecBreakpointPinContext(Pin, NodeGuid, PinName))
		{
			return;
		}

		DebuggerSubsystemPtr->RemovePinBreakpoint(NodeGuid, PinName);
	}
}

bool SFlowGraphEditor::CanRemoveBreakpoint() const
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			if (DebuggerSubsystemPtr->FindBreakpoint(SelectedNode->NodeGuid) != nullptr)
			{
				return true;
			}
		}
	}

	return false;
}

bool SFlowGraphEditor::CanRemovePinBreakpoint()
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	return HasPinBreakpoint(DebuggerSubsystemPtr,GetGraphPinForMenu());
}

void SFlowGraphEditor::OnEnableBreakpoint() const
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystemPtr->SetBreakpointEnabled(SelectedNode->NodeGuid, true);
		}
	}
}

void SFlowGraphEditor::OnEnablePinBreakpoint()
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		FGuid NodeGuid;
		FName PinName;
		if (!GetValidExecBreakpointPinContext(Pin, NodeGuid, PinName))
		{
			return;
		}

		DebuggerSubsystemPtr->SetBreakpointEnabled(NodeGuid, PinName, true);
	}
}

bool SFlowGraphEditor::CanEnableBreakpoint() const
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			const FFlowBreakpoint* Breakpoint = DebuggerSubsystemPtr->FindBreakpoint(SelectedNode->NodeGuid);
			if (Breakpoint && !Breakpoint->IsEnabled())
			{
				return true;
			}
		}
	}

	return false;
}

bool SFlowGraphEditor::CanEnablePinBreakpoint()
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	return HasPinBreakpoint(DebuggerSubsystemPtr,GetGraphPinForMenu())
		&& !HasEnabledPinBreakpoint(DebuggerSubsystemPtr,GetGraphPinForMenu());
}

void SFlowGraphEditor::OnDisableBreakpoint() const
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystemPtr->SetBreakpointEnabled(SelectedNode->NodeGuid, false);
		}
	}
}

void SFlowGraphEditor::OnDisablePinBreakpoint()
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		FGuid NodeGuid;
		FName PinName;
		if (!GetValidExecBreakpointPinContext(Pin, NodeGuid, PinName))
		{
			return;
		}

		DebuggerSubsystemPtr->SetBreakpointEnabled(NodeGuid, PinName, false);
	}
}

bool SFlowGraphEditor::CanDisableBreakpoint() const
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			const FFlowBreakpoint* Breakpoint = DebuggerSubsystemPtr->FindBreakpoint(SelectedNode->NodeGuid);
			if (Breakpoint && Breakpoint->IsEnabled())
			{
				return true;
			}
		}
	}

	return false;
}

bool SFlowGraphEditor::CanDisablePinBreakpoint()
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	return HasEnabledPinBreakpoint(DebuggerSubsystemPtr,GetGraphPinForMenu());
}

void SFlowGraphEditor::OnToggleBreakpoint() const
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystemPtr->ToggleBreakpoint(SelectedNode->NodeGuid);
		}
	}
}

void SFlowGraphEditor::OnTogglePinBreakpoint()
{
	UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get();
	check(DebuggerSubsystemPtr);

	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		FGuid NodeGuid;
		FName PinName;
		if (!GetValidExecBreakpointPinContext(Pin, NodeGuid, PinName))
		{
			return;
		}

		DebuggerSubsystemPtr->ToggleBreakpoint(NodeGuid, PinName);
	}
}

bool SFlowGraphEditor::CanToggleBreakpoint() const
{
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			return true;
		}
	}

	return false;
}

bool SFlowGraphEditor::CanTogglePinBreakpoint()
{
	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		FGuid NodeGuid;
		FName PinName;
		return GetValidExecBreakpointPinContext(Pin, NodeGuid, PinName);
	}

	return false;
}

void SFlowGraphEditor::EnableAllBreakpoints() const
{
	if (UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get())
	{
		DebuggerSubsystemPtr->SetAllBreakpointsEnabled(FlowAsset, true);
	}
}

bool SFlowGraphEditor::HasAnyDisabledBreakpoints() const
{
	return DebuggerSubsystem.IsValid() && UFlowDebuggerSubsystem::HasAnyBreakpointsDisabled(FlowAsset);
}

void SFlowGraphEditor::DisableAllBreakpoints() const
{
	if (UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get())
	{
		DebuggerSubsystemPtr->SetAllBreakpointsEnabled(FlowAsset, false);
	}
}

bool SFlowGraphEditor::HasAnyEnabledBreakpoints() const
{
	return DebuggerSubsystem.IsValid() && UFlowDebuggerSubsystem::HasAnyBreakpointsEnabled(FlowAsset);
}

void SFlowGraphEditor::RemoveAllBreakpoints() const
{
	if (UFlowDebuggerSubsystem* DebuggerSubsystemPtr =DebuggerSubsystem.Get())
	{
		DebuggerSubsystemPtr->RemoveAllBreakpoints(FlowAsset);
	}
}

bool SFlowGraphEditor::HasAnyBreakpoints() const
{
	return DebuggerSubsystem.IsValid() && UFlowDebuggerSubsystem::HasAnyBreakpoints(FlowAsset);
}

void SFlowGraphEditor::SetSignalMode(const EFlowSignalMode Mode) const
{
	const FScopedTransaction Transaction(LOCTEXT("SetSignalMode", "Set Signal Mode"));

	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanSetSignalMode(Mode))
		{
			SelectedNode->SetSignalMode(Mode);
		}
	}

	FlowAsset->Modify();
}

bool SFlowGraphEditor::CanSetSignalMode(const EFlowSignalMode Mode) const
{
	if (IsPIE())
	{
		return false;
	}

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanSetSignalMode(Mode))
		{
			return true;
		}
	}

	return false;
}

void SFlowGraphEditor::OnForcePinActivation()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->ForcePinActivation(Pin);
		}
	}
}

void SFlowGraphEditor::FocusViewport() const
{
	const TArray<UFlowGraphNode*> SelectedNodes = GetSelectedFlowNodes();
	if (SelectedNodes.Num() == 1)
	{
		const UFlowGraphNode* SelectedNode = SelectedNodes[0];
		if (const UFlowNode* FlowNode = Cast<UFlowNode>(SelectedNode->GetFlowNodeBase()))
		{
			if (UFlowNode* InspectedInstance = FlowNode->GetInspectedInstance())
			{
				if (AActor* ActorToFocus = InspectedInstance->GetActorToFocus())
				{
					GEditor->SelectNone(false, false, false);
					GEditor->SelectActor(ActorToFocus, true, true, true);
					GEditor->NoteSelectionChange();

					GEditor->MoveViewportCamerasToActor(*ActorToFocus, false);

					const FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
					const TSharedPtr<SDockTab> LevelEditorTab = LevelEditorModule.GetLevelEditorInstanceTab().Pin();
					if (LevelEditorTab.IsValid())
					{
						LevelEditorTab->DrawAttention();
					}
				}
			}
		}
	}
}

bool SFlowGraphEditor::CanFocusViewport() const
{
	return GetSelectedFlowNodes().Num() == 1;
}

void SFlowGraphEditor::JumpToNodeDefinition() const
{
	const TArray<UFlowGraphNode*> SelectedNodes = GetSelectedFlowNodes();
	if (SelectedNodes.Num() == 1)
	{
		const UFlowGraphNode* SelectedNode = SelectedNodes[0];
		SelectedNode->JumpToDefinition();
	}
}

bool SFlowGraphEditor::CanJumpToNodeDefinition() const
{
	return GetSelectedFlowNodes().Num() == 1;
}

#undef LOCTEXT_NAMESPACE
