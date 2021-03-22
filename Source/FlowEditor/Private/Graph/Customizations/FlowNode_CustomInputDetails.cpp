#include "FlowNode_CustomInputDetails.h"
#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_CustomInput.h"

#include "DetailWidgetRow.h"
#include "PropertyEditing.h"
#include "UnrealEd.h"

#define LOCTEXT_NAMESPACE "FlowNode_CustomInputDetails"

void FFlowNode_CustomInputDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	DetailLayout.GetObjectsBeingCustomized(ObjectsBeingEdited);
	GetEventNames();

	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("CustomInput", LOCTEXT("CustomInputCategory", "Custom Event"));
	Category.AddCustomRow(LOCTEXT("CustomRowName", "Event Name"))
			.NameContent()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("EventName", "Event Name"))
				]
			.ValueContent()
				.HAlign(HAlign_Fill)
				[
					SNew(SComboBox<TSharedPtr<FName>>)
						.OptionsSource(&EventNames)
						.OnGenerateWidget(this, &FFlowNode_CustomInputDetails::GenerateEventWidget)
						.OnSelectionChanged(this, &FFlowNode_CustomInputDetails::PinSelectionChanged)
						[
							SNew(STextBlock)
								.Text(this, &FFlowNode_CustomInputDetails::GetSelectedEventText)
						]
				];
}

void FFlowNode_CustomInputDetails::GetEventNames()
{
	EventNames.Empty();
	EventNames.Emplace(MakeShareable(new FName(NAME_None)));

	if (ObjectsBeingEdited[0].IsValid() && ObjectsBeingEdited[0].Get()->GetOuter())
	{
		const UFlowAsset* FlowAsset = Cast<UFlowAsset>(ObjectsBeingEdited[0].Get()->GetOuter());
		TArray<FName> SortedNames = FlowAsset->GetCustomInputs();

		for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
		{
			if (Node.Value->GetClass()->IsChildOf(UFlowNode_CustomInput::StaticClass()))
			{
				SortedNames.Remove(Cast<UFlowNode_CustomInput>(Node.Value)->EventName);
			}
		}

		SortedNames.Sort([](const FName& A, const FName& B)
		{
			return A.LexicalLess(B);
		});

		for (const FName& EventName : SortedNames)
		{
			if (!EventName.IsNone())
			{
				EventNames.Emplace(MakeShareable(new FName(EventName)));
			}
		}
	}
}

TSharedRef<SWidget> FFlowNode_CustomInputDetails::GenerateEventWidget(const TSharedPtr<FName> Item) const
{
	return SNew(STextBlock).Text(FText::FromName(*Item.Get()));
}

FText FFlowNode_CustomInputDetails::GetSelectedEventText() const
{
	FText PropertyValue;

	ensure(ObjectsBeingEdited[0].IsValid());
	if (const UFlowNode_CustomInput* Node = Cast<UFlowNode_CustomInput>(ObjectsBeingEdited[0].Get()))
	{
		PropertyValue = FText::FromName(Node->EventName);
	}

	return PropertyValue;
}

void FFlowNode_CustomInputDetails::PinSelectionChanged(const TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo) const
{
	ensure(ObjectsBeingEdited[0].IsValid());
	if (UFlowNode_CustomInput* Node = Cast<UFlowNode_CustomInput>(ObjectsBeingEdited[0].Get()))
	{
		Node->EventName = *Item.Get();
	}
}

#undef LOCTEXT_NAMESPACE
