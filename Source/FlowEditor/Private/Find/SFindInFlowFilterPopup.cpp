// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Find/SFindInFlowFilterPopup.h"

#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "FindInFlow"

void SFindInFlowFilterPopup::Construct(const FArguments& InArgs)
{
	OnApplyDelegate = InArgs._OnApply;
	ProposedFlags = InArgs._InitialFlags;

	// Build the checkbox container with slots added during construction
	SAssignNew(CheckBoxContainer, SVerticalBox);

	for (EFlowSearchFlags Flag : MakeFlagsRange(EFlowSearchFlags::All))
	{
		CheckBoxContainer->AddSlot()
			.AutoHeight()
			[
				SNew(SCheckBox)
					.IsChecked(this, &SFindInFlowFilterPopup::GetCheckState, Flag)
					.OnCheckStateChanged_Lambda([this, Flag](ECheckBoxState NewState)
						{
							if (NewState == ECheckBoxState::Checked)
							{
								EnumAddFlags(ProposedFlags, Flag);
							}
							else
							{
								EnumRemoveFlags(ProposedFlags, Flag);
							}
						})
					[
						SNew(STextBlock)
							.Text(UEnum::GetDisplayValueAsText(Flag))
					]
			];
	}

	ChildSlot
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(10, 10, 10, 5)
				[
					SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							CheckBoxContainer.ToSharedRef()
						]
				]
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10, 0, 10, 10)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
								.Text(LOCTEXT("ToggleAllFilters", "Toggle All"))
								.OnClicked(this, &SFindInFlowFilterPopup::OnToggleAllClicked)
						]
				]
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
								.Text(LOCTEXT("ApplyFilters", "Apply"))
								.OnClicked(this, &SFindInFlowFilterPopup::OnApplyClicked)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(4, 0, 0, 0)
						[
							SNew(SButton)
								.Text(LOCTEXT("CancelFilters", "Cancel"))
								.OnClicked(this, &SFindInFlowFilterPopup::OnCancelClicked)
						]
				]
		];
}

FReply SFindInFlowFilterPopup::OnApplyClicked()
{
	OnApplyDelegate.ExecuteIfBound(ProposedFlags);
	FSlateApplication::Get().DismissAllMenus();
	return FReply::Handled();
}

FReply SFindInFlowFilterPopup::OnCancelClicked()
{
	FSlateApplication::Get().DismissAllMenus();
	return FReply::Handled();
}

FReply SFindInFlowFilterPopup::OnToggleAllClicked()
{
	if (ProposedFlags != EFlowSearchFlags::None)
	{
		ProposedFlags = EFlowSearchFlags::None;
	}
	else
	{
		ProposedFlags = EFlowSearchFlags::All;
	}

	CheckBoxContainer->Invalidate(EInvalidateWidgetReason::Layout);

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE