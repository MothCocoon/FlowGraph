// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowDataPinValueOwnerCustomization.h"
#include "IDetailCustomization.h"
#include "Templates/SharedPointer.h"

class UFlowNodeAddOn;

class FFlowNodeAddOn_Details final : public TFlowDataPinValueOwnerCustomization<UFlowNodeAddOn>
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowNodeAddOn_Details());
	}

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// --
};
