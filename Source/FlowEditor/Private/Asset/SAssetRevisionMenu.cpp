// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/SAssetRevisionMenu.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IAssetTypeActions.h"
#include "ISourceControlModule.h"
#include "ISourceControlRevision.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SourceControlOperations.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SFlowRevisionMenu"

/**  */
namespace ESourceControlQueryState
{
	enum Type
	{
		NotQueried,
		QueryInProgress,
		Queried,
	};
}

//------------------------------------------------------------------------------
SAssetRevisionMenu::~SAssetRevisionMenu()
{
	// cancel any operation if this widget is destroyed while in progress
	if (SourceControlQueryState == ESourceControlQueryState::QueryInProgress)
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		if (SourceControlQueryOp.IsValid() && SourceControlProvider.CanCancelOperation(SourceControlQueryOp.ToSharedRef()))
		{
			SourceControlProvider.CancelOperation(SourceControlQueryOp.ToSharedRef());
		}
	}
}

//------------------------------------------------------------------------------
void SAssetRevisionMenu::Construct(const FArguments& InArgs, const FString& InFilename)
{
	bIncludeLocalRevision = InArgs._bIncludeLocalRevision;
	OnRevisionSelected = InArgs._OnRevisionSelected;

	SourceControlQueryState = ESourceControlQueryState::NotQueried;

	ChildSlot
	[
		SAssignNew(MenuBox, SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SBorder)
			.Visibility(this, &SAssetRevisionMenu::GetInProgressVisibility)
			.BorderImage(FAppStyle::GetBrush("Menu.Background"))
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SThrobber)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DiffMenuOperationInProgress", "Updating history..."))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Visibility(this, &SAssetRevisionMenu::GetCancelButtonVisibility)
					.OnClicked(this, &SAssetRevisionMenu::OnCancelButtonClicked)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DiffMenuCancelButton", "Cancel"))
					]
				]
			]
		]
	];

	Filename = InFilename;
	if (!Filename.IsEmpty())
	{
		// make sure the history info is up to date
		SourceControlQueryOp = ISourceControlOperation::Create<FUpdateStatus>();
		SourceControlQueryOp->SetUpdateHistory(true);
		ISourceControlModule::Get().GetProvider().Execute(SourceControlQueryOp.ToSharedRef(), Filename, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SAssetRevisionMenu::OnSourceControlQueryComplete));

		SourceControlQueryState = ESourceControlQueryState::QueryInProgress;
	}
}

//------------------------------------------------------------------------------
EVisibility SAssetRevisionMenu::GetInProgressVisibility() const
{
	return (SourceControlQueryState == ESourceControlQueryState::QueryInProgress) ? EVisibility::Visible : EVisibility::Collapsed;
}

//------------------------------------------------------------------------------
EVisibility SAssetRevisionMenu::GetCancelButtonVisibility() const
{
	const ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	return SourceControlQueryOp.IsValid() && SourceControlProvider.CanCancelOperation(SourceControlQueryOp.ToSharedRef()) ? EVisibility::Visible : EVisibility::Collapsed;
}

//------------------------------------------------------------------------------
FReply SAssetRevisionMenu::OnCancelButtonClicked() const
{
	if (SourceControlQueryOp.IsValid())
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		SourceControlProvider.CancelOperation(SourceControlQueryOp.ToSharedRef());
	}

	return FReply::Handled();
}

//------------------------------------------------------------------------------
void SAssetRevisionMenu::OnSourceControlQueryComplete(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	check(SourceControlQueryOp == InOperation);


	// Add pop-out menu for each revision
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection =*/true, /*InCommandList =*/nullptr);

	MenuBuilder.BeginSection("AddDiffRevision", LOCTEXT("Revisions", "Revisions"));
	if (bIncludeLocalRevision)
	{
		FText const ToolTipText = LOCTEXT("LocalRevisionToolTip", "The current copy you have saved to disk (locally)");

		FOnRevisionSelected OnRevisionSelectedDelegate = OnRevisionSelected;
		auto OnMenuItemSelected = [OnRevisionSelectedDelegate, this]()
		{
			OnRevisionSelectedDelegate.ExecuteIfBound(FRevisionInfo::InvalidRevision(), Filename);
		};

		MenuBuilder.AddMenuEntry(LOCTEXT("LocalRevision", "Local"), ToolTipText, FSlateIcon(), FUIAction(FExecuteAction::CreateLambda(OnMenuItemSelected)));
	}

	if (InResult == ECommandResult::Succeeded)
	{
		// get the cached state
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Filename, EStateCacheUsage::Use);

		if (SourceControlState.IsValid() && SourceControlState->GetHistorySize() > 0)
		{
			// Figure out the highest revision # (so we can label it "Depot")
			int32 LatestRevision = 0;
			for (int32 HistoryIndex = 0; HistoryIndex < SourceControlState->GetHistorySize(); HistoryIndex++)
			{
				TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(HistoryIndex);
				if (Revision.IsValid() && Revision->GetRevisionNumber() > LatestRevision)
				{
					LatestRevision = Revision->GetRevisionNumber();
				}
			}

			for (int32 HistoryIndex = 0; HistoryIndex < SourceControlState->GetHistorySize(); HistoryIndex++)
			{
				TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(HistoryIndex);
				if (Revision.IsValid())
				{
					FInternationalization& I18N = FInternationalization::Get();

					FText Label = FText::Format(LOCTEXT("RevisionNumber", "Revision {0}"), FText::AsNumber(Revision->GetRevisionNumber(), nullptr, I18N.GetInvariantCulture()));

					FFormatNamedArguments Args;
					Args.Add(TEXT("CheckInNumber"), FText::AsNumber(Revision->GetCheckInIdentifier(), nullptr, I18N.GetInvariantCulture()));
					Args.Add(TEXT("Revision"), FText::FromString(Revision->GetRevision()));
					Args.Add(TEXT("UserName"), FText::FromString(Revision->GetUserName()));
					Args.Add(TEXT("DateTime"), FText::AsDate(Revision->GetDate()));
					Args.Add(TEXT("ChanglistDescription"), FText::FromString(Revision->GetDescription()));
					FText ToolTipText;
					if (ISourceControlModule::Get().GetProvider().UsesChangelists())
					{
						ToolTipText = FText::Format(LOCTEXT("ChangelistToolTip", "CL #{CheckInNumber} {UserName} \n{DateTime} \n{ChanglistDescription}"), Args);
					}
					else
					{
						ToolTipText = FText::Format(LOCTEXT("RevisionToolTip", "{Revision} {UserName} \n{DateTime} \n{ChanglistDescription}"), Args);
					}

					if (LatestRevision == Revision->GetRevisionNumber())
					{
						Label = LOCTEXT("Depo", "Depot");
					}

					FRevisionInfo RevisionInfo = {
						Revision->GetRevision(),
						Revision->GetCheckInIdentifier(),
						Revision->GetDate()
					};
					FOnRevisionSelected OnRevisionSelectedDelegate = OnRevisionSelected;
					auto OnMenuItemSelected = [RevisionInfo, OnRevisionSelectedDelegate, this]()
					{
						OnRevisionSelectedDelegate.ExecuteIfBound(RevisionInfo, Filename);
					};
					MenuBuilder.AddMenuEntry(TAttribute<FText>(Label), ToolTipText, FSlateIcon(), FUIAction(FExecuteAction::CreateLambda(OnMenuItemSelected)));
				}
			}
		}
		else if (!bIncludeLocalRevision)
		{
			// Show 'empty' item in toolbar
			MenuBuilder.AddMenuEntry(LOCTEXT("NoRevisonHistory", "No revisions found"), FText(), FSlateIcon(), FUIAction());
		}
	}
	else if (!bIncludeLocalRevision)
	{
		// Show 'empty' item in toolbar
		MenuBuilder.AddMenuEntry(LOCTEXT("NoRevisonHistory", "No revisions found"), FText(), FSlateIcon(), FUIAction());
	}

	MenuBuilder.EndSection();
	MenuBox->AddSlot()
	[
		MenuBuilder.MakeWidget(nullptr, 500)
	];

	SourceControlQueryOp.Reset();
	SourceControlQueryState = ESourceControlQueryState::Queried;
}

#undef LOCTEXT_NAMESPACE
