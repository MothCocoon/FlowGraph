// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowAssetDetails.h"
#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_SubGraph.h"

#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditing.h"
#include "Widgets/Input/SEditableTextBox.h"

#define LOCTEXT_NAMESPACE "FlowAssetDetails"

void FFlowAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& FlowAssetCategory = DetailBuilder.EditCategory("SubGraph", LOCTEXT("SubGraphCategory", "Sub Graph"));

	TArray<TSharedPtr<IPropertyHandle>> ArrayPropertyHandles;
	ArrayPropertyHandles.Add(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomInputs)));
	ArrayPropertyHandles.Add(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomOutputs)));
	for (const TSharedPtr<IPropertyHandle>& PropertyHandle : ArrayPropertyHandles)
	{
		if (PropertyHandle.IsValid() && PropertyHandle->AsArray().IsValid())
		{
			const TSharedRef<FDetailArrayBuilder> ArrayBuilder = MakeShareable(new FDetailArrayBuilder(PropertyHandle.ToSharedRef()));
			ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FFlowAssetDetails::GenerateCustomPinArray));

			FlowAssetCategory.AddCustomBuilder(ArrayBuilder);
		}
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
			SNew(SEditableTextBox)
				.Text(this, &FFlowAssetDetails::GetCustomPinText, PropertyHandle)
				.OnTextCommitted_Static(&FFlowAssetDetails::OnCustomPinTextCommitted, PropertyHandle)
				.OnVerifyTextChanged_Static(&FFlowAssetDetails::VerifyNewCustomPinText)
		];
}

FText FFlowAssetDetails::GetCustomPinText(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	FText PropertyValue;
	const FPropertyAccess::Result GetValueResult = PropertyHandle->GetValueAsDisplayText(PropertyValue);
	ensure(GetValueResult == FPropertyAccess::Success);
	return PropertyValue;
}

void FFlowAssetDetails::OnCustomPinTextCommitted(const FText& InText, ETextCommit::Type InCommitType, TSharedRef<IPropertyHandle> PropertyHandle)
{
	const FPropertyAccess::Result SetValueResult = PropertyHandle->SetValueFromFormattedString(InText.ToString());
	ensure(SetValueResult == FPropertyAccess::Success);
}

bool FFlowAssetDetails::VerifyNewCustomPinText(const FText& InNewText, FText& OutErrorMessage)
{
	const FName NewString = *InNewText.ToString();

	if (NewString == UFlowNode_SubGraph::StartPin.PinName || NewString == UFlowNode_SubGraph::FinishPin.PinName)
	{
		OutErrorMessage = LOCTEXT("VerifyTextFailed", "This is a standard pin name of Sub Graph node!");
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
