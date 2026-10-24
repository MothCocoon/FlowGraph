// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "IPropertyTypeCustomization.h"

class IPropertyHandle;

/**
 * Customizes an instanced Flow Node AddOn property row in the Details panel.
 * Shows the AddOn class instead of the object path and lists the AddOn properties as children.
 */
class FFlowNodeAddOnCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	static TArray<UObject*> GetAddOns(const TSharedRef<IPropertyHandle>& PropertyHandle);
	static TSharedPtr<IPropertyHandle> FindAddOnsHandle(const TSharedRef<IPropertyHandle>& RowHandle);
};
