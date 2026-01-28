// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class IPropertyHandle;

// Customizes the FFlowAssetParamsPtr property in the Details panel.
class FFlowAssetParamsPtrCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	void HandleCreateNew();
	bool ShouldFilterAsset(const FAssetData& AssetData) const;
};