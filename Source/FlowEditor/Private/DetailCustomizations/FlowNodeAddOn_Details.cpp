// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNodeAddOn_Details.h"

#include "AddOns/FlowNodeAddOn.h"
#include "DetailCustomizations/FlowDetailsAddOnUI.h"
#include "Graph/FlowGraphEditorSettings.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowNodeAddOnDetails"

void FFlowNodeAddOn_Details::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// hide class properties while editing node addon instance placed in the graph
	if (DetailLayout.HasClassDefaultObject() == false)
	{
		if (GetDefault<UFlowGraphEditorSettings>()->bMergeAddOnDetails)
		{
			IDetailCategoryBuilder& AddOnDetailsCategory = DetailLayout.EditCategory(
				TEXT("AddOnDetails"),
				LOCTEXT("AddOnDetailsCategory", "AddOn Details"),
				ECategoryPriority::Uncommon);

			TSharedRef<IPropertyHandle> AddOnsProperty = DetailLayout.GetProperty(TEXT("AddOns"), UFlowNodeBase::StaticClass());
			IDetailPropertyRow& AddOnsRow = AddOnDetailsCategory.AddProperty(AddOnsProperty);
			AddOnsRow.CustomWidget(true)
				.NameContent()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AddOnDetailsProperty", "Child AddOns"))
					.ToolTipText(LOCTEXT("AddOnDetailsTooltip", "AddOns attached to this AddOn."))
				]
				.ValueContent()
				[
					SNew(STextBlock)
				];
			AddOnsRow.ShouldAutoExpand(true);
		}
	}

	// Cache edited addon
	{
		TArray<TWeakObjectPtr<UObject>> Objects;
		DetailLayout.GetObjectsBeingCustomized(Objects);

		EditedAddOn = nullptr;

		for (const TWeakObjectPtr<UObject>& Obj : Objects)
		{
			if (UFlowNodeAddOn* AsAddOn = Cast<UFlowNodeAddOn>(Obj.Get()))
			{
				EditedAddOn = AsAddOn;

				break;
			}
		}
	}

	// Add "Attach AddOn..." dropdown (menu button)
	if (EditedAddOn.IsValid() && !DetailLayout.HasClassDefaultObject())
	{
		IDetailCategoryBuilder& AddOnsCategory = DetailLayout.EditCategory(
			TEXT("AddOns"),
			LOCTEXT("AddOnsCategory", "AddOns"),
			ECategoryPriority::Important);

		AddOnsCategory.AddCustomRow(LOCTEXT("AttachAddOnSearch", "Attach AddOn"))
		.WholeRowContent()
		[
			SNew(SComboButton)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AttachAddOnButton", "Attach AddOn..."))
			]
			.ToolTipText(LOCTEXT("AttachAddOnButtonTooltip", "Attach an AddOn to the selected node/addon."))
			.IsEnabled_Lambda([this]()
			{
				return EditedAddOn.IsValid() && FFlowDetailsAddOnUI::CanAttachAddOn(EditedAddOn.Get());
			})
			.OnGetMenuContent_Lambda([this]()
			{
				return EditedAddOn.IsValid()
					? FFlowDetailsAddOnUI::BuildAttachAddOnMenuContent(EditedAddOn.Get())
					: SNullWidget::NullWidget;
			})
		];
	}

	// Call base template to set up rebuild delegate wiring
	TFlowDataPinValueOwnerCustomization<UFlowNodeAddOn>::CustomizeDetails(DetailLayout);
}

#undef LOCTEXT_NAMESPACE