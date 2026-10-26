// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Details/Pins/FlowDataPinValueOwnerCustomization.h"
#include "IDetailCustomization.h"
#include "Templates/SharedPointer.h"

class UFlowNodeAddOn;

class FFlowNodeAddOn_Details final : public TFlowDataPinValueOwnerCustomization<UFlowNodeAddOn>
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// --

private:
	TWeakObjectPtr<UFlowNodeAddOn> EditedAddOn = nullptr;
};