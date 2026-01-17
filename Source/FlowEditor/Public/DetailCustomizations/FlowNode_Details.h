// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowDataPinValueOwnerCustomization.h"
#include "IDetailCustomization.h"
#include "Templates/SharedPointer.h"

class UFlowNode;

class FFlowNode_Details final : public TFlowDataPinValueOwnerCustomization<UFlowNode>
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowNode_Details());
	}

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// --
};
