// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphEditor.h"

#include "Asset/FlowAssetEditor.h"
#include "FlowEditorCommands.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "Debugger/FlowDebuggerSubsystem.h"

#include "EdGraphUtilities.h"
#include "Editor/UnrealEdEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "UnrealEdGlobals.h"
#include "Widgets/Docking/SDockTab.h"
#include "Algo/AnyOf.h"

#define LOCTEXT_NAMESPACE "FlowGraphEditor"

void SFlowGraphEditor::Construct(const FArguments& InArgs, const TSharedPtr<FFlowAssetEditor> InAssetEditor)
{
	FlowAssetEditor = InAssetEditor;
	FlowAsset = FlowAssetEditor.Pin()->GetFlowAsset();
	DetailsView = InArgs._DetailsView;

	DebuggerSubsystem = GEngine->GetEngineSubsystem<UFlowDebuggerSubsystem>();

	BindGraphCommands();

	SGraphEditor::FArguments Arguments;
	Arguments._AdditionalCommands = CommandList;
	Arguments._Appearance = GetGraphAppearanceInfo();
	Arguments._GraphToEdit = FlowAsset->GetGraph();
	Arguments._GraphEvents = InArgs._GraphEvents;
	Arguments._AutoExpandActionMenu = true;
	Arguments._GraphEvents.OnSelectionChanged = FOnSelectionChanged::CreateSP(this, &SFlowGraphEditor::OnSelectedNodesChanged);
	Arguments._GraphEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &SFlowGraphEditor::OnNodeDoubleClicked);
	Arguments._GraphEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &SFlowGraphEditor::OnNodeTitleCommitted);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
	Arguments._GraphEvents.OnSpawnNodeByShortcut = FOnSpawnNodeByShortcut::CreateStatic(&SFlowGraphEditor::OnSpawnGraphNodeByShortcut, static_cast<UEdGraph*>(FlowAsset->GetGraph()));
#else
	Arguments._GraphEvents.OnSpawnNodeByShortcutAtLocation = FOnSpawnNodeByShortcutAtLocation::CreateStatic(&SFlowGraphEditor::OnSpawnGraphNodeByShortcut, static_cast<UEdGraph*>(FlowAsset->GetGraph()));
#endif

	SGraphEditor::Construct(Arguments);
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
	                               FCanExecuteAction::CreateStatic(&SFlowGraphEditor::CanEdit));

	CommandList->MapAction(GraphEditorCommands.StraightenConnections,
	                               FExecuteAction::CreateSP(this, &SFlowGraphEditor::OnStraightenConnections));
	
	CommandList->MapAction(GraphEditorCommands.DeleteAndReconnectNodes,
							   FExecuteAction::CreateSP(this, &SFlowGraphEditor::DeleteSelectedNodes),
							   FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanDeleteNodes));

	// Generic Node commands
	CommandList->MapAction(GenericCommands.Undo,
	                               FExecuteAction::CreateStatic(&SFlowGraphEditor::UndoGraphAction),
	                               FCanExecuteAction::CreateStatic(&SFlowGraphEditor::CanEdit));

	CommandList->MapAction(GenericCommands.Redo,
	                               FExecuteAction::CreateStatic(&SFlowGraphEditor::RedoGraphAction),
	                               FCanExecuteAction::CreateStatic(&SFlowGraphEditor::CanEdit));

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

	// Organisation Commands
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
}

FGraphAppearanceInfo SFlowGraphEditor::GetGraphAppearanceInfo() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = GetCornerText();

	if (IsPlaySessionPaused())
	{
		AppearanceInfo.PIENotifyText = LOCTEXT("PausedLabel", "PAUSED");
	}

	return AppearanceInfo;
}

FText SFlowGraphEditor::GetCornerText() const
{
	return LOCTEXT("AppearanceCornerText_FlowAsset", "FLOW");
}

void SFlowGraphEditor::UndoGraphAction()
{
	GEditor->UndoTransaction();
}

void SFlowGraphEditor::RedoGraphAction()
{
	GEditor->RedoTransaction();
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
FReply SFlowGraphEditor::OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph)
#else
FReply SFlowGraphEditor::OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2f& InPosition, UEdGraph* InGraph)
#endif
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
	CommentAction.PerformAction(FlowAsset->GetGraph(), nullptr, GetPasteLocation());
#else
	CommentAction.PerformAction(FlowAsset->GetGraph(), nullptr, GetPasteLocation2f());
#endif
}

bool SFlowGraphEditor::IsTabFocused() const
{
	return FlowAssetEditor.Pin()->IsTabFocused(FFlowAssetEditor::GraphTab);
}

bool SFlowGraphEditor::CanEdit()
{
	return GEditor->PlayWorld == nullptr;
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
				SelectedObjects.Add(Cast<UObject>(GraphNode->GetFlowNodeBase()));
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

TSet<UFlowGraphNode*> SFlowGraphEditor::GetSelectedFlowNodes() const
{
	TSet<UFlowGraphNode*> Result;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UFlowGraphNode* SelectedNode = Cast<UFlowGraphNode>(*NodeIt))
		{
			Result.Emplace(SelectedNode);
		}
	}

	return Result;
}

void SFlowGraphEditor::ReconnectExecPins(const UFlowGraphNode* Node)
{
	if(Node == nullptr)
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
	FlowAsset->Modify();

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	FlowAssetEditor.Pin()->SetUISelectionState(NAME_None);

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = CastChecked<UEdGraphNode>(*NodeIt);
		if (Node->CanUserDeleteNode())
		{
			if (DebuggerSubsystem.IsValid())
			{
				DebuggerSubsystem->RemoveAllBreakpoints(Node->NodeGuid);
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

					FlowAsset->UnregisterNode(FlowNode->GetGuid());
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

void SFlowGraphEditor::CopySelectedNodes() const
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	FGraphPanelSelectionSet NewSelectedNodes;

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(*SelectedIter))
		{
			constexpr int32 RootEdNodeParentIndex = INDEX_NONE;
			PrepareFlowGraphNodeForCopy(*FlowGraphNode, RootEdNodeParentIndex, NewSelectedNodes);
		}
		else
		{
			NewSelectedNodes.Add(*SelectedIter);
		}
	}

	FString ExportedText;

	FEdGraphUtilities::ExportNodesToText(NewSelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	for (FGraphPanelSelectionSet::TIterator SelectedIter(NewSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(*SelectedIter))
		{
			FlowGraphNode->PostCopyNode();
		}
	}
}

void SFlowGraphEditor::PrepareFlowGraphNodeForCopy(UFlowGraphNode& FlowGraphNode, const int32 ParentEdNodeIndex, FGraphPanelSelectionSet& NewSelectedNodes)
{
	const int32 ThisFlowGraphNodeIndex = NewSelectedNodes.Num();
	bool bAlreadyInSet = false;
	NewSelectedNodes.Add(&FlowGraphNode, &bAlreadyInSet);

	if (bAlreadyInSet)
	{
		return;
	}

	FlowGraphNode.PrepareForCopying();
	FlowGraphNode.CopySubNodeParentIndex = ParentEdNodeIndex;
	FlowGraphNode.CopySubNodeIndex = ThisFlowGraphNodeIndex;

	// append all subnodes for selection
	for (UFlowGraphNode* SubNode : FlowGraphNode.SubNodes)
	{
		if (SubNode)
		{
			PrepareFlowGraphNodeForCopy(*SubNode, ThisFlowGraphNodeIndex, NewSelectedNodes);
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

void SFlowGraphEditor::PasteNodes()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
	PasteNodesHere(GetPasteLocation());
#else
	PasteNodesHere(GetPasteLocation2f());
#endif
}

void SFlowGraphEditor::PasteNodesHere(const FVector2D& Location)
{
	// Undo/Redo support
	const FScopedTransaction Transaction(LOCTEXT("PasteNode", "Paste Node"));
	UFlowGraph* FlowGraph = CastChecked<UFlowGraph>(FlowAsset->GetGraph());
	FlowGraph->Modify();
	FlowAsset->Modify();

	FlowGraph->LockUpdates();

	const TArray<UFlowGraphNode*> PasteTargetNodes = DerivePasteTargetNodesFromSelectedNodes();
	if (Algo::AnyOf(PasteTargetNodes, [](const UFlowGraphNode* Node) { return Node && !Node->SubNodes.IsEmpty(); }))
	{
		checkf(PasteTargetNodes.Num() <= 1, TEXT("This should be enforced in CanPasteNodes()"));
	}

	UFlowGraphNode* PasteTargetNode = !PasteTargetNodes.IsEmpty() ? PasteTargetNodes.Top() : nullptr;

	FString TextToImport;
	const TSet<UEdGraphNode*> NodesToPaste = ImportNodesToPasteFromClipboard(*FlowGraph, TextToImport);

	// Clear the selection set (newly pasted stuff will be selected)
	ClearSelectionSet();
	FlowAssetEditor.Pin()->SetUISelectionState(NAME_None);

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);

	// Number of nodes used to calculate AvgNodePosition
	int32 AvgCount = 0;

	for (TSet<UEdGraphNode*>::TConstIterator It(NodesToPaste); It; ++It)
	{
		UEdGraphNode* EdNode = *It;
		UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(EdNode);
		if (EdNode && (FlowGraphNode == nullptr || !FlowGraphNode->IsSubNode()))
		{
			AvgNodePosition.X += EdNode->NodePosX;
			AvgNodePosition.Y += EdNode->NodePosY;
			++AvgCount;
		}
	}

	if (AvgCount > 0)
	{
		float InvNumNodes = 1.0f / static_cast<float>(AvgCount);
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;
	}

	TMap<int32, UFlowGraphNode*> EdNodeCopyIndexMap;
	for (TSet<UEdGraphNode*>::TConstIterator It(NodesToPaste); It; ++It)
	{
		UEdGraphNode* PastedNode = *It;

		UFlowGraphNode* PastedFlowGraphNode = Cast<UFlowGraphNode>(PastedNode);
		if (PastedFlowGraphNode)
		{
			EdNodeCopyIndexMap.Add(PastedFlowGraphNode->CopySubNodeIndex, PastedFlowGraphNode);
		}

		if (PastedNode && (PastedFlowGraphNode == nullptr || !PastedFlowGraphNode->IsSubNode()))
		{
			// Select the newly pasted stuff
			constexpr bool bSelectNodes = true;
			SetNodeSelection(PastedNode, bSelectNodes);

			PastedNode->NodePosX = (PastedNode->NodePosX - AvgNodePosition.X) + Location.X;
			PastedNode->NodePosY = (PastedNode->NodePosY - AvgNodePosition.Y) + Location.Y;

			PastedNode->SnapToGrid(16);

			// Give new node a different Guid from the old one
			PastedNode->CreateNewGuid();
		}

		if (PastedFlowGraphNode)
		{
			if (UFlowNode* FlowNode = Cast<UFlowNode>(PastedFlowGraphNode->GetFlowNodeBase()))
			{
				// Only full FlowNodes are registered with the Asset
				// (for now?  perhaps we register AddOns in the future?)
				FlowAsset->RegisterNode(PastedNode->NodeGuid, FlowNode);
			}
			
			PastedFlowGraphNode->RemoveAllSubNodes();
		}
	}

	for (TSet<UEdGraphNode*>::TConstIterator It(NodesToPaste); It; ++It)
	{
		UFlowGraphNode* PasteNode = Cast<UFlowGraphNode>(*It);
		if (PasteNode && PasteNode->IsSubNode())
		{
			PasteNode->NodePosX = 0;
			PasteNode->NodePosY = 0;

			// remove subnode from graph, it will be referenced from parent node
			PasteNode->DestroyNode();

			if (PasteNode->CopySubNodeParentIndex == INDEX_NONE)
			{
				// INDEX_NONE parent index indicates we should set the parent to the PasteTargetNode
				if (PasteTargetNode)
				{
					PasteTargetNode->AddSubNode(PasteNode, FlowGraph);
				}
			}
			else if (UFlowGraphNode* PastedParentNode = EdNodeCopyIndexMap.FindRef(PasteNode->CopySubNodeParentIndex))
			{
				PastedParentNode->AddSubNode(PasteNode, FlowGraph);
			}
		}
	}

	if (FlowGraph)
	{
		FlowGraph->UpdateClassData();
		FlowGraph->OnNodesPasted(TextToImport);
		FlowGraph->UnlockUpdates();
	}

	// Update UI
	NotifyGraphChanged();

	if (UObject* GraphOwner = FlowGraph->GetOuter())
	{
		GraphOwner->PostEditChange();
		GraphOwner->MarkPackageDirty();
	}
}

TSet<UEdGraphNode*> SFlowGraphEditor::ImportNodesToPasteFromClipboard(UFlowGraph& FlowGraph, FString& OutTextToImport)
{
	// Grab the text to paste from the clipboard.
	FPlatformApplicationMisc::ClipboardPaste(OutTextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> NodesToPaste;
	FEdGraphUtilities::ImportNodesFromText(&FlowGraph, OutTextToImport, /*out*/ NodesToPaste);

	return NodesToPaste;
}

TArray<UFlowGraphNode*> SFlowGraphEditor::DerivePasteTargetNodesFromSelectedNodes() const
{
	TArray<UFlowGraphNode*> PasteTargetNodes;
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UFlowGraphNode* Node = Cast<UFlowGraphNode>(*SelectedIter);
		if (IsValid(Node))
		{
			PasteTargetNodes.Add(Node);
		}
	}

	return PasteTargetNodes;
}

bool SFlowGraphEditor::CanPasteNodes() const
{
	if (!CanEdit() || !IsTabFocused())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	UFlowGraph* FlowGraph = CastChecked<UFlowGraph>(FlowAsset->GetGraph());
	if (!ensure(IsValid(FlowGraph)))
	{
		// We expect to have a legal FlowGraph pointer at this point
		return false;
	}

	const bool bIsPastePossible = FEdGraphUtilities::CanImportNodesFromText(FlowGraph, ClipboardContent);
	if (!bIsPastePossible)
	{
		return false;
	}

	// Disallow paste when multiple target nodes are selected, and if there are subnodes involved.
	const TArray<UFlowGraphNode*> PasteTargetNodes = DerivePasteTargetNodesFromSelectedNodes();
	const bool bHasSubNodes = Algo::AnyOf(PasteTargetNodes, [](const UFlowGraphNode* Node) { return Node && !Node->SubNodes.IsEmpty(); });

	if (bHasSubNodes && PasteTargetNodes.Num() > 1)
	{
		// NOTE (gtaylor) It's possible we could support multi-paste, but we'd need to rework PasteNodesHere()
		// to understand how to paste copies onto each target node.
		return false;
	}

	FString TextToImport;
	const TSet<UEdGraphNode*> NodesToPaste = ImportNodesToPasteFromClipboard(*FlowGraph, TextToImport);

	if (NodesToPaste.IsEmpty())
	{
		// Must have at least one node to paste
		return false;
	}

	ON_SCOPE_EXIT
	{
		// We need to clean up the nodes we built to test the paste operation
		for (TSet<UEdGraphNode*>::TConstIterator It(NodesToPaste); It; ++It)
		{
			UFlowGraphNode* NodeToPaste = Cast<UFlowGraphNode>(*It);
			if (IsValid(NodeToPaste))
			{
				NodeToPaste->ClearFlags(RF_Public);
				NodeToPaste->SetFlags(RF_Transient);

				const FString NewNameStr = MakeUniqueObjectName(NodeToPaste->GetOuter(), NodeToPaste->GetClass()).ToString();

				// This will remove the node from its graph
				NodeToPaste->DestroyNode();

				// Rename and garbage the node so that it can't be found by name if the same clipboard is re-pasted
				NodeToPaste->Rename(*NewNameStr, nullptr, REN_NonTransactional | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
				NodeToPaste->MarkAsGarbage();
			}
		}
	};

	// If pasting onto a selected node, confirm that the paste operation is legal
	if (bHasSubNodes && PasteTargetNodes.Num() >= 1)
	{
		checkf(PasteTargetNodes.Num() == 1, TEXT("This is enforced earlier in this function, just confirming the code stays that way here."));

		const UFlowGraphNode* PasteTargetNode = PasteTargetNodes.Top();
		if (!CanPasteNodesAsSubNodes(NodesToPaste, *PasteTargetNode))
		{
			return false;
		}
	}

	return true;
}

bool SFlowGraphEditor::CanPasteNodesAsSubNodes(const TSet<UEdGraphNode*>& NodesToPaste, const UFlowGraphNode& PasteTargetNode)
{
	TSet<const UEdGraphNode*> AllRootSubNodesToPaste;
	for (TSet<UEdGraphNode*>::TConstIterator It(NodesToPaste); It; ++It)
	{
		const UFlowGraphNode* NodeToPaste = Cast<UFlowGraphNode>(*It);
		if (!ensure(IsValid(NodeToPaste)))
		{
			return false;
		}

		if (!NodeToPaste->IsSubNode())
		{
			// Only SubNodes can be pasted onto other nodes

			return false;
		}

		// Only concerned with the 'root' subnodes
		// (we assume the rest of the subnode tree is valid when put into the copy buffer)
		if (NodeToPaste->CopySubNodeParentIndex != INDEX_NONE)
		{
			// a non-INDEX_NONE parent index indicates the subnode is a non-root subnode in the NodesToPaste set

			continue;
		}

		AllRootSubNodesToPaste.Add(NodeToPaste);
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
		SelectedNode->ReconstructNode();
	}
}

bool SFlowGraphEditor::CanReconstructNode() const
{
	if (CanEdit() && GetSelectedFlowNodes().Num() == 1)
	{
		for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
		{
			return SelectedNode->SupportsContextPins();
		}
	}

	return false;
}

void SFlowGraphEditor::AddInput() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->AddUserInput();
	}
}

bool SFlowGraphEditor::CanAddInput() const
{
	if (CanEdit() && GetSelectedFlowNodes().Num() == 1)
	{
		for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
		{
			return SelectedNode->CanUserAddInput();
		}
	}

	return false;
}

void SFlowGraphEditor::AddOutput() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->AddUserOutput();
	}
}

bool SFlowGraphEditor::CanAddOutput() const
{
	if (CanEdit() && GetSelectedFlowNodes().Num() == 1)
	{
		for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
		{
			return SelectedNode->CanUserAddOutput();
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
	check(DebuggerSubsystem.IsValid());
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystem->AddBreakpoint(SelectedNode->NodeGuid);
		}
	}
}

void SFlowGraphEditor::OnAddPinBreakpoint()
{
	check(DebuggerSubsystem.IsValid());
	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		const UFlowGraphNode* OwningNode = Cast<const UFlowGraphNode>(Pin->GetOwningNode());
		if (!OwningNode || OwningNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystem->AddBreakpoint(Pin->GetOwningNode()->NodeGuid, Pin->PinName);
		}
	}
}

bool SFlowGraphEditor::CanAddBreakpoint() const
{
	check(DebuggerSubsystem.IsValid());
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			if (DebuggerSubsystem->FindBreakpoint(SelectedNode->NodeGuid) == nullptr)
			{
				return true;
			}
		}
	}

	return false;
}

bool SFlowGraphEditor::CanAddPinBreakpoint()
{
	check(DebuggerSubsystem.IsValid());
	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		const UFlowGraphNode* OwningNode = Cast<const UFlowGraphNode>(Pin->GetOwningNode());
		if (!OwningNode || OwningNode->CanPlaceBreakpoints())
		{
			return DebuggerSubsystem->FindBreakpoint(Pin->GetOwningNode()->NodeGuid, Pin->PinName) == nullptr;
		}
	}

	return false;
}

void SFlowGraphEditor::OnRemoveBreakpoint() const
{
	check(DebuggerSubsystem.IsValid());
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystem->RemoveNodeBreakpoint(SelectedNode->NodeGuid);
		}
	}
}

void SFlowGraphEditor::OnRemovePinBreakpoint()
{
	check(DebuggerSubsystem.IsValid());
	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		const UFlowGraphNode* OwningNode = Cast<const UFlowGraphNode>(Pin->GetOwningNode());
		if (!OwningNode || OwningNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystem->RemovePinBreakpoint(Pin->GetOwningNode()->NodeGuid, Pin->PinName);
		}
	}
}

bool SFlowGraphEditor::CanRemoveBreakpoint() const
{
	check(DebuggerSubsystem.IsValid());
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			if (DebuggerSubsystem->FindBreakpoint(SelectedNode->NodeGuid) != nullptr)
			{
				return true;
			}
		}
	}

	return false;
}

bool SFlowGraphEditor::CanRemovePinBreakpoint()
{
	check(DebuggerSubsystem.IsValid());
	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		const UFlowGraphNode* OwningNode = Cast<const UFlowGraphNode>(Pin->GetOwningNode());
		if (!OwningNode || OwningNode->CanPlaceBreakpoints())
		{
			return DebuggerSubsystem->FindBreakpoint(Pin->GetOwningNode()->NodeGuid, Pin->PinName) != nullptr;
		}
	}

	return false;
}

void SFlowGraphEditor::OnEnableBreakpoint() const
{
	check(DebuggerSubsystem.IsValid());
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystem->SetBreakpointEnabled(SelectedNode->NodeGuid, true);
		}
	}
}

void SFlowGraphEditor::OnEnablePinBreakpoint()
{
	check(DebuggerSubsystem.IsValid());
	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		const UFlowGraphNode* OwningNode = Cast<const UFlowGraphNode>(Pin->GetOwningNode());
		if (!OwningNode || OwningNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystem->SetBreakpointEnabled(Pin->GetOwningNode()->NodeGuid, Pin->PinName, true);
		}
	}
}

bool SFlowGraphEditor::CanEnableBreakpoint() const
{
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			const FFlowBreakpoint* Breakpoint = DebuggerSubsystem->FindBreakpoint(SelectedNode->NodeGuid);
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
	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		const UFlowGraphNode* OwningNode = Cast<const UFlowGraphNode>(Pin->GetOwningNode());
		if (!OwningNode || OwningNode->CanPlaceBreakpoints())
		{
			const FFlowBreakpoint* Breakpoint = DebuggerSubsystem->FindBreakpoint(Pin->GetOwningNode()->NodeGuid, Pin->PinName);
			return Breakpoint && !Breakpoint->IsEnabled();
		}
	}

	return false;
}

void SFlowGraphEditor::OnDisableBreakpoint() const
{
	check(DebuggerSubsystem.IsValid());
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystem->SetBreakpointEnabled(SelectedNode->NodeGuid, false);
		}
	}
}

void SFlowGraphEditor::OnDisablePinBreakpoint()
{
	check(DebuggerSubsystem.IsValid());
	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		const UFlowGraphNode* OwningNode = Cast<const UFlowGraphNode>(Pin->GetOwningNode());
		if (!OwningNode || OwningNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystem->SetBreakpointEnabled(Pin->GetOwningNode()->NodeGuid, Pin->PinName, false);
		}
	}
}

bool SFlowGraphEditor::CanDisableBreakpoint() const
{
	check(DebuggerSubsystem.IsValid());
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			const FFlowBreakpoint* Breakpoint = DebuggerSubsystem->FindBreakpoint(SelectedNode->NodeGuid);
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
	check(DebuggerSubsystem.IsValid());
	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		const UFlowGraphNode* OwningNode = Cast<const UFlowGraphNode>(Pin->GetOwningNode());
		if (!OwningNode || OwningNode->CanPlaceBreakpoints())
		{
			const FFlowBreakpoint* Breakpoint = DebuggerSubsystem->FindBreakpoint(Pin->GetOwningNode()->NodeGuid, Pin->PinName);
			return Breakpoint && Breakpoint->IsEnabled();
		}
	}

	return false;
}

void SFlowGraphEditor::OnToggleBreakpoint() const
{
	check(DebuggerSubsystem.IsValid());
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		if (SelectedNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystem->ToggleBreakpoint(SelectedNode->NodeGuid);
		}
	}
}

void SFlowGraphEditor::OnTogglePinBreakpoint()
{
	check(DebuggerSubsystem.IsValid());
	if (const UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		const UFlowGraphNode* OwningNode = Cast<const UFlowGraphNode>(Pin->GetOwningNode());
		if (!OwningNode || OwningNode->CanPlaceBreakpoints())
		{
			DebuggerSubsystem->ToggleBreakpoint(Pin->GetOwningNode()->NodeGuid, Pin->PinName);
		}
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
		const UFlowGraphNode* OwningNode = Cast<const UFlowGraphNode>(Pin->GetOwningNode());
		if (!OwningNode || OwningNode->CanPlaceBreakpoints())
		{
			return true;
		}
	}

	return false;
}

void SFlowGraphEditor::SetSignalMode(const EFlowSignalMode Mode) const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->SetSignalMode(Mode);
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
		return SelectedNode->CanSetSignalMode(Mode);
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
	// Iterator used but should only contain one node
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		const UFlowNode* FlowNode = Cast<UFlowNode>(SelectedNode->GetFlowNodeBase());
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

		return;
	}
}

bool SFlowGraphEditor::CanFocusViewport() const
{
	return GetSelectedFlowNodes().Num() == 1;
}

void SFlowGraphEditor::JumpToNodeDefinition() const
{
	// Iterator used but should only contain one node
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->JumpToDefinition();
		return;
	}
}

bool SFlowGraphEditor::CanJumpToNodeDefinition() const
{
	return GetSelectedFlowNodes().Num() == 1;
}

#undef LOCTEXT_NAMESPACE
