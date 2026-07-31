// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "IDetailCustomization.h"


class FFlowComponentDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowComponentDetails());
	}

	virtual ~FFlowComponentDetails() override;
	
	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// --
	
protected:
	virtual void ResolveCategoriesMeta(TSharedPtr<IPropertyHandle> PropertyHandle, FString& MetaString) const;
	
private:
	TSharedPtr<IPropertyHandle> IdentityTagsHandle;
};