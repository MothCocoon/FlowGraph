// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Components/VerticalBox.h"
#include "ISourceControlProvider.h"
#include "Widgets/SCompoundWidget.h"

class FUpdateStatus;
class SVerticalBox;
struct FRevisionInfo;

// Forced to make a variant of SBlueprintRevisionMenu, only to replace to UBlueprint* parameter
class FLOWEDITOR_API SAssetRevisionMenu : public SCompoundWidget
{
	DECLARE_DELEGATE_TwoParams(FOnRevisionSelected, FRevisionInfo const& RevisionInfo, const FString& InFilename)

public:
	SLATE_BEGIN_ARGS(SAssetRevisionMenu)
			: _bIncludeLocalRevision(false)
		{
		}

		SLATE_ARGUMENT(bool, bIncludeLocalRevision)
		SLATE_EVENT(FOnRevisionSelected, OnRevisionSelected)
	SLATE_END_ARGS()

	virtual ~SAssetRevisionMenu() override;

	void Construct(const FArguments& InArgs, const FString& InFilename);

private:
	/** Delegate used to determine the visibility 'in progress' widgets */
	EVisibility GetInProgressVisibility() const;
	/** Delegate used to determine the visibility of the cancel button */
	EVisibility GetCancelButtonVisibility() const;

	/** Delegate used to cancel a source control operation in progress */
	FReply OnCancelButtonClicked() const;
	/** Callback for when the source control operation is complete */
	void OnSourceControlQueryComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);

	/**  */
	bool bIncludeLocalRevision = false;
	/**  */
	FOnRevisionSelected OnRevisionSelected;
	/** The name of the file we want revision info for */
	FString Filename;
	/** The box we are using to display our menu */
	TSharedPtr<SVerticalBox> MenuBox;
	/** The source control operation in progress */
	TSharedPtr<FUpdateStatus, ESPMode::ThreadSafe> SourceControlQueryOp;
	/** The state of the SCC query */
	uint32 SourceControlQueryState = 0;
};
