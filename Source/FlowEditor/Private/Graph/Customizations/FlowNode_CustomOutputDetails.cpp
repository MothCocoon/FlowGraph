#include "FlowNode_CustomOutputDetails.h"
#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_CustomOutput.h"

#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyEditing.h"
#include "UnrealEd.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowNode_CustomOutputDetails"

void FFlowNode_CustomOutputDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	DetailLayout.GetObjectsBeingCustomized(ObjectsBeingEdited);
	GetEventNames();

	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("CustomOutput", LOCTEXT("CustomEventsCategory", "Custom Output"));
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
						.OnGenerateWidget(this, &FFlowNode_CustomOutputDetails::GenerateEventWidget)
						.OnSelectionChanged(this, &FFlowNode_CustomOutputDetails::PinSelectionChanged)
						[
							SNew(STextBlock)
								.Text(this, &FFlowNode_CustomOutputDetails::GetSelectedEventText)
						]
				];
}

void FFlowNode_CustomOutputDetails::GetEventNames()
{
	EventNames.Empty();
	EventNames.Emplace(MakeShareable(new FName(NAME_None)));

	if (ObjectsBeingEdited[0].IsValid() && ObjectsBeingEdited[0].Get()->GetOuter())
	{
		const UFlowAsset* FlowAsset = Cast<UFlowAsset>(ObjectsBeingEdited[0].Get()->GetOuter());
		TArray<FName> SortedNames = FlowAsset->GetCustomOutputs();

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

TSharedRef<SWidget> FFlowNode_CustomOutputDetails::GenerateEventWidget(const TSharedPtr<FName> Item) const
{
	return SNew(STextBlock).Text(FText::FromName(*Item.Get()));
}

FText FFlowNode_CustomOutputDetails::GetSelectedEventText() const
{
	FText PropertyValue;

	ensure(ObjectsBeingEdited[0].IsValid());
	if (const UFlowNode_CustomOutput* Node = Cast<UFlowNode_CustomOutput>(ObjectsBeingEdited[0].Get()))
	{
		PropertyValue = FText::FromName(Node->EventName);
	}

	return PropertyValue;
}

void FFlowNode_CustomOutputDetails::PinSelectionChanged(const TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo) const
{
	ensure(ObjectsBeingEdited[0].IsValid());
	if (UFlowNode_CustomOutput* Node = Cast<UFlowNode_CustomOutput>(ObjectsBeingEdited[0].Get()))
	{
		Node->EventName = *Item.Get();
	}
}

#undef LOCTEXT_NAMESPACE
