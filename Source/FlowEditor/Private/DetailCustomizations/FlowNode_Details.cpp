// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNode_Details.h"

#include "DetailCustomizations/FlowDetailsAddOnUI.h"
#include "Nodes/FlowNode.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowNodeDetails"

void FFlowNode_Details::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Hide class-level category when editing an instance (not the CDO)
	if (!DetailLayout.HasClassDefaultObject())
	{
		DetailLayout.HideCategory(TEXT("FlowNode"));
	}

	// Cache edited object
	{
		TArray<TWeakObjectPtr<UObject>> Objects;
		DetailLayout.GetObjectsBeingCustomized(Objects);

		EditedNode = nullptr;
		for (const TWeakObjectPtr<UObject>& Obj : Objects)
		{
			if (UFlowNode* AsNode = Cast<UFlowNode>(Obj.Get()))
			{
				EditedNode = AsNode;
				break;
			}
		}
	}

	// Add "Attach AddOn..." dropdown (menu button)
	if (EditedNode.IsValid() && !DetailLayout.HasClassDefaultObject())
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
				return EditedNode.IsValid() && FFlowDetailsAddOnUI::CanAttachAddOn(EditedNode.Get());
			})
			.OnGetMenuContent_Lambda([this]()
			{
				return EditedNode.IsValid()
					? FFlowDetailsAddOnUI::BuildAttachAddOnMenuContent(EditedNode.Get())
					: SNullWidget::NullWidget;
			})
		];
	}

	// Call base template to set up rebuild delegate wiring
	TFlowDataPinValueOwnerCustomization<UFlowNode>::CustomizeDetails(DetailLayout);
}

#undef LOCTEXT_NAMESPACE