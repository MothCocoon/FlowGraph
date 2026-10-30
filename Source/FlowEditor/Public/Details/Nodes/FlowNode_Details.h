// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Details/Pins/FlowDataPinValueOwnerCustomization.h"
#include "IDetailCustomization.h"
#include "Templates/SharedPointer.h"

class UFlowNode;

class FFlowNode_Details final : public TFlowDataPinValueOwnerCustomization<UFlowNode>
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// --

private:
	TWeakObjectPtr<UFlowNode> EditedNode = nullptr;
};