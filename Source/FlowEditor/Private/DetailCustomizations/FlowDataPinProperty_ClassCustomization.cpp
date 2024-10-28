// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowDataPinProperty_ClassCustomization.h"
#include "DetailWidgetRow.h"
#include "Types/FlowDataPinProperties.h"
#include "EditorClassUtils.h"
#include "PropertyCustomizationHelpers.h"
#include "IDetailChildrenBuilder.h"

void FFlowDataPinProperty_ClassCustomizationBase::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;

	// Based on SoftClassPtr Customization

	const FString& MustImplementName = StructPropertyHandle->GetMetaData("MustImplement");
	const bool bAllowAbstract = StructPropertyHandle->HasMetaData("AllowAbstract");
	const bool bIsBlueprintBaseOnly = StructPropertyHandle->HasMetaData("IsBlueprintBaseOnly") || StructPropertyHandle->HasMetaData("BlueprintBaseOnly");
	const bool bAllowNone = !(StructPropertyHandle->GetMetaDataProperty()->PropertyFlags & CPF_NoClear);
	const bool bShowTreeView = StructPropertyHandle->HasMetaData("ShowTreeView");
	const bool bHideViewOptions = StructPropertyHandle->HasMetaData("HideViewOptions");
	const bool bShowDisplayNames = StructPropertyHandle->HasMetaData("ShowDisplayNames");

	CachedMetaClassPtr = DeriveBestClassFilter();

	TrySetClassFilterFromMetaData();

	const UClass* const RequiredInterface = FEditorClassUtils::GetClassFromString(MustImplementName);

	HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		.MaxDesiredWidth(0.0f)
		[
			// Add a class entry box.  Even though this isn't an class entry, we will simulate one
			SNew(SClassPropertyEntryBox)
				.MetaClass(BuildMetaClass())
				.RequiredInterface(RequiredInterface)
				.AllowAbstract(bAllowAbstract)
				.IsBlueprintBaseOnly(bIsBlueprintBaseOnly)
				.AllowNone(bAllowNone)
				.ShowTreeView(bShowTreeView)
				.HideViewOptions(bHideViewOptions)
				.ShowDisplayNames(bShowDisplayNames)
				.SelectedClass(this, &FFlowDataPinProperty_ClassCustomizationBase::OnGetClass)
				.OnSetClass(this, &FFlowDataPinProperty_ClassCustomizationBase::OnSetClass)
		];
}

void FFlowDataPinProperty_ClassCustomizationBase::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	if (TSharedPtr<IPropertyHandle> ClassFilterHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Class, ClassFilter)))
	{
		StructBuilder.AddProperty(ClassFilterHandle.ToSharedRef());

		ClassFilterHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FFlowDataPinProperty_ClassCustomizationBase::OnClassFilterChanged));
	}
}

void FFlowDataPinProperty_ClassCustomizationBase::OnClassFilterChanged()
{
	// We don't allow changing away from the class filter specified in property metadata.
	// So potentially undo the change (would be better to make it non-editable if the metadata was set, but I'm not sure how to do that)
	TrySetClassFilterFromMetaData();

	UClass* MetaClass = DeriveBestClassFilter();

	TSharedPtr<IPropertyHandle> ClassValueHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Class, Value));
	UObject* ClassValueAsObject = nullptr;
	ClassValueHandle->GetValue(ClassValueAsObject);
	UClass* ClassValue = Cast<UClass>(ClassValueAsObject);
	
	if (MetaClass && ClassValue && !ClassValue->IsChildOf(MetaClass))
	{
		// Clear the class value if it is not compatible with the new ClassFilter value
		const UClass* NullClassPtr = nullptr;
		ClassValueHandle->SetValue(NullClassPtr, EPropertyValueSetFlags::DefaultFlags);
	}

	CachedMetaClassPtr = MetaClass;

	IFlowExtendedPropertyTypeCustomization::OnAnyChildPropertyChanged();
}

UClass* FFlowDataPinProperty_ClassCustomizationBase::DeriveBestClassFilter() const
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
	if (TSharedPtr<IPropertyHandle> ClassFilterHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Class, ClassFilter)))
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

void FFlowDataPinProperty_ClassCustomizationBase::TrySetClassFilterFromMetaData()
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

	// If the class filter was set in meta data, force that value to the ClassFilter property
	if (TSharedPtr<IPropertyHandle> ClassFilterHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Class, ClassFilter)))
	{
		UObject* ExistingMetaClass = nullptr;
		ClassFilterHandle->GetValue(ExistingMetaClass);

		if (ExistingMetaClass != MetaClass)
		{
			ClassFilterHandle->SetValue(MetaClass, EPropertyValueSetFlags::DefaultFlags);
		}
	}
}

const UClass* FFlowDataPinProperty_ClassCustomizationBase::OnGetClass() const
{
	FString ClassName;

	if (TSharedPtr<IPropertyHandle> ClassValueHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Class, Value)))
	{
		ClassValueHandle->GetValueAsFormattedString(ClassName);
	}

	// Do we have a valid cached class pointer?
	const UClass* Class = CachedClassPtr.Get();
	if (!Class || Class->GetPathName() != ClassName)
	{
		Class = FEditorClassUtils::GetClassFromString(ClassName);
		CachedClassPtr = MakeWeakObjectPtr(const_cast<UClass*>(Class));
	}
	return Class;
}

UClass* FFlowDataPinProperty_ClassCustomizationBase::BuildMetaClass() const
{
	return CachedMetaClassPtr.Get();
}

void FFlowDataPinProperty_ClassCustomizationBase::OnSetClass(const UClass* NewClass)
{
	if (TSharedPtr<IPropertyHandle> ClassValueHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Class, Value)))
	{
		if (ClassValueHandle->SetValueFromFormattedString((NewClass) ? NewClass->GetPathName() : "None") == FPropertyAccess::Result::Success)
		{
			CachedClassPtr = MakeWeakObjectPtr(const_cast<UClass*>(NewClass));
		}
	}
}
