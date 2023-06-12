// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

// NOTE (gtaylor) This class is planned for submission to Epic to include in baseline UE.
//  If/when that happens, we will want to remove this version and update to the latest one in the PropertyModule

#pragma once

#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"
#include "Templates/SharedPointer.h"

#include "IPropertyTypeCustomization.h"


// Forward Declarations
class STextBlock;
class FDetailWidgetRow;
class IDetailChildrenBuilder;
class IPropertyTypeCustomizationUtils;
class IDetailPropertyRow;
class IPropertyHandle;


// An extension of IPropertyTypeCustomization 
//  which adds some quality-of-life improvements for subclasses
class FLOWEDITOR_API IFlowExtendedPropertyTypeCustomization : public IPropertyTypeCustomization
{
public:

	// IPropertyTypeCustomization interface
	virtual void CustomizeHeader( TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override
		{ CustomizeChildrenDefaultImpl(InStructPropertyHandle, StructBuilder, StructCustomizationUtils); }

	static void CustomizeChildrenDefaultImpl(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils);

	template <class StructT>
	static StructT* TryGetTypedStructValue(const TSharedPtr<IPropertyHandle>& StructPropertyHandle);

protected:

	void RefreshHeader() const;

	virtual void CreateHeaderRowWidget(FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils);
	virtual FText BuildHeaderText() const;

	// Callbacks for property editor delegates
	void OnAnyChildPropertyChanged();

protected:

	// Cached struct property
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	// Header property text block, (re-)built in RefreshHeader
	TSharedPtr<STextBlock> HeaderTextBlock;
};


// Inline Implementations

template <class StructT>
StructT* IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue(const TSharedPtr<IPropertyHandle>& StructPropertyHandle)
{
	if (StructPropertyHandle.IsValid())
	{
		// Get the actual struct data from the handle and cast it to the correct type
		TArray<void*> RawData;
		StructPropertyHandle->AccessRawData(RawData);

		if (RawData.Num() > 0)
		{
			return reinterpret_cast<StructT*>(RawData[0]);
		}
	}

	return nullptr;
}
