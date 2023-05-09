// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowNode_CustomEventBaseDetails.h"
#include "Templates/SharedPointer.h"

class FFlowNode_CustomInputDetails final : public FFlowNode_CustomEventBaseDetails
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
