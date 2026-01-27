// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"

#include "FindInFlowEnums.h"

DECLARE_DELEGATE_OneParam(FFindInFlowApplyDelegate, EFlowSearchFlags);

class SFindInFlowFilterPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFindInFlowFilterPopup) {}
		SLATE_ARGUMENT(FFindInFlowApplyDelegate, OnApply)
		SLATE_ARGUMENT(FFindInFlowApplyDelegate, OnSaveAsDefault)
		SLATE_ARGUMENT(EFlowSearchFlags, InitialFlags)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:

	EFlowSearchFlags ProposedFlags = EFlowSearchFlags::DefaultSearchFlags;

	FFindInFlowApplyDelegate OnApplyDelegate;
	FFindInFlowApplyDelegate OnSaveAsDefaultDelegate;

	TSharedPtr<SVerticalBox> CheckBoxContainer;

	ECheckBoxState GetCheckState(EFlowSearchFlags Flag) const
	{
		return EnumHasAnyFlags(ProposedFlags, Flag) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	FReply OnApplyClicked();
	FReply OnCancelClicked();
	FReply OnToggleAllClicked();
	FReply OnSaveAsDefaultClicked();
};