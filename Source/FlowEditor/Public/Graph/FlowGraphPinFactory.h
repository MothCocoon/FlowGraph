// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"

struct FFlowPin;

class FFlowGraphPinFactory : public FGraphPanelPinFactory
{
public:
	// FGraphPanelPinFactory interface
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
	// --

	static int32 GatherValidPinsCount(const TArray<FFlowPin>& Pins);
};
