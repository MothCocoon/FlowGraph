// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "SGraphPalette.h"

class FFlowAssetEditor;

/** Widget displaying a single item  */
class SFlowPaletteItem : public SGraphPaletteItem
{
public:
	SLATE_BEGIN_ARGS(SFlowPaletteItem) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData);

private:
	TSharedRef<SWidget> CreateHotkeyDisplayWidget(const FSlateFontInfo& NameFont, const TSharedPtr<const FInputChord> HotkeyChord) const;
	virtual FText GetItemTooltip() const override;
};

/** Flow Palette  */
class SFlowPalette : public SGraphPalette
{
public:
	SLATE_BEGIN_ARGS(SFlowPalette) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor);
	virtual ~SFlowPalette() override;

protected:
	void Refresh();
	void UpdateCategoryNames();

	// SGraphPalette
	virtual TSharedRef<SWidget> OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData) override;
	virtual void CollectAllActions(FGraphActionListBuilderBase& OutAllActions) override;
	// --

	FString GetFilterCategoryName() const;
	void CategorySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	void OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions, ESelectInfo::Type InSelectionType) const;

public:
	void ClearGraphActionMenuSelection() const;

protected:
	TWeakPtr<FFlowAssetEditor> FlowAssetEditorPtr;
	TArray<TSharedPtr<FString>> CategoryNames;
	TSharedPtr<STextComboBox> CategoryComboBox;
};
