// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Pins/SFlowPinHandle.h"

#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SFlowPinHandle"

SFlowPinHandle::SFlowPinHandle()
	: Blueprint(nullptr)
{
}

SFlowPinHandle::~SFlowPinHandle()
{
	Blueprint = nullptr;
}

void SFlowPinHandle::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj, const UBlueprint* InBlueprint)
{
	Blueprint = InBlueprint;
	RefreshNameList();
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget> SFlowPinHandle::GetDefaultValueWidget()
{
	ParseDefaultValueData();

	// Create widget
	return SAssignNew(ComboBox, SNameComboBox)
		.ContentPadding(FMargin(6.0f, 2.0f))
		.OptionsSource(&PinNames)
		.InitiallySelectedItem(CurrentlySelectedName)
		.OnSelectionChanged(this, &SFlowPinHandle::ComboBoxSelectionChanged)
		.Visibility(this, &SGraphPin::GetDefaultValueVisibility);
}

void SFlowPinHandle::ParseDefaultValueData()
{
	FString DefaultValue = GraphPinObj->GetDefaultAsString();
	if (DefaultValue.StartsWith(TEXT("(PinName=")) && DefaultValue.EndsWith(TEXT(")")))
	{
		DefaultValue.Split(TEXT("PinName=\""), nullptr, &DefaultValue);
		DefaultValue.Split(TEXT("\""), &DefaultValue, nullptr);
	}

	// Preserve previous selection
	if (PinNames.Num() > 0)
	{
		for (TSharedPtr<FName> PinName : PinNames)
		{
			if (*PinName.Get() == *DefaultValue)
			{
				CurrentlySelectedName = PinName;
				break;
			}
		}
	}
}

void SFlowPinHandle::ComboBoxSelectionChanged(const TSharedPtr<FName> NameItem, ESelectInfo::Type SelectInfo) const
{
	const FName Name = NameItem.IsValid() ? *NameItem : NAME_None;
	if (const UEdGraphSchema* Schema = (GraphPinObj ? GraphPinObj->GetSchema() : nullptr))
	{
		const FString ValueString = TEXT("(PinName=\"") + Name.ToString() + TEXT("\")");

		if (GraphPinObj->GetDefaultAsString() != ValueString)
		{
			const FScopedTransaction Transaction(LOCTEXT("ChangePinValue", "Change Pin Value"));
			GraphPinObj->Modify();

			Schema->TrySetDefaultValue(*GraphPinObj, ValueString);
		}
	}
}

#undef LOCTEXT_NAMESPACE
