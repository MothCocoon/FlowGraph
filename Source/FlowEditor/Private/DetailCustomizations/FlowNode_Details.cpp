// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNode_Details.h"

#include "AddOns/FlowNodeAddOn.h"
#include "DetailCustomizations/FlowDetailsAddOnUI.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Nodes/FlowNode.h"

#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowNodeDetails"

void FFlowNode_Details::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Hide class-level category when editing an instance (not the CDO)
	if (!DetailLayout.HasClassDefaultObject())
	{
		if (GetDefault<UFlowGraphEditorSettings>()->bMergeAddOnDetails)
		{
			IDetailCategoryBuilder& AddOnDetailsCategory = DetailLayout.EditCategory(
				TEXT("AddOnDetails"),
				LOCTEXT("AddOnDetailsCategory", "AddOn Details"),
				ECategoryPriority::Uncommon);

			TSharedRef<IPropertyHandle> AddOnsProperty = DetailLayout.GetProperty(TEXT("AddOns"), UFlowNodeBase::StaticClass());
			const TSharedRef<FDetailArrayBuilder> AddOnsArrayBuilder = MakeShared<FDetailArrayBuilder>(
				AddOnsProperty,
				/*InGenerateHeader=*/true,
				/*InDisplayResetToDefault=*/false);
			AddOnsArrayBuilder->SetDisplayName(LOCTEXT("AddOnDetailsProperty", "AddOns"));
			AddOnsArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateLambda(
				[](TSharedRef<IPropertyHandle> ElementHandle, int32 /*ElementIndex*/, IDetailChildrenBuilder& ChildrenBuilder)
				{
					IDetailPropertyRow& AddOnRow = ChildrenBuilder.AddProperty(ElementHandle);

					UObject* AddOnObject = nullptr;
					if (ElementHandle->GetValue(AddOnObject) == FPropertyAccess::Success)
					{
						if (const UFlowNodeAddOn* AddOn = Cast<UFlowNodeAddOn>(AddOnObject))
						{
							AddOnRow.DisplayName(AddOn->GetNodeTitle());
						}
					}
				}));

			AddOnDetailsCategory.AddCustomBuilder(AddOnsArrayBuilder);
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