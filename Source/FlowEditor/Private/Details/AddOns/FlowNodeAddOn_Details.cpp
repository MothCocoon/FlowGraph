// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Details/AddOns/FlowNodeAddOn_Details.h"

#include "AddOns/FlowNodeAddOn.h"
#include "Details/AddOns/FlowDetailsAddOnUI.h"
#include "Graph/FlowGraphEditorSettings.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowNodeAddOnDetails"

TSharedRef<IDetailCustomization> FFlowNodeAddOn_Details::MakeInstance()
{
	return MakeShareable(new FFlowNodeAddOn_Details());
}

void FFlowNodeAddOn_Details::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Merged AddOn list, shown on instances only
	if (DetailLayout.HasClassDefaultObject() == false)
	{
		if (GetDefault<UFlowGraphEditorSettings>()->bShowAddOnsInNodeDetails)
		{
			IDetailCategoryBuilder& AddOnDetailsCategory = DetailLayout.EditCategory(
				TEXT("AddOnDetails"),
				LOCTEXT("AddOnDetailsCategory", "AddOn Details"),
				ECategoryPriority::Uncommon);

			TArray<TWeakObjectPtr<UObject>> EditedObjects;
			DetailLayout.GetObjectsBeingCustomized(EditedObjects);

			TSharedRef<IPropertyHandle> AddOnsProperty = DetailLayout.GetProperty(TEXT("AddOns"), UFlowNodeBase::StaticClass());
			IDetailPropertyRow& AddOnsRow = AddOnDetailsCategory.AddProperty(AddOnsProperty);
			AddOnsRow.CustomWidget(true)
				.Visibility(TAttribute<EVisibility>::CreateLambda([EditedObjects]()
				{
					return FFlowDetailsAddOnUI::GetAddOnsVisibility(EditedObjects);
				}))
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
				UFlowNodeAddOn* EditedAddOnPtr = EditedAddOn.Get();
				return EditedAddOnPtr && FFlowDetailsAddOnUI::CanAttachAddOn(EditedAddOnPtr);
			})
			.OnGetMenuContent_Lambda([this]()
			{
				UFlowNodeAddOn* EditedAddOnPtr = EditedAddOn.Get();
				return EditedAddOnPtr
					? FFlowDetailsAddOnUI::BuildAttachAddOnMenuContent(EditedAddOnPtr)
					: SNullWidget::NullWidget;
			})
		];
	}

	// Call base template to set up rebuild delegate wiring
	TFlowDataPinValueOwnerCustomization<UFlowNodeAddOn>::CustomizeDetails(DetailLayout);
}

#undef LOCTEXT_NAMESPACE