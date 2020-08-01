#include "FlowNode_CustomEventCustomization.h"
#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_CustomEvent.h"

#include "DetailWidgetRow.h"
#include "PropertyEditing.h"
#include "UnrealEd.h"

#define LOCTEXT_NAMESPACE "FlowNode_CustomEventCustomization"

void FFlowNode_CustomEventCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	DetailLayout.GetObjectsBeingCustomized(ObjectsBeingEdited);
	GetEventNames();

	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("CustomEvent", LOCTEXT("CustomEventCategory", "Custom Event"));
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
			.OnGenerateWidget(this, &FFlowNode_CustomEventCustomization::GenerateEventWidget)
			.OnSelectionChanged(this, &FFlowNode_CustomEventCustomization::PinSelectionChanged)
			[
				SNew(STextBlock)
					.Text(this, &FFlowNode_CustomEventCustomization::GetSelectedEventText)
			]
		];
}

void FFlowNode_CustomEventCustomization::GetEventNames()
{
	EventNames.Empty();
	EventNames.Emplace(MakeShareable(new FName(NAME_None)));

	if (ObjectsBeingEdited[0].IsValid() && ObjectsBeingEdited[0].Get()->GetOuter())
	{
		const UFlowAsset* FlowAsset = Cast<UFlowAsset>(ObjectsBeingEdited[0].Get()->GetOuter());
		TArray<FName> SortedNames = FlowAsset->GetCustomEvents();

		for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
		{
			if (Node.Value->GetClass()->IsChildOf(UFlowNode_CustomEvent::StaticClass()))
			{
				SortedNames.Remove(Cast<UFlowNode_CustomEvent>(Node.Value)->EventName);
			}
		}

		SortedNames.Sort([](const FName& A, const FName& B) 
		{
			return A.LexicalLess(B);
		});
		
		for (const FName& EventName : SortedNames)
		{
			if (EventName != NAME_None)
			{
				EventNames.Emplace(MakeShareable(new FName(EventName)));
			}
		}
	}
}

TSharedRef<SWidget> FFlowNode_CustomEventCustomization::GenerateEventWidget(const TSharedPtr<FName> Item) const
{
	return SNew(STextBlock).Text(FText::FromName(*Item.Get()));
}

FText FFlowNode_CustomEventCustomization::GetSelectedEventText() const
{
	FText PropertyValue;

	ensure(ObjectsBeingEdited[0].IsValid());
	if (const UFlowNode_CustomEvent* Node = Cast<UFlowNode_CustomEvent>(ObjectsBeingEdited[0].Get()))
	{
		PropertyValue = FText::FromName(Node->EventName);
	}

	return PropertyValue;
}

void FFlowNode_CustomEventCustomization::PinSelectionChanged(const TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo) const
{
	ensure(ObjectsBeingEdited[0].IsValid());
	if (UFlowNode_CustomEvent* Node = Cast<UFlowNode_CustomEvent>(ObjectsBeingEdited[0].Get()))
	{
		Node->EventName = *Item.Get();
	}
}

#undef LOCTEXT_NAMESPACE