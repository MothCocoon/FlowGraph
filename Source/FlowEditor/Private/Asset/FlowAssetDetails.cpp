#include "FlowAssetDetails.h"
#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_SubGraph.h"

#include "PropertyCustomizationHelpers.h"
#include "PropertyEditing.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "FlowAssetDetails"

void FFlowAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	IDetailCategoryBuilder& FlowAssetCategory = DetailLayout.EditCategory("FlowAsset", LOCTEXT("FlowAssetCategory", "FlowAsset"));
	
	const TSharedPtr<IPropertyHandle> EventsPropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomEvents));
	if (EventsPropertyHandle.IsValid())
	{
		TSharedRef<FDetailArrayBuilder> EventsArrayBuilder = MakeShareable(new FDetailArrayBuilder(EventsPropertyHandle.ToSharedRef()));
		EventsArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FFlowAssetDetails::GenerateCustomPinArray));

		FlowAssetCategory.AddCustomBuilder(EventsArrayBuilder);
	}

	const TSharedPtr<IPropertyHandle> OutputsPropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomOutputs));
	if (OutputsPropertyHandle.IsValid())
	{
		TSharedRef<FDetailArrayBuilder> OutputsArrayBuilder = MakeShareable(new FDetailArrayBuilder(OutputsPropertyHandle.ToSharedRef()));
		OutputsArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FFlowAssetDetails::GenerateCustomPinArray));

		FlowAssetCategory.AddCustomBuilder(OutputsArrayBuilder);
	}
}

void FFlowAssetDetails::GenerateCustomPinArray(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	IDetailPropertyRow& PropertyRow = ChildrenBuilder.AddProperty(PropertyHandle);
	PropertyRow.ShowPropertyButtons(true);
	PropertyRow.ShouldAutoExpand(true);

	PropertyRow.CustomWidget(false)
		.ValueContent()
		[
			SNew(SInlineEditableTextBlock)
				.Text(this, &FFlowAssetDetails::GetCustomPinText, PropertyHandle)
				.OnVerifyTextChanged_Static(&FFlowAssetDetails::OnCustomPinTextVerifyChanged)
				.OnTextCommitted(this, &FFlowAssetDetails::OnCustomPinTextCommited, PropertyHandle)
		];
}

FText FFlowAssetDetails::GetCustomPinText(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	FText PropertyValue;
	const FPropertyAccess::Result GetValueResult = PropertyHandle->GetValueAsDisplayText(PropertyValue);
	ensure(GetValueResult == FPropertyAccess::Success);
	return PropertyValue;
}

bool FFlowAssetDetails::OnCustomPinTextVerifyChanged(const FText& InNewText, FText& OutErrorMessage)
{
	const FName NewString = *InNewText.ToString();

	if (NewString == UFlowNode_SubGraph::StartPinName || NewString == UFlowNode_SubGraph::FinishPinName)
	{
		OutErrorMessage = LOCTEXT("VerifyTextFailed", "This is a standard pin name of Sub Graph node!");
		return false;
	}

	return true;
}

void FFlowAssetDetails::OnCustomPinTextCommited(const FText& InText, ETextCommit::Type InCommitType, TSharedRef<IPropertyHandle> PropertyHandle)
{
	const FPropertyAccess::Result SetValueResult = PropertyHandle->SetValueFromFormattedString(InText.ToString());
	ensure(SetValueResult == FPropertyAccess::Success);
}

#undef LOCTEXT_NAMESPACE
