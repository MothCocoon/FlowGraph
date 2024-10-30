// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphEditor.h"

#include "FlowEditorCommands.h"
#include "FlowEditorModule.h"

#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowDebuggerSubsystem.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Nodes/Route/FlowNode_SubGraph.h"

#include "EdGraphUtilities.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FlowGraphEditor"

void SFlowGraphEditor::Construct(const FArguments& InArgs, const TSharedPtr<FFlowAssetEditor> InAssetEditor)
{
	FlowAssetEditor = InAssetEditor;
	FlowAsset = FlowAssetEditor.Pin()->GetFlowAsset();

	DetailsView = InArgs._DetailsView;

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
	Arguments._GraphEvents.OnSpawnNodeByShortcut = FOnSpawnNodeByShortcut::CreateStatic(&SFlowGraphEditor::OnSpawnGraphNodeByShortcut, static_cast<UEdGraph*>(FlowAsset->GetGraph()));

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
	CommandList->MapAction(FlowGraphCommands.RefreshContextPins,
	                               FExecuteAction::CreateSP(this, &SFlowGraphEditor::RefreshContextPins),
	                               FCanExecuteAction::CreateSP(this, &SFlowGraphEditor::CanRefreshContextPins));

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

	if (UFlowDebuggerSubsystem::IsPlaySessionPaused())
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

FReply SFlowGraphEditor::OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph)
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
	CommentAction.PerformAction(FlowAsset->GetGraph(), nullptr, GetPasteLocation());
}

bool SFlowGraphEditor::CanEdit()
{
	return GEditor->PlayWorld == nullptr;
}

bool SFlowGraphEditor::IsPIE()
{
	return GEditor->PlayWorld != nullptr;
}

bool SFlowGraphEditor::IsTabFocused() const
{
	return FlowAssetEditor.Pin()->IsTabFocused(FFlowAssetEditor::GraphTab);
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
			if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
			{
				if (const UFlowNode* FlowNode = Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase()))
				{
					const FGuid NodeGuid = FlowNode->GetGuid();
					
					// If the user is pressing shift then try and reconnect the pins
					if (FSlateApplication::Get().GetModifierKeys().IsShiftDown())
					{
						ReconnectExecPins(FlowGraphNode);
					}

					GetCurrentGraph()->GetSchema()->BreakNodeLinks(*Node);
					Node->DestroyNode();

					FlowAsset->UnregisterNode(NodeGuid);
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
	}

	FString ExportedText;

	FEdGraphUtilities::ExportNodesToText(NewSelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	for (FGraphPanelSelectionSet::TIterator SelectedIter(NewSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(*SelectedIter);
		if (FlowGraphNode)
		{
			FlowGraphNode->PostCopyNode();
		}
	}
}

void SFlowGraphEditor::PrepareFlowGraphNodeForCopy(UFlowGraphNode& FlowGraphNode, int32 ParentEdNodeIndex, FGraphPanelSelectionSet& NewSelectedNodes) const
{
	FlowGraphNode.PrepareForCopying();

	FlowGraphNode.CopySubNodeParentIndex = ParentEdNodeIndex;

	const int32 ThisFlowGraphNodeIndex = NewSelectedNodes.Num();
	FlowGraphNode.CopySubNodeIndex = ThisFlowGraphNodeIndex;
	NewSelectedNodes.Add(&FlowGraphNode);

	// append all subnodes for selection
	const TArray<UFlowGraphNode*>& FlowGraphNodeSubNodes = FlowGraphNode.SubNodes;

	for (int32 Idx = 0; Idx < FlowGraphNodeSubNodes.Num(); ++Idx)
	{
		UFlowGraphNode* SubNodeCur = FlowGraphNodeSubNodes[Idx];
		if (SubNodeCur)
		{
			PrepareFlowGraphNodeForCopy(*SubNodeCur, ThisFlowGraphNodeIndex, NewSelectedNodes);
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
	PasteNodesHere(GetPasteLocation());
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
	checkf(PasteTargetNodes.Num() <= 1, TEXT("This should be enforced in CanPasteNodes()"));

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
		float InvNumNodes = 1.0f / float(AvgCount);
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;
	}

	bool bPastedParentNode = false;

	TMap<int32, UFlowGraphNode*> EdNodeCopyIndexMap;
	for (TSet<UEdGraphNode*>::TConstIterator It(NodesToPaste); It; ++It)
	{
		UEdGraphNode* PasteNode = *It;
		UFlowGraphNode* PasteFlowGraphNode = Cast<UFlowGraphNode>(PasteNode);

		EdNodeCopyIndexMap.Add(PasteFlowGraphNode->CopySubNodeIndex, PasteFlowGraphNode);

		if (PasteNode && (PasteFlowGraphNode == nullptr || !PasteFlowGraphNode->IsSubNode()))
		{
			bPastedParentNode = true;

			// Select the newly pasted stuff
			constexpr bool bSelectNodes = true;
			SetNodeSelection(PasteNode, bSelectNodes);

			PasteNode->NodePosX = (PasteNode->NodePosX - AvgNodePosition.X) + Location.X;
			PasteNode->NodePosY = (PasteNode->NodePosY - AvgNodePosition.Y) + Location.Y;

			PasteNode->SnapToGrid(16);

			// Give new node a different Guid from the old one
			PasteNode->CreateNewGuid();

			if (UFlowNode* FlowNode = Cast<UFlowNode>(PasteFlowGraphNode->GetFlowNodeBase()))
			{
				// Only full FlowNodes are registered with the Asset
				// (for now?  perhaps we register AddOns in the future?)
				FlowAsset->RegisterNode(PasteNode->NodeGuid, FlowNode);
			}
		}

		if (PasteFlowGraphNode)
		{
			PasteFlowGraphNode->RemoveAllSubNodes();
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

	UObject* GraphOwner = FlowGraph->GetOuter();
	if (GraphOwner)
	{
		GraphOwner->PostEditChange();
		GraphOwner->MarkPackageDirty();
	}
}

TSet<UEdGraphNode*> SFlowGraphEditor::ImportNodesToPasteFromClipboard(UFlowGraph& FlowGraph, FString& OutTextToImport) const
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

	// Disallow paste when multiple target nodes are selected.
	const TArray<UFlowGraphNode*> PasteTargetNodes = DerivePasteTargetNodesFromSelectedNodes();
	if (PasteTargetNodes.Num() > 1)
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
		// We need to clean-up the nodes we built to test the paste operation
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
	if (PasteTargetNodes.Num() >= 1)
	{
		checkf(PasteTargetNodes.Num() == 1, TEXT("This is enforced earlier in this function, just confirming the code stays that way here."));

		UFlowGraphNode* PasteTargetNode = PasteTargetNodes.Top();

		if (!CanPasteNodesAsSubNodes(NodesToPaste, *PasteTargetNode))
		{
			return false;
		}
	}

	return true;
}

bool SFlowGraphEditor::CanPasteNodesAsSubNodes(const TSet<UEdGraphNode*>& NodesToPaste, const UFlowGraphNode& PasteTargetNode) const
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
			// a non-INDEX_NONE parent index indicates the subnode is is a non-root subnode in the NodesToPaste set

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
	UFlowNodeBase* FlowNodeBase = Cast<UFlowGraphNode>(Node)->GetFlowNodeBase();
	UFlowNode* FlowNode = Cast<UFlowNode>(FlowNodeBase);

	if (IsValid(FlowNodeBase))
	{
		if (UFlowGraphEditorSettings::Get()->NodeDoubleClickTarget == EFlowNodeDoubleClickTarget::NodeDefinition)
		{
			Node->JumpToDefinition();
		}
		else
		{
			FString AssetPath;
			UObject* AssetToEdit = nullptr;
			
			if (FlowNode)
			{
				AssetPath = FlowNode->GetAssetPath();
				AssetToEdit = FlowNode->GetAssetToEdit();
			}
			
			if (!AssetPath.IsEmpty())
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetPath);
			}
			else if (AssetToEdit)
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetToEdit);

				if (IsPIE())
				{
					if (UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(FlowNode))
					{
						const TWeakObjectPtr<UFlowAsset> SubFlowInstance = SubGraphNode->GetFlowAsset()->GetFlowInstance(SubGraphNode);
						if (SubFlowInstance.IsValid())
						{
							SubGraphNode->GetFlowAsset()->GetTemplateAsset()->SetInspectedInstance(SubFlowInstance->GetDisplayName());
						}
					}
				}
			}
			else if (UFlowGraphEditorSettings::Get()->NodeDoubleClickTarget == EFlowNodeDoubleClickTarget::PrimaryAssetOrNodeDefinition)
			{
				Node->JumpToDefinition();
			}
		}
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

void SFlowGraphEditor::RefreshContextPins() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->RefreshContextPins(true);
	}
}

bool SFlowGraphEditor::CanRefreshContextPins() const
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
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->NodeBreakpoint.AllowTrait();
	}
}

void SFlowGraphEditor::OnAddPinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints.Add(Pin, FFlowPinTrait(true));
		}
	}
}

bool SFlowGraphEditor::CanAddBreakpoint() const
{
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		return !SelectedNode->NodeBreakpoint.IsAllowed();
	}

	return false;
}

bool SFlowGraphEditor::CanAddPinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			return !GraphNode->PinBreakpoints.Contains(Pin) || !GraphNode->PinBreakpoints[Pin].IsAllowed();
		}
	}

	return false;
}

void SFlowGraphEditor::OnRemoveBreakpoint() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->NodeBreakpoint.DisallowTrait();
	}
}

void SFlowGraphEditor::OnRemovePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints.Remove(Pin);
		}
	}
}

bool SFlowGraphEditor::CanRemoveBreakpoint() const
{
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		return SelectedNode->NodeBreakpoint.IsAllowed();
	}

	return false;
}

bool SFlowGraphEditor::CanRemovePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin);
		}
	}

	return false;
}

void SFlowGraphEditor::OnEnableBreakpoint() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->NodeBreakpoint.EnableTrait();
	}
}

void SFlowGraphEditor::OnEnablePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints[Pin].EnableTrait();
		}
	}
}

bool SFlowGraphEditor::CanEnableBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin);
		}
	}

	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		return SelectedNode->NodeBreakpoint.CanEnable();
	}

	return false;
}

bool SFlowGraphEditor::CanEnablePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin) && GraphNode->PinBreakpoints[Pin].CanEnable();
		}
	}

	return false;
}

void SFlowGraphEditor::OnDisableBreakpoint() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->NodeBreakpoint.DisableTrait();
	}
}

void SFlowGraphEditor::OnDisablePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints[Pin].DisableTrait();
		}
	}
}

bool SFlowGraphEditor::CanDisableBreakpoint() const
{
	for (const UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		return SelectedNode->NodeBreakpoint.IsEnabled();
	}

	return false;
}

bool SFlowGraphEditor::CanDisablePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			return GraphNode->PinBreakpoints.Contains(Pin) && GraphNode->PinBreakpoints[Pin].IsEnabled();
		}
	}

	return false;
}

void SFlowGraphEditor::OnToggleBreakpoint() const
{
	for (UFlowGraphNode* SelectedNode : GetSelectedFlowNodes())
	{
		SelectedNode->NodeBreakpoint.ToggleTrait();
	}
}

void SFlowGraphEditor::OnTogglePinBreakpoint()
{
	if (UEdGraphPin* Pin = GetGraphPinForMenu())
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pin->GetOwningNode()))
		{
			GraphNode->PinBreakpoints.Add(Pin, FFlowPinTrait());
			GraphNode->PinBreakpoints[Pin].ToggleTrait();
		}
	}
}

bool SFlowGraphEditor::CanToggleBreakpoint() const
{
	return GetSelectedFlowNodes().Num() > 0;
}

bool SFlowGraphEditor::CanTogglePinBreakpoint()
{
	return GetGraphPinForMenu() != nullptr;
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
