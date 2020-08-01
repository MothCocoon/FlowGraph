#pragma once

#include "IDetailCustomization.h"

class FFlowNode_CustomOutputCustomization final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowNode_CustomOutputCustomization());
	}

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// --

private:
	void GetEventNames();
	TSharedRef<SWidget> GenerateEventWidget(TSharedPtr<FName> Item) const;
	FText GetSelectedEventText() const;
	void PinSelectionChanged(TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo) const;

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;
	TArray<TSharedPtr<FName>> EventNames;
};
