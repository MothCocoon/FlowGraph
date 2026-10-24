// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "SGraphPin.h"
#include "SNameComboBox.h"

class FLOWEDITOR_API SFlowPinHandle : public SGraphPin
{
public:
	virtual ~SFlowPinHandle() override;

	SLATE_BEGIN_ARGS(SFlowPinHandle) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj, const UBlueprint* InBlueprint);
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;

protected:
	virtual void RefreshNameList() {}
	void ParseDefaultValueData();
	void ComboBoxSelectionChanged(const TSharedPtr<FName> NameItem, ESelectInfo::Type SelectInfo) const;

	const UBlueprint* Blueprint = nullptr;
	TArray<TSharedPtr<FName>> PinNames;
	
	TSharedPtr<SNameComboBox> ComboBox;
	TSharedPtr<FName> CurrentlySelectedName;
};
