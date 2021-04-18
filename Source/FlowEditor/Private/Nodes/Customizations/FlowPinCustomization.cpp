#include "FlowPinCustomization.h"
#include "Nodes/FlowPin.h"

#include "PropertyEditing.h"

#define LOCTEXT_NAMESPACE "FlowPinCustomization"

void FFlowPinCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

void FFlowPinCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	TArray<UObject*> Objects;
	StructPropertyHandle->GetOuterObjects(Objects);
	const FFlowPin* FlowPin = reinterpret_cast<FFlowPin*>(StructPropertyHandle->GetValueBaseAddress(reinterpret_cast<uint8*>(Objects[0])));
	
	uint32 PropertyChildNum = -1;
	if (StructPropertyHandle->GetNumChildren(PropertyChildNum) == FPropertyAccess::Success)
	{
		for (uint32 i = 0; i < PropertyChildNum; ++i)
		{
			TSharedPtr<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(i);

			if (ChildHandle.IsValid() && ChildHandle->GetProperty())
			{
				const FName PropertyName = ChildHandle->GetProperty()->GetFName();
				if (PropertyName == GET_MEMBER_NAME_CHECKED(FFlowPin, PinName))
				{
					StructBuilder.AddCustomRow(LOCTEXT("PinNameValue", "PinNameValue"))
                        .NameContent()
	                        [
	                            StructPropertyHandle->CreatePropertyNameWidget()
	                        ]
						.ValueContent()
	                        .MinDesiredWidth(125.0f)
	                        .MaxDesiredWidth(125.0f)
	                        [
		                        SNew(SEditableTextBox)
		                            .Text(FText::FromName(FlowPin->PinName))
		                            .OnTextCommitted_Static(&FFlowPinCustomization::OnPinNameCommitted, ChildHandle.ToSharedRef())
		                            .OnVerifyTextChanged_Static(&FFlowPinCustomization::VerifyNewPinName)
	                        ];
				}
				else
				{
					StructBuilder.AddProperty(ChildHandle.ToSharedRef());
				}
			}
		}
	}
}

void FFlowPinCustomization::OnPinNameCommitted(const FText& InText, ETextCommit::Type InCommitType, TSharedRef<IPropertyHandle> PropertyHandle)
{
	TArray<UObject*> Objects;
	PropertyHandle->GetParentHandle()->GetOuterObjects(Objects);
	if (FFlowPin* FlowText = reinterpret_cast<FFlowPin*>(PropertyHandle->GetValueBaseAddress(reinterpret_cast<uint8*>(Objects[0]))))
	{
		FlowText->PinName = *InText.ToString();
	}
}

bool FFlowPinCustomization::VerifyNewPinName(const FText& InNewText, FText& OutErrorMessage)
{
	const FName NewString = *InNewText.ToString();

	if (NewString.IsNone())
	{
		OutErrorMessage = LOCTEXT("VerifyTextFailed", "Pin Name can't be None!");
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
