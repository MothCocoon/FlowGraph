// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "IDetailCustomization.h"
#include "Templates/SharedPointer.h"
#include "Types/SlateEnums.h"

class UFlowNode_CustomEventBase;
class UFlowAsset;
class IDetailChildrenBuilder;
class IDetailLayoutBuilder;
class IPropertyHandle;

class FFlowAssetDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowAssetDetails());
	}

	// IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// --

private:
	void GenerateCustomPinArray(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder);

	FText GetCustomPinText(TSharedRef<IPropertyHandle> PropertyHandle) const;
	static void OnCustomPinTextCommitted(const FText& InText, ETextCommit::Type InCommitType, TSharedRef<IPropertyHandle> PropertyHandle);
	static bool VerifyNewCustomPinText(const FText& InNewText, FText& OutErrorMessage);

	void OnBrowseClicked(TSharedRef<IPropertyHandle> PropertyHandle);
	bool IsBrowseEnabled(TSharedRef<IPropertyHandle> PropertyHandle) const;
	UFlowNode_CustomEventBase* GetCustomEventNode(TSharedRef<IPropertyHandle> PropertyHandle) const;
	
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;

	TSharedPtr<IPropertyHandle> CustomInputsHandle;
	TSharedPtr<IPropertyHandle> CustomOutputsHandle;
	
};
