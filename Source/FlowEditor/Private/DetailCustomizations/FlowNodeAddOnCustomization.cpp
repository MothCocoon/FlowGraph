// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "DetailCustomizations/FlowNodeAddOnCustomization.h"

#include "DetailCustomizations/FlowDetailsAddOnUI.h"
#include "Nodes/FlowNodeBase.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowNodeAddOnCustomization"

TSharedRef<IPropertyTypeCustomization> FFlowNodeAddOnCustomization::MakeInstance()
{
	return MakeShareable(new FFlowNodeAddOnCustomization());
}

void FFlowNodeAddOnCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TArray<UObject*> AddOns = GetAddOns(PropertyHandle);

	FText TitleText;
	if (AddOns.Num() == 1)
	{
		if (const UFlowNodeBase* AddOn = Cast<UFlowNodeBase>(AddOns[0]))
		{
			TitleText = AddOn->GetNodeTitle();
		}
	}

	FText ClassText;
	if (AddOns.Num() > 0)
	{
		const UClass* AddOnClass = AddOns[0]->GetClass();
		const bool bSameClass = AddOns.ContainsByPredicate([AddOnClass](const UObject* AddOn)
		{
			return AddOn->GetClass() != AddOnClass;
		}) == false;

		ClassText = bSameClass ? AddOnClass->GetDisplayNameText() : LOCTEXT("MultipleClasses", "Multiple Values");
	}

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget(TitleText)
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(ClassText)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];
}

void FFlowNodeAddOnCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TArray<TWeakObjectPtr<UObject>> AddOnObjects(GetAddOns(PropertyHandle));

	// The only child is the inline AddOn object node, its children are the AddOn categories
	const TSharedPtr<IPropertyHandle> AddOnObjectHandle = PropertyHandle->GetChildHandle(0);
	if (AddOnObjectHandle.IsValid())
	{
		uint32 NumChildren = 0;
		AddOnObjectHandle->GetNumChildren(NumChildren);

		for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
		{
			const TSharedPtr<IPropertyHandle> ChildHandle = AddOnObjectHandle->GetChildHandle(ChildIndex);
			if (ChildHandle.IsValid())
			{
				const TSharedPtr<IPropertyHandle> AddOnsHandle = FindAddOnsHandle(ChildHandle.ToSharedRef());
				if (AddOnsHandle.IsValid())
				{
					// Child AddOns replace their category row and are shown only while the AddOn has child AddOns
					ChildBuilder.AddProperty(AddOnsHandle.ToSharedRef())
						.DisplayName(LOCTEXT("ChildAddOns", "Child AddOns"))
						.Visibility(TAttribute<EVisibility>::CreateLambda([AddOnObjects]()
						{
							return FFlowDetailsAddOnUI::GetAddOnsVisibility(AddOnObjects);
						}));
				}
				else
				{
					ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
				}
			}
		}
	}
}

TSharedPtr<IPropertyHandle> FFlowNodeAddOnCustomization::FindAddOnsHandle(const TSharedRef<IPropertyHandle>& RowHandle)
{
	TSharedPtr<IPropertyHandle> AddOnsHandle = RowHandle;

	// Category rows have no property, the AddOns category holds only the AddOns property
	if (RowHandle->GetProperty() == nullptr)
	{
		uint32 NumChildren = 0;
		RowHandle->GetNumChildren(NumChildren);
		AddOnsHandle = NumChildren == 1 ? RowHandle->GetChildHandle(0) : TSharedPtr<IPropertyHandle>();
	}

	const FProperty* Property = AddOnsHandle.IsValid() ? AddOnsHandle->GetProperty() : nullptr;
	return Property && Property->GetFName() == TEXT("AddOns") ? AddOnsHandle : TSharedPtr<IPropertyHandle>();
}

TArray<UObject*> FFlowNodeAddOnCustomization::GetAddOns(const TSharedRef<IPropertyHandle>& PropertyHandle)
{
	TArray<UObject*> AddOns;

	if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(PropertyHandle->GetProperty()))
	{
		TArray<const void*> RawData;
		PropertyHandle->AccessRawData(RawData);

		for (const void* Data : RawData)
		{
			if (Data)
			{
				if (UObject* AddOn = ObjectProperty->GetObjectPropertyValue(Data))
				{
					AddOns.Add(AddOn);
				}
			}
		}
	}

	return AddOns;
}

#undef LOCTEXT_NAMESPACE
