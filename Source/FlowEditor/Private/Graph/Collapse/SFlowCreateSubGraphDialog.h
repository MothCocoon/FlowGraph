// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class UFlowAsset;

/**
 * Modal prompt for the name of a sub-graph asset generated from a node selection. Validates the name
 * as it is typed, and lists any warnings the collapse plan raised so the designer can back out.
 */
class SFlowCreateSubGraphDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlowCreateSubGraphDialog) {}
		SLATE_ARGUMENT(const UFlowAsset*, SourceAsset)
		SLATE_ARGUMENT(TArray<FText>, Warnings)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Shows the prompt modally. Returns false when the designer cancelled. */
	static bool ShowModal(const UFlowAsset& SourceAsset, const TArray<FText>& Warnings, FString& OutAssetName);

private:
	FReply OnCreateClicked();
	FReply OnCancelClicked();
	void OnAssetNameChanged(const FText& NewName);
	void OnAssetNameCommitted(const FText& NewName, ETextCommit::Type CommitType);
	bool IsCreateEnabled() const;
	FText GetValidationErrorText() const;
	EVisibility GetErrorVisibility() const;

	TWeakObjectPtr<const UFlowAsset> SourceAsset;
	TSharedPtr<SEditableTextBox> AssetNameTextBox;
	FString AssetName;
	FText ValidationError;
	bool bConfirmed = false;
};