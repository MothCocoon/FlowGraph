// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EdGraphUtilities.h"
#include "SFlowPinHandle.h"

class FLOWEDITOR_API SFlowOutputPinHandle : public SFlowPinHandle
{
protected:
	virtual void RefreshNameList() override;
};

class FFlowOutputPinHandleFactory final : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
};
