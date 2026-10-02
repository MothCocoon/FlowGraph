// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Graph/Collapse/FlowCollapseToSubGraphTool.h"

#include "Asset/FlowAssetEditor.h"
#include "FlowCollapseToSubGraphPlan.h"
#include "FlowCollapseToSubGraphApply.h"
#include "FlowCollapseToSubGraphSourceControl.h"
#include "SFlowCreateSubGraphDialog.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "Editor/UnrealEdEngine.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "FlowCollapseToSubGraphTool"

void FFlowCollapseToSubGraphTool::Execute(SFlowGraphEditor& GraphEditor)
{
	if (!GraphEditor.FlowAsset.IsValid())
	{
		return;
	}

	TSet<UFlowGraphNode*> SelectedFlowNodes;
	for (UFlowGraphNode* SelectedNode : GraphEditor.GetSelectedFlowNodes())
	{
		SelectedFlowNodes.Add(SelectedNode);
	}

	const FlowCollapseToSubGraph::FCollapsePlan Plan = FlowCollapseToSubGraph::PlanCollapse(SelectedFlowNodes);
	if (!Plan.CanApply())
	{
		FTextBuilder ErrorBuilder;
		ErrorBuilder.AppendLine(LOCTEXT("CannotCollapse", "This selection cannot be collapsed into a sub-graph:"));
		for (const FText& Error : Plan.Errors)
		{
			ErrorBuilder.AppendLineFormat(LOCTEXT("ErrorBullet", "- {0}"), Error);
		}

		FMessageDialog::Open(EAppMsgType::Ok, ErrorBuilder.ToText());
		return;
	}

	FString NewAssetName;
	if (!SFlowCreateSubGraphDialog::ShowModal(*GraphEditor.FlowAsset, Plan.Warnings, NewAssetName))
	{
		return;
	}

	FlowCollapseToSubGraph::FSourceControlContext SourceControlContext;
	FText SourceControlPreparationError;
	FlowCollapseToSubGraph::FCollapseResult Result;
	{
		const FScopedTransaction Transaction(
			LOCTEXT("CreateSubGraphFromSelectionTransaction", "Create Sub-Graph from Selection"));

		GraphEditor.FlowAssetEditor.Pin()->SetUISelectionState(NAME_None);
		GraphEditor.ClearSelectionSet();

		Result = FlowCollapseToSubGraph::ApplyCollapse(Plan, *GraphEditor.FlowAsset, NewAssetName);
		if (Result.Succeeded())
		{
			SourceControlPreparationError = FlowCollapseToSubGraph::PrepareSourceControl(
				*GraphEditor.FlowAsset,
				NewAssetName,
				SourceControlContext);
		}
	}

	if (!Result.Succeeded() || !SourceControlPreparationError.IsEmpty())
	{
		// Attempt to undo the recorded transaction.
		const bool bRollbackSucceeded = !Result.bTransactionRecorded || GEditor->UndoTransaction(false);

		// Ensure the created asset is discarded. Set to true if Result.NewAsset is invalid.
		const bool bDiscardSucceeded = FlowCollapseToSubGraph::DiscardCreatedAsset(Result.NewAsset);

		GraphEditor.NotifyGraphChanged();
		for (UFlowGraphNode* SelectedNode : Plan.SelectedNodes)
		{
			if (IsValid(SelectedNode))
			{
				GraphEditor.SetNodeSelection(SelectedNode, true);
			}
		}

		FText FailureMessage = SourceControlPreparationError.IsEmpty() ? Result.Error : SourceControlPreparationError;
		if (!bRollbackSucceeded)
		{
			FailureMessage = FText::Format(
				LOCTEXT("CollapseRollbackFailed", "{0}\n\nThe source graph could not be restored automatically."),
				FailureMessage);
		}
		else if (!bDiscardSucceeded)
		{
			FailureMessage = FText::Format(
				LOCTEXT("CollapseCleanupFailed", "{0}\n\nThe generated asset could not be removed automatically: {1}"),
				FailureMessage,
				FText::FromString(Result.NewAsset->GetPathName()));
		}

		FMessageDialog::Open(EAppMsgType::Ok, FailureMessage);
		return;
	}

	GraphEditor.NotifyGraphChanged();

	check(IsValid(Result.SubGraphNode));
	constexpr bool bSelectNode = true;
	GraphEditor.SetNodeSelection(Result.SubGraphNode, bSelectNode);

	const FText SourceControlError = FlowCollapseToSubGraph::SaveAndFinalizeSourceControl(
		*Result.NewAsset,
		*GraphEditor.FlowAsset,
		SourceControlContext);
	if (!SourceControlError.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, SourceControlError);
	}
}

#undef LOCTEXT_NAMESPACE
