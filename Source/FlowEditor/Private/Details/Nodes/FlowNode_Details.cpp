// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Details/Nodes/FlowNode_Details.h"

#include "Details/AddOns/FlowDetailsAddOnUI.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Nodes/FlowNode.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowNodeDetails"

TSharedRef<IDetailCustomization> FFlowNode_Details::MakeInstance()
{
	return MakeShareable(new FFlowNode_Details());
}

void FFlowNode_Details::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Merged AddOn list, shown on instances only
	if (!DetailLayout.HasClassDefaultObject())
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
					AddOnsProperty->CreatePropertyNameWidget(LOCTEXT("AddOnDetailsProperty", "AddOns"))
				]
				.ValueContent()
				[
					AddOnsProperty->CreatePropertyValueWidget(false)
				];
			AddOnsRow.ShouldAutoExpand(true);
		}
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
				UFlowNode* EditedNodePtr = EditedNode.Get();
				return EditedNodePtr && FFlowDetailsAddOnUI::CanAttachAddOn(EditedNodePtr);
			})
			.OnGetMenuContent_Lambda([this]()
			{
				UFlowNode* EditedNodePtr = EditedNode.Get();
				return EditedNodePtr
					? FFlowDetailsAddOnUI::BuildAttachAddOnMenuContent(EditedNodePtr)
					: SNullWidget::NullWidget;
			})
		];
	}

	// Call base template to set up rebuild delegate wiring
	TFlowDataPinValueOwnerCustomization<UFlowNode>::CustomizeDetails(DetailLayout);
}

#undef LOCTEXT_NAMESPACE