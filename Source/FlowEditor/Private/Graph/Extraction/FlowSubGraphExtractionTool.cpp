// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Graph/Extraction/FlowSubGraphExtractionTool.h"

#include "Asset/FlowAssetEditor.h"
#include "Graph/Extraction/FlowSubGraphExtractionPlan.h"
#include "Graph/Extraction/FlowSubGraphExtractionApply.h"
#include "Graph/Extraction/FlowSubGraphExtractionSourceControl.h"
#include "Graph/Extraction/SFlowCreateSubGraphDialog.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FlowSubGraphExtractionTool"

void FFlowSubGraphExtractionTool::Execute(SFlowGraphEditor& GraphEditor)
{
	if (!GraphEditor.FlowAsset.IsValid())
	{
		return;
	}

	TSet<UFlowGraphNode*> SelectedNodes;
	for (UFlowGraphNode* SelectedNode : GraphEditor.GetSelectedFlowNodes())
	{
		SelectedNodes.Add(SelectedNode);
	}
	const FlowSubGraphExtraction::FExtractionPlan Plan = FlowSubGraphExtraction::PlanExtraction(SelectedNodes);
	if (!Plan.CanApply())
	{
		FTextBuilder ErrorBuilder;
		ErrorBuilder.AppendLine(LOCTEXT("CannotExtract", "This selection cannot be extracted into a SubGraph."));
		for (const FText& Error : Plan.Errors)
		{
			ErrorBuilder.AppendLineFormat(LOCTEXT("ErrorBullet", "• {0}"), Error);
		}

		FMessageDialog::Open(EAppMsgType::Ok, ErrorBuilder.ToText());
		return;
	}

	FString NewAssetName;
	if (!SFlowCreateSubGraphDialog::ShowModal(*GraphEditor.FlowAsset, Plan.Warnings, NewAssetName))
	{
		return;
	}

	GraphEditor.FlowAssetEditor.Pin()->SetUISelectionState(NAME_None);
	GraphEditor.ClearSelectionSet();

	FText SaveError;
	const FlowSubGraphExtraction::FExtractionResult Result = ApplyAndSave(Plan, *GraphEditor.FlowAsset, NewAssetName, SaveError);
	GraphEditor.NotifyGraphChanged();

	if (!Result.Succeeded())
	{
		for (UFlowGraphNode* SelectedNode : Plan.SelectedNodes)
		{
			if (IsValid(SelectedNode))
			{
				GraphEditor.SetNodeSelection(SelectedNode, true);
			}
		}

		FMessageDialog::Open(EAppMsgType::Ok, Result.Error);
		return;
	}

	check(IsValid(Result.SubGraphNode));
	constexpr bool bSelectNode = true;
	GraphEditor.SetNodeSelection(Result.SubGraphNode, bSelectNode);

	if (!SaveError.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, SaveError);
	}
}

FlowSubGraphExtraction::FExtractionResult FFlowSubGraphExtractionTool::ApplyAndSave(
	const FlowSubGraphExtraction::FExtractionPlan& Plan,
	UFlowAsset& SourceAsset,
	const FString& NewAssetName,
	FText& OutSaveError)
{
	OutSaveError = FText::GetEmpty();

	FlowSubGraphExtraction::FSourceControlContext SourceControlContext;
	FText SourceControlPreparationError;
	FlowSubGraphExtraction::FExtractionResult Result;
	{
		const FScopedTransaction Transaction(
			LOCTEXT("ExtractToSubGraphTransaction", "Extract to SubGraph"));

		Result = FlowSubGraphExtraction::ApplyExtraction(Plan, SourceAsset, NewAssetName);
		if (Result.Succeeded())
		{
			SourceControlPreparationError = FlowSubGraphExtraction::PrepareSourceControl(
				SourceAsset,
				NewAssetName,
				SourceControlContext);
		}
	}

	if (!Result.Succeeded() || !SourceControlPreparationError.IsEmpty())
	{
		// Attempt to undo the recorded transaction.
		const bool bRollbackSucceeded = !Result.bTransactionRecorded || GEditor->UndoTransaction(false);

		// Ensure the created asset is discarded. Set to true if Result.NewAsset is invalid.
		const bool bDiscardSucceeded = FlowSubGraphExtraction::DiscardCreatedAsset(Result.NewAsset);

		FText FailureMessage = SourceControlPreparationError.IsEmpty() ? Result.Error : SourceControlPreparationError;
		if (!bRollbackSucceeded)
		{
			FailureMessage = FText::Format(
				LOCTEXT("ExtractionRollbackFailed", "{0}\n\nThe source graph could not be restored automatically."),
				FailureMessage);
		}
		else if (!bDiscardSucceeded)
		{
			FailureMessage = FText::Format(
				LOCTEXT("ExtractionCleanupFailed", "{0}\n\nThe generated asset could not be removed automatically: {1}"),
				FailureMessage,
				FText::FromString(Result.NewAsset->GetPathName()));
		}

		Result.Error = FailureMessage;
		Result.NewAsset = nullptr;
		Result.SubGraphNode = nullptr;
		return Result;
	}

	OutSaveError = FlowSubGraphExtraction::SaveAndFinalizeSourceControl(
		*Result.NewAsset,
		SourceAsset,
		SourceControlContext);
	return Result;
}

#undef LOCTEXT_NAMESPACE
