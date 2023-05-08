// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowNode_CustomNodeBaseDetails.h"
#include "Templates/SharedPointer.h"

class FFlowNode_CustomInputDetails final : public FFlowNode_CustomNodeBaseDetails
{
public:
	FFlowNode_CustomInputDetails();

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowNode_CustomInputDetails());
	}

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// --

protected:
	virtual IDetailCategoryBuilder& CreateDetailCategory(IDetailLayoutBuilder& DetailLayout) const override;
	virtual TArray<FName> BuildEventNames(const UFlowAsset& FlowAsset) const override;
};
