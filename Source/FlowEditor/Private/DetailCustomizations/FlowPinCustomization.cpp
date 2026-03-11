// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowPinCustomization.h"
#include "Nodes/FlowPin.h"
#include "IDetailChildrenBuilder.h"
#include "UObject/UnrealType.h"

void FFlowPinCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Add all of the child properties as normal, but also bind a callback when the property value changes
	uint32 NumChildren = 0;
	verify(InStructPropertyHandle->GetNumChildren(NumChildren) == FPropertyAccess::Success);
	for (uint32 ChildIdx = 0; ChildIdx < NumChildren; ++ChildIdx)
	{
		TSharedPtr<IPropertyHandle> ChildProperty = InStructPropertyHandle->GetChildHandle(ChildIdx);
		if (ChildProperty.IsValid())
		{
			ChildProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FFlowPinCustomization::OnChildPropertyValueChanged));

			ChildBuilder.AddProperty(ChildProperty.ToSharedRef());
		}
	}
}

void FFlowPinCustomization::OnChildPropertyValueChanged()
{
	if (FFlowPin* FlowPin = GetFlowPin())
	{
		IFlowExtendedPropertyTypeCustomization::OnAnyChildPropertyChanged();
	}
}

FText FFlowPinCustomization::BuildHeaderText() const
{
	if (const FFlowPin* FlowPin = GetFlowPin())
	{
		return FlowPin->BuildHeaderText();
	}

	return Super::BuildHeaderText();
}
