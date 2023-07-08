// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "IDetailCustomization.h"

class FFlowNode_SubGraphDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowNode_SubGraphDetails);
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
};
