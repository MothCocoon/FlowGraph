// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Details/Nodes/FlowNode_CustomEventBaseDetails.h"

#include "FlowAsset.h"
#include "Nodes/Graph/FlowNode_CustomEventBase.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWidget.h"

void FFlowNode_CustomEventBaseDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Subclasses must override this function (and call CustomizeDetailsInternal with the localized text)
	checkNoEntry();
}

void FFlowNode_CustomEventBaseDetails::CustomizeDetailsInternal(IDetailLayoutBuilder& DetailLayout, const FText& CustomRowNameText, const FText& EventNameText)
{
	DetailLayout.GetObjectsBeingCustomized(ObjectsBeingEdited);

	if (const UObject* EditedObject = ObjectsBeingEdited[0].Get())
	{
		const UFlowNode_CustomEventBase* EventNode = CastChecked<UFlowNode_CustomEventBase>(EditedObject);
		CachedEventNameSelected = MakeShared<FName>(EventNode->GetEventName());
	}

	IDetailCategoryBuilder& Category = CreateDetailCategory(DetailLayout);

	Category.AddCustomRow(CustomRowNameText)
	        .NameContent()
			[
				SNew(STextBlock)
				.Text(EventNameText)
			]
			.ValueContent()
			.HAlign(HAlign_Fill)
			[
				SAssignNew(EventTextListWidget, SComboBox<TSharedPtr<FName>>)
								.OptionsSource(&EventNames)
								.OnGenerateWidget(this, &FFlowNode_CustomEventBaseDetails::GenerateEventWidget)
								.OnComboBoxOpening(this, &FFlowNode_CustomEventBaseDetails::OnComboBoxOpening)
								.OnSelectionChanged(this, &FFlowNode_CustomEventBaseDetails::PinSelectionChanged)
								[
									SNew(STextBlock)
									.Text(this, &FFlowNode_CustomEventBaseDetails::GetSelectedEventText)
								]
			];
}

void FFlowNode_CustomEventBaseDetails::OnComboBoxOpening()
{
	RebuildEventNames();
}

void FFlowNode_CustomEventBaseDetails::RebuildEventNames()
{
	EventNames.Empty();

	check(CachedEventNameSelected.IsValid());
	EventNames.Add(CachedEventNameSelected);

	const UObject* EditedObject = ObjectsBeingEdited[0].Get();
	if (EditedObject && EditedObject->GetOuter())
	{
		const UFlowAsset* FlowAsset = CastChecked<UFlowAsset>(EditedObject->GetOuter());
		TArray<FName> SortedNames = BuildEventNames(*FlowAsset);

		if (bExcludeReferencedEvents)
		{
			for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
			{
				if (Node.Value->GetClass()->IsChildOf(UFlowNode_CustomEventBase::StaticClass()))
				{
					SortedNames.Remove(Cast<UFlowNode_CustomEventBase>(Node.Value)->GetEventName());
				}
			}
		}

		SortedNames.Sort([](const FName& A, const FName& B)
		{
			return A.LexicalLess(B);
		});

		for (const FName& EventName : SortedNames)
		{
			const bool bIsCurrentSelection = (EventName == *CachedEventNameSelected);
			if (!EventName.IsNone() && !bIsCurrentSelection)
			{
				EventNames.Add(MakeShared<FName>(EventName));
			}
		}
	}

	if (!IsInEventNames(NAME_None))
	{
		EventNames.Add(MakeShared<FName>(NAME_None));
	}
}

TSharedRef<SWidget> FFlowNode_CustomEventBaseDetails::GenerateEventWidget(const TSharedPtr<FName> Item) const
{
	return SNew(STextBlock)
		.Text(FText::FromName(*Item.Get()));
}

FText FFlowNode_CustomEventBaseDetails::GetSelectedEventText() const
{
	check(CachedEventNameSelected.IsValid());
	return FText::FromName(*CachedEventNameSelected.Get());
}

void FFlowNode_CustomEventBaseDetails::PinSelectionChanged(const TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo)
{
	UObject* EditedObject = ObjectsBeingEdited[0].Get();
	ensure(EditedObject);

	UFlowNode_CustomEventBase* Node = Cast<UFlowNode_CustomEventBase>(EditedObject);
	if (IsValid(Node) && Item)
	{
		const bool bIsChanged = (*CachedEventNameSelected != *Item);

		if (bIsChanged)
		{
			CachedEventNameSelected = Item;

			const FName ItemAsName = *CachedEventNameSelected;
			Node->SetEventName(ItemAsName);

			if (EventTextListWidget.IsValid())
			{
				// Tell UDE to refresh the widget to show the new change
				EventTextListWidget->Invalidate(EInvalidateWidgetReason::Paint);
			}
		}
	}
}

bool FFlowNode_CustomEventBaseDetails::IsInEventNames(const FName& EventName) const
{
	const bool bIsInEventNames = EventNames.ContainsByPredicate([&EventName](const TSharedPtr<FName>& ExistingName)
	{
		return *ExistingName == EventName;
	});

	return bIsInEventNames;
}
