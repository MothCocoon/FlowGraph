// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UnrealExtensions/IFlowExtendedPropertyTypeCustomization.h"

struct FFlowPin;

// Details customization for FFlowPin
class FFlowPinCustomization : public IFlowExtendedPropertyTypeCustomization
{
	typedef IFlowExtendedPropertyTypeCustomization Super;

public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FFlowPinCustomization()); }

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:

	// Accessor to return the actual struct being edited
	FORCEINLINE FFlowPin* GetFlowPin() const
	{
		return IFlowExtendedPropertyTypeCustomization::TryGetTypedStructValue<FFlowPin>(StructPropertyHandle);
	}

	void OnChildPropertyValueChanged();

	virtual FText BuildHeaderText() const override;
};
