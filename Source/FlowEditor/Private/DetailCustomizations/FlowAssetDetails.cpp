// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowAssetDetails.h"
#include "FlowAsset.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"

#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphUtils.h"

#include "Nodes/Graph/FlowNode_CustomInput.h"
#include "Nodes/Graph/FlowNode_CustomOutput.h"

#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "FlowAssetDetails"

void FFlowAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingEdited);

	IDetailCategoryBuilder& FlowAssetCategory = DetailBuilder.EditCategory("SubGraph", LOCTEXT("SubGraphCategory", "Sub Graph"));

	TArray<TSharedPtr<IPropertyHandle>> ArrayPropertyHandles;
	CustomInputsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomInputs));
	CustomOutputsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomOutputs));
	ArrayPropertyHandles.Add(CustomInputsHandle);
	ArrayPropertyHandles.Add(CustomOutputsHandle);
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
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(2.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SEditableTextBox)
				.Text(this, &FFlowAssetDetails::GetCustomPinText, PropertyHandle)
				.OnTextCommitted_Static(&FFlowAssetDetails::OnCustomPinTextCommitted, PropertyHandle)
				.OnVerifyTextChanged_Static(&FFlowAssetDetails::VerifyNewCustomPinText)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
            .VAlign(VAlign_Center)
			[
				PropertyCustomizationHelpers::MakeBrowseButton(
					FSimpleDelegate::CreateRaw(this, &FFlowAssetDetails::OnBrowseClicked, PropertyHandle),
					LOCTEXT("SelectEventNode", "Select Event Node in Graph"),
					TAttribute<bool>::CreateRaw(this, &FFlowAssetDetails::IsBrowseEnabled, PropertyHandle),
					true) // intentionally true, to set "correct" icon
			]
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

void FFlowAssetDetails::OnBrowseClicked(TSharedRef<IPropertyHandle> PropertyHandle)
{
	ensure(ObjectsBeingEdited[0].IsValid());

	UFlowAsset* Asset = Cast<UFlowAsset>(ObjectsBeingEdited[0]);
	UFlowNode_CustomEventBase* EventNode = GetCustomEventNode(PropertyHandle);
	
	if (EventNode)
	{
		TSharedPtr<SFlowGraphEditor> Editor = FFlowGraphUtils::GetFlowGraphEditor(Asset->GetGraph());
		Editor->ClearSelectionSet();
		Editor->SelectSingleNode(EventNode->GetGraphNode());
		Editor->ZoomToFit(true);
	}
}

bool FFlowAssetDetails::IsBrowseEnabled(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	return GetCustomEventNode(PropertyHandle) != nullptr;
}

UFlowNode_CustomEventBase* FFlowAssetDetails::GetCustomEventNode(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	ensure(ObjectsBeingEdited[0].IsValid());

	UFlowAsset* Asset = Cast<UFlowAsset>(ObjectsBeingEdited[0]);
	FName Text = FName(GetCustomPinText( PropertyHandle ).ToString());
	TSharedPtr<IPropertyHandle> ArrayHandle = PropertyHandle->GetParentHandle();

	if (ArrayHandle->IsSamePropertyNode(CustomInputsHandle))
	{
		UFlowNode_CustomInput* Input = Asset->TryFindCustomInputNodeByEventName(Text);
		if (Input)
		{
			return Input;
		}
	}
	else if (ArrayHandle->IsSamePropertyNode(CustomOutputsHandle))
	{
		UFlowNode_CustomOutput* Output = Asset->TryFindCustomOutputNodeByEventName(Text);
		if (Output)
		{
			return Output;
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
