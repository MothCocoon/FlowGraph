#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/InputChord.h"
#include "SGraphPalette.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"

class FFlowAssetEditor;

/** Widget for displaying a single item  */
class SFlowPaletteItem : public SGraphPaletteItem
{
public:
	SLATE_BEGIN_ARGS(SFlowPaletteItem) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData);

private:
	TSharedRef<SWidget> CreateHotkeyDisplayWidget(const FSlateFontInfo& NameFont, const TSharedPtr<const FInputChord> HotkeyChord);
	virtual FText GetItemTooltip() const override;
};

/** Flow Palette  */
class SFlowPalette : public SGraphPalette
{
public:
	SLATE_BEGIN_ARGS( SFlowPalette ) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditorPtr);

protected:
	// SGraphPalette Interface
	virtual TSharedRef<SWidget> OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData) override;
	virtual void CollectAllActions(FGraphActionListBuilderBase& OutAllActions) override;
	// End of SGraphPalette Interface

	FString GetFilterCategoryName() const;
	void CategorySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

protected:
	TWeakPtr<FFlowAssetEditor> FlowAssetEditorPtr;
	TArray<TSharedPtr<FString>> CategoryNames;
	TSharedPtr<STextComboBox> CategoryComboBox;
};
