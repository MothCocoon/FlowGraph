#include "Graph/Widgets/SFlowPalette.h"
#include "Asset/FlowAssetEditor.h"
#include "FlowEditorCommands.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSchema_Actions.h"

#include "Nodes/FlowNode.h"

#include "EditorStyleSet.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "Widgets/Input/STextBlock.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "FlowGraphPalette"

void SFlowPaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData)
{
	const FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);

	check(InCreateData->Action.IsValid());

	const TSharedPtr<FEdGraphSchemaAction> GraphAction = InCreateData->Action;
	ActionPtr = InCreateData->Action;

	// Get the hotkey chord if one exists for this action
	TSharedPtr<const FInputChord> HotkeyChord;

	if (FFlowSpawnNodeCommands::IsRegistered())
	{
		if (GraphAction->GetTypeId() == FFlowGraphSchemaAction_NewNode::StaticGetTypeId())
		{
			UClass* FlowNodeClass = StaticCastSharedPtr<FFlowGraphSchemaAction_NewNode>(GraphAction)->NativeNodeClass;
			HotkeyChord = FFlowSpawnNodeCommands::Get().GetChordByClass(FlowNodeClass);
		}
		else if (GraphAction->GetTypeId() == FFlowGraphSchemaAction_NewComment::StaticGetTypeId())
		{
			HotkeyChord = FFlowSpawnNodeCommands::Get().GetChordByClass(UFlowNode::StaticClass());
		}
	}

	// Find icons
	const FSlateBrush* IconBrush = FEditorStyle::GetBrush(TEXT("NoBrush"));
	const FSlateColor IconColor = FSlateColor::UseForeground();
	const FText IconToolTip = GraphAction->GetTooltipDescription();
	const bool bIsReadOnly = false;

	const TSharedRef<SWidget> IconWidget = CreateIconWidget(IconToolTip, IconBrush, IconColor);
	const TSharedRef<SWidget> NameSlotWidget = CreateTextSlotWidget(NameFont, InCreateData, bIsReadOnly);
	const TSharedRef<SWidget> HotkeyDisplayWidget = CreateHotkeyDisplayWidget(NameFont, HotkeyChord);

	// Create the actual widget
	this->ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				IconWidget
			]
		+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			.Padding(3, 0)
			[
				NameSlotWidget
			]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				HotkeyDisplayWidget
			]
	];
}

TSharedRef<SWidget> SFlowPaletteItem::CreateHotkeyDisplayWidget(const FSlateFontInfo& NameFont, const TSharedPtr<const FInputChord> HotkeyChord) const
{
	FText HotkeyText;
	if (HotkeyChord.IsValid())
	{
		HotkeyText = HotkeyChord->GetInputText();
	}

	return SNew(STextBlock)
		.Text(HotkeyText)
		.Font(NameFont);
}

FText SFlowPaletteItem::GetItemTooltip() const
{
	return ActionPtr.Pin()->GetTooltipDescription();
}

void SFlowPalette::Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditor)
{
	FlowAssetEditorPtr = InFlowAssetEditor;

	UpdateCategoryNames();
	UFlowGraphSchema::OnNodeListChanged.AddSP(this, &SFlowPalette::Refresh);

	this->ChildSlot
	[
		SNew(SBorder)
			.Padding(2.0f)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot() // Filter UI
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SAssignNew(CategoryComboBox, STextComboBox)
									.OptionsSource(&CategoryNames)
									.OnSelectionChanged(this, &SFlowPalette::CategorySelectionChanged)
									.InitiallySelectedItem(CategoryNames[0])
							]
					]
				+ SVerticalBox::Slot() // Content list
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SAssignNew(GraphActionMenu, SGraphActionMenu)
							.OnActionDragged(this, &SFlowPalette::OnActionDragged)
							.OnActionSelected(this, &SFlowPalette::OnActionSelected)
							.OnCreateWidgetForAction(this, &SFlowPalette::OnCreateWidgetForAction)
							.OnCollectAllActions(this, &SFlowPalette::CollectAllActions)
							.AutoExpandActionMenu(true)
					]
			]
	];
}

SFlowPalette::~SFlowPalette()
{
	UFlowGraphSchema::OnNodeListChanged.RemoveAll(this);
}

void SFlowPalette::Refresh()
{
	const FString LastSelectedCategory = CategoryComboBox->GetSelectedItem().IsValid() ? *CategoryComboBox->GetSelectedItem().Get() : FString();

	UpdateCategoryNames();
	RefreshActionsList(true);

	// refresh list of category and currently selected category
	CategoryComboBox->RefreshOptions();
	TSharedPtr<FString> SelectedCategory = CategoryNames[0];
	if (!LastSelectedCategory.IsEmpty())
	{
		for (const TSharedPtr<FString>& CategoryName : CategoryNames)
		{
			if (*CategoryName.Get() == LastSelectedCategory)
			{
				SelectedCategory = CategoryName;
				break;
			}
		}
	}
	CategoryComboBox->SetSelectedItem(SelectedCategory);
}

void SFlowPalette::UpdateCategoryNames()
{
	CategoryNames = {MakeShareable(new FString(TEXT("All")))};
	CategoryNames.Append(UFlowGraphSchema::GetFlowNodeCategories());
}

TSharedRef<SWidget> SFlowPalette::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return SNew(SFlowPaletteItem, InCreateData);
}

void SFlowPalette::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	FGraphActionMenuBuilder ActionMenuBuilder;
	UFlowGraphSchema::GetPaletteActions(ActionMenuBuilder, GetFilterCategoryName());
	OutAllActions.Append(ActionMenuBuilder);
}

FString SFlowPalette::GetFilterCategoryName() const
{
	if (CategoryComboBox.IsValid() && CategoryComboBox->GetSelectedItem() != CategoryNames[0])
	{
		return *CategoryComboBox->GetSelectedItem();
	}

	return FString();
}

void SFlowPalette::CategorySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	RefreshActionsList(true);
}

void SFlowPalette::OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions, ESelectInfo::Type InSelectionType)
{
	if (InSelectionType == ESelectInfo::OnMouseClick || InSelectionType == ESelectInfo::OnKeyPress || InSelectionType == ESelectInfo::OnNavigation || InActions.Num() == 0)
	{
		TSharedPtr<FFlowAssetEditor> FlowAssetEditor = FlowAssetEditorPtr.Pin();
		if (FlowAssetEditor)
		{
			FlowAssetEditor->SetUISelectionState(FFlowAssetEditor::PaletteTab);
		}
	}
}

void SFlowPalette::ClearGraphActionMenuSelection() const
{
	GraphActionMenu->SelectItemByName(NAME_None);
}

#undef LOCTEXT_NAMESPACE
