// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

// NOTE (gtaylor) This class is planned for submission to Epic to include in baseline UE.
//  If/when that happens, we will want to remove this version and update to the latest one in the PropertyModule

#include "UnrealExtensions/IFlowCuratedNamePropertyCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EditorClassUtils.h"
#include "IDetailPropertyRow.h"
#include "IDetailChildrenBuilder.h"
#include "Internationalization/Text.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"


// IFlowCuratedNamePropertyCustomization Implementation

TSharedPtr<FText> IFlowCuratedNamePropertyCustomization::NoneAsText = nullptr;

void IFlowCuratedNamePropertyCustomization::Initialize()
{
	// Cache off "None" as a sharable FText, for use later
	if (!NoneAsText.IsValid())
	{
		NoneAsText = MakeShared<FText>(FText::FromName(NAME_None));
	}

	// Cache the Name property handle
	check(StructPropertyHandle.IsValid());
	CachedNameHandle = GetCuratedNamePropertyHandle();
	check(CachedNameHandle->IsValidHandle());

	// Initial setup the CachedTextSelected and CachedTextList
	//  (via SetCuratedNameWithSideEffects)
	check(!CachedTextSelected.IsValid());
	check(CachedTextList.IsEmpty());

	const FName CuratedName = GetCuratedName();
	const bool bChangedValue = TrySetCuratedNameWithSideEffects(CuratedName);

	check(bChangedValue);
	check(CachedTextSelected.IsValid());
	check(CachedTextList.Num() == 1);
}

void IFlowCuratedNamePropertyCustomization::CreateHeaderRowWidget(FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils & StructCustomizationUtils)
{
	// Do one-time setup first
	Initialize();

	// Replace the default HeaderRow widget with one of our own
	HeaderRow
		.NameContent()
		[
			SAssignNew(HeaderTextBlock, STextBlock)
			.Text(BuildHeaderText())
		]
		.ValueContent()
		.MaxDesiredWidth(250.0f)			
		[
			SAssignNew(TextListWidget, SComboBox<TSharedPtr<FText>>)
			.OptionsSource(&CachedTextList)
			.OnGenerateWidget(this, &IFlowCuratedNamePropertyCustomization::GenerateTextListWidget)
			.OnComboBoxOpening(this, &IFlowCuratedNamePropertyCustomization::OnTextListComboBoxOpening)
			.OnSelectionChanged(this, &IFlowCuratedNamePropertyCustomization::OnTextSelected)
			[
				SNew(STextBlock)
				.Text(this, &IFlowCuratedNamePropertyCustomization::GetCachedText)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ToolTipText(this, &IFlowCuratedNamePropertyCustomization::GetCachedText)
			] 
		];

	// Hook-up the ResetToDefault overrides
	FIsResetToDefaultVisible IsResetVisible =
		FIsResetToDefaultVisible::CreateSP(
			this,
			&IFlowCuratedNamePropertyCustomization::CustomIsResetToDefaultVisible);
	FResetToDefaultHandler ResetHandler = 
		FResetToDefaultHandler::CreateSP(
			this,
			&IFlowCuratedNamePropertyCustomization::CustomResetToDefault);
	FResetToDefaultOverride ResetOverride = FResetToDefaultOverride::Create(IsResetVisible, ResetHandler);

	HeaderRow.OverrideResetToDefault(ResetOverride);
}

bool IFlowCuratedNamePropertyCustomization::CustomIsResetToDefaultVisible(TSharedPtr<IPropertyHandle> Property) const
{
	const FName CuratedName = GetCuratedName();
	return !CuratedName.IsNone();
}

void IFlowCuratedNamePropertyCustomization::CustomResetToDefault(TSharedPtr<IPropertyHandle> Property)
{
	if (TrySetCuratedNameWithSideEffects(NAME_None))
	{
		RepaintTextListWidget();
	}
}

bool IFlowCuratedNamePropertyCustomization::TrySetCuratedNameWithSideEffects(const FName& NewName)
{
	const FName ExistingName = GetCuratedName();

	if (ExistingName != NewName)
	{
		// Set the new name on the actual struct first
		SetCuratedName(NewName);
	}

	// Ensure the FText representations are up to date

	TSharedPtr<FText> NewText = FindCachedOrCreateText(NewName);

	const bool bIsChanged = (NewText != CachedTextSelected);

	CachedTextSelected = NewText;
	
	InsertAtHeadOfCachedTextList(CachedTextSelected);

	// Set the Name property to the new value
	check(CachedNameHandle.IsValid());
	CachedNameHandle->SetValue(NewName);
	
	return bIsChanged;
}

FText IFlowCuratedNamePropertyCustomization::GetCachedText() const
{
	check(CachedTextSelected.IsValid());

	return *CachedTextSelected.Get();
}

TSharedRef<SWidget> IFlowCuratedNamePropertyCustomization::GenerateTextListWidget(TSharedPtr<FText> InItem)
{
	return
		SNew(STextBlock)
		.Text(*InItem)
		.ColorAndOpacity(FSlateColor::UseForeground())
		.Font(IDetailLayoutBuilder::GetDetailFont());
}

void IFlowCuratedNamePropertyCustomization::OnTextListComboBoxOpening()
{
	check(CachedTextSelected.IsValid());

	// Create a dictionary of Names to their shared FTexts
	//  (to preserve the shared FText objects, if they already exist)
	TMap<FName, TSharedPtr<FText>> MapNameToText;

	const FName CurrentName = GetCuratedName();
	MapNameToText.Add(CurrentName, CachedTextSelected);

	for (TSharedPtr<FText>& Text : CachedTextList)
	{
		(void) MapNameToText.FindOrAdd(FName(Text.Get()->ToString()), Text);
	}

	TArray<FName> CuratedNameOptions = GetCuratedNameOptions();

	// (+2 to reserve space for the Selected and None entry)
	CachedTextList.Empty(CuratedNameOptions.Num() + 2);

	// Populate the current selection at the top of the list
	if (CuratedNameOptions.Contains(CurrentName) || CurrentName.IsNone())
	{
		CachedTextList.Add(CachedTextSelected);
	}

	// Populate the other curated name options
	for (const FName& NameOption : CuratedNameOptions)
	{
		if (!NameOption.IsNone() && NameOption != CurrentName)
		{
			AddToCachedTextList(FindCachedOrCreateText(NameOption));
		}
	}

	// Ensure "None" is in the list (if CurrentName is not None)
	if (!CurrentName.IsNone())
	{
		check(!CachedTextList.Contains(NoneAsText));

		CachedTextList.Add(NoneAsText);
	}
}

void IFlowCuratedNamePropertyCustomization::OnTextSelected(TSharedPtr<FText> NewSelection, ESelectInfo::Type SelectInfo)
{
	// Called when the combo box has selected a new element 

	// Process NewSelection and derive the matching Name
	// (NewSelection can be null)

	FName NewName;

	if (NewSelection.IsValid())
	{
		// Ensure NewSelection is in the CachedTextList
		AddToCachedTextList(NewSelection);

		NewName = FName(NewSelection->ToString());
	}
	else
	{
		NewName = NAME_None;
	}

	if (TrySetCuratedNameWithSideEffects(NewName))
	{
		RepaintTextListWidget();
	}
}

TSharedPtr<FText> IFlowCuratedNamePropertyCustomization::FindCachedOrCreateText(const FName& NewName)
{
	if (NewName.IsNone())
	{
		return NoneAsText;
	}

	const FText NewText = FText::FromName(NewName);

	for (int32 Index = 0; Index < CachedTextList.Num(); ++Index)
	{
		const TSharedPtr<FText>& TextCur = CachedTextList[Index];

		if (TextCur->EqualTo(NewText, ETextComparisonLevel::Default))
		{
			return TextCur;
		}
	}

	TSharedPtr<FText> Result = MakeShareable(new FText(NewText));
	return Result;
}

void IFlowCuratedNamePropertyCustomization::InsertAtHeadOfCachedTextList(TSharedPtr<FText> Text)
{
	CachedTextList.Remove(Text);

	CachedTextList.Insert(Text, 0);
}

void IFlowCuratedNamePropertyCustomization::AddToCachedTextList(TSharedPtr<FText> Text)
{
	CachedTextList.AddUnique(Text);
}

void IFlowCuratedNamePropertyCustomization::RepaintTextListWidget()
{
	if (TextListWidget.IsValid())
	{
		// Prod UDE to refresh the widget to show the new change
		TextListWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}
