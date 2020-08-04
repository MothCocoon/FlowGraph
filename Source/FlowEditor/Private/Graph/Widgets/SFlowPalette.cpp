#include "Graph/Widgets/SFlowPalette.h"
#include "Graph/FlowAssetEditor.h"
#include "Graph/FlowGraphCommands.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSchema_Actions.h"

#include "Nodes/FlowNode.h"

#include "EditorStyleSet.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/SOverlay.h"

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
			UClass* FlowNodeClass = StaticCastSharedPtr<FFlowGraphSchemaAction_NewNode>(GraphAction)->NodeClass;
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

			// Icon slot
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				IconWidget
			]

			// Name slot
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			.Padding(3, 0)
			[
				NameSlotWidget
			]

			// Hotkey slot
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

void SFlowPalette::Construct(const FArguments& InArgs, TWeakPtr<FFlowAssetEditor> InFlowAssetEditorPtr)
{
	FlowAssetEditorPtr = InFlowAssetEditorPtr;

	CategoryNames.Add(MakeShareable(new FString(TEXT("All"))));
	CategoryNames.Append(UFlowGraphSchema::GetFlowNodeCategories());

	this->ChildSlot
		[
			SNew(SBorder)
			.Padding(2.0f)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)

				// Filter UI
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					// Combo button to select a class
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SAssignNew(CategoryComboBox, STextComboBox)
						.OptionsSource(&CategoryNames)
						.OnSelectionChanged(this, &SFlowPalette::CategorySelectionChanged)
						.InitiallySelectedItem(CategoryNames[0])
					]
				]

				// Content list
				+ SVerticalBox::Slot()
					[
						SNew(SOverlay)

						+ SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						[
							// Old Expression and Function lists were auto expanded so do the same here for now
							SAssignNew(GraphActionMenu, SGraphActionMenu)
							.OnActionDragged(this, &SFlowPalette::OnActionDragged)
							.OnCreateWidgetForAction(this, &SFlowPalette::OnCreateWidgetForAction)
							.OnCollectAllActions(this, &SFlowPalette::CollectAllActions)
							.AutoExpandActionMenu(true)
						]
				]
			]
		];
}

TSharedRef<SWidget> SFlowPalette::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return	SNew(SFlowPaletteItem, InCreateData);
}

void SFlowPalette::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	const UFlowGraphSchema* Schema = GetDefault<UFlowGraphSchema>();

	FGraphActionMenuBuilder ActionMenuBuilder;

	// Determine all possible actions
	Schema->GetPaletteActions(ActionMenuBuilder, GetFilterCategoryName());

	//@TODO: Avoid this copy
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

#undef LOCTEXT_NAMESPACE