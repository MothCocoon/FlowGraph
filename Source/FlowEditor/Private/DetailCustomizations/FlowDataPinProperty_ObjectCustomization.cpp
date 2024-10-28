// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinProperty_ObjectCustomization.h"
#include "DetailWidgetRow.h"
#include "Types/FlowDataPinProperties.h"
#include "EditorClassUtils.h"
#include "IDetailChildrenBuilder.h"

void FFlowDataPinProperty_ObjectCustomizationBase::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;

	CachedMetaClassPtr = DeriveBestClassFilter();

	TrySetClassFilterFromMetaData();

	// NOTE (gtaylor) Unfortunately, I wasn't able to get the customization filtering the object options using the ClassFilter
	// (like FFlowDataPinProperty_ClassCustomizationBase does), because the object selection widget is less customizable (compared to the Class selection widget).
	// Longer-term, this property customization could be improved to do this object filtering using the ClassFilter, 
	// but I don't have time to do that right now.

	HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		];

	// This avoids making duplicate reset boxes
	StructPropertyHandle->MarkResetToDefaultCustomized();
}

void FFlowDataPinProperty_ObjectCustomizationBase::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	if (TSharedPtr<IPropertyHandle> ClassFilterHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Object, ClassFilter)))
	{
		StructBuilder.AddProperty(ClassFilterHandle.ToSharedRef());

		ClassFilterHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FFlowDataPinProperty_ObjectCustomizationBase::OnClassFilterChanged));
	}

	if (TSharedPtr<IPropertyHandle> ReferenceValueHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Object, ReferenceValue)))
	{
		StructBuilder.AddProperty(ReferenceValueHandle.ToSharedRef());

		ReferenceValueHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FFlowDataPinProperty_ObjectCustomizationBase::OnObjectValueChanged));
	}

	if (TSharedPtr<IPropertyHandle> InlineValueHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Object, InlineValue)))
	{
		StructBuilder.AddProperty(InlineValueHandle.ToSharedRef());

		InlineValueHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FFlowDataPinProperty_ObjectCustomizationBase::OnObjectValueChanged));
	}
}

void FFlowDataPinProperty_ObjectCustomizationBase::OnClassFilterChanged()
{
	// We don't allow changing away from the Object filter specified in property metadata.
	// So potentially undo the change (would be better to make it non-editable if the metadata was set, but I'm not sure how to do that)
	TrySetClassFilterFromMetaData();

	UClass* MetaClass = DeriveBestClassFilter();
	void* ObjectValuePropertyAsVoid = nullptr; 
	StructPropertyHandle->GetValueData(ObjectValuePropertyAsVoid);
	FFlowDataPinOutputProperty_Object* ObjectValueProperty = static_cast<FFlowDataPinOutputProperty_Object*>(ObjectValuePropertyAsVoid);

	UObject* ObjectValue = ObjectValueProperty ? ObjectValueProperty->GetObjectValue() : nullptr;
	
	if (MetaClass && ObjectValue && !ObjectValue->IsA(MetaClass))
	{
		TSharedPtr<IPropertyHandle> ReferenceObjectValueHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Object, ReferenceValue));
		TSharedPtr<IPropertyHandle> InlineObjectValueHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Object, InlineValue));

		// Clear the Object value if it is not compatible with the new ClassFilter value
		const UObject* NullObjectPtr = nullptr;
		ReferenceObjectValueHandle->SetValue(NullObjectPtr, EPropertyValueSetFlags::DefaultFlags);
		InlineObjectValueHandle->SetValue(NullObjectPtr, EPropertyValueSetFlags::DefaultFlags);
	}

	CachedMetaClassPtr = MetaClass;

	IFlowExtendedPropertyTypeCustomization::OnAnyChildPropertyChanged();
}

void FFlowDataPinProperty_ObjectCustomizationBase::OnObjectValueChanged()
{
	OnClassFilterChanged();
}

UClass* FFlowDataPinProperty_ObjectCustomizationBase::DeriveBestClassFilter() const
{
	const FProperty* StructProperty = StructPropertyHandle->GetProperty();

	if (!StructProperty)
	{
		return nullptr;
	}

	if (UClass* MetaClass = FFlowDataPinOutputProperty_Class::TryGetMetaClassFromProperty(*StructProperty))
	{
		return MetaClass;
	}

	// Allow the Instance to edit the ClassFilter to override the MetaClass
	if (TSharedPtr<IPropertyHandle> ClassFilterHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Object, ClassFilter)))
	{
		UObject* ExistingMetaClass = nullptr;
		ClassFilterHandle->GetValue(ExistingMetaClass);

		if (ExistingMetaClass)
		{
			return Cast<UClass>(ExistingMetaClass);
		}
	}

	return nullptr;
}

void FFlowDataPinProperty_ObjectCustomizationBase::TrySetClassFilterFromMetaData()
{
	const FString& MetaClassName = StructPropertyHandle->GetMetaData("MetaClass");

	if (MetaClassName.IsEmpty())
	{
		return;
	}

	UClass* MetaClass = FEditorClassUtils::GetClassFromString(MetaClassName);
	if (!MetaClass)
	{
		return;
	}

	// If the Object filter was set in meta data, force that value to the ClassFilter property
	if (TSharedPtr<IPropertyHandle> ClassFilterHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Object, ClassFilter)))
	{
		UObject* ExistingMetaClass = nullptr;
		ClassFilterHandle->GetValue(ExistingMetaClass);

		if (ExistingMetaClass != MetaClass)
		{
			ClassFilterHandle->SetValue(MetaClass, EPropertyValueSetFlags::DefaultFlags);
		}
	}
}

UClass* FFlowDataPinProperty_ObjectCustomizationBase::BuildMetaClass() const
{
	return CachedMetaClassPtr.Get();
}
