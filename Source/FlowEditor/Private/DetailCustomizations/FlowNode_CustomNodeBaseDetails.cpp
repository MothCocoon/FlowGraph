// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "DetailCustomizations/FlowNode_CustomNodeBaseDetails.h"
#include "FlowAsset.h"
#include "Nodes/Route/FlowNode_CustomNodeBase.h"

#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyEditing.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWidget.h"


void FFlowNode_CustomNodeBaseDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Subclasses must override this function (and call CustomizeDetailsInternal with the localized text)
	checkNoEntry();
}

void FFlowNode_CustomNodeBaseDetails::CustomizeDetailsInternal(IDetailLayoutBuilder& DetailLayout, const FText& CustomRowNameText, const FText& EventNameText)
{
	DetailLayout.GetObjectsBeingCustomized(ObjectsBeingEdited);

	if (ObjectsBeingEdited[0].IsValid())
	{
		UFlowNode_CustomNodeBase* FlowNodeBase = CastChecked<UFlowNode_CustomNodeBase>(ObjectsBeingEdited[0]);
		CachedEventNameSelected = MakeShared<FName>(FlowNodeBase->GetEventName());
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
					.OnGenerateWidget(this, &FFlowNode_CustomNodeBaseDetails::GenerateEventWidget)
					.OnComboBoxOpening(this, &FFlowNode_CustomNodeBaseDetails::OnComboBoxOpening)
					.OnSelectionChanged(this, &FFlowNode_CustomNodeBaseDetails::PinSelectionChanged)
					[
						SNew(STextBlock)
							.Text(this, &FFlowNode_CustomNodeBaseDetails::GetSelectedEventText)
					]
			];
}

void FFlowNode_CustomNodeBaseDetails::OnComboBoxOpening()
{
	RebuildEventNames();
}

void FFlowNode_CustomNodeBaseDetails::RebuildEventNames()
{
	EventNames.Empty();

	check(CachedEventNameSelected.IsValid());
	EventNames.Add(CachedEventNameSelected);

	if (ObjectsBeingEdited[0].IsValid() && ObjectsBeingEdited[0].Get()->GetOuter())
	{
		const UFlowAsset* FlowAsset = CastChecked<UFlowAsset>(ObjectsBeingEdited[0].Get()->GetOuter());
		TArray<FName> SortedNames = BuildEventNames(*FlowAsset);

		if (bExcludeReferencedEvents)
		{
			for (const TPair<FGuid, UFlowNode*>& Node : FlowAsset->GetNodes())
			{
				if (Node.Value->GetClass()->IsChildOf(UFlowNode_CustomNodeBase::StaticClass()))
				{
					SortedNames.Remove(Cast<UFlowNode_CustomNodeBase>(Node.Value)->GetEventName());
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

bool FFlowNode_CustomNodeBaseDetails::IsInEventNames(const FName& EventName) const
{
	const bool bIsInEventNames =
		EventNames.ContainsByPredicate([&EventName](const TSharedPtr<FName>& ExistingName)
			{
				return *ExistingName == EventName;
			});

	return bIsInEventNames;
}

TSharedRef<SWidget> FFlowNode_CustomNodeBaseDetails::GenerateEventWidget(const TSharedPtr<FName> Item) const
{
	return
		SNew(STextBlock)
		.Text(FText::FromName(*Item.Get()));
}

FText FFlowNode_CustomNodeBaseDetails::GetSelectedEventText() const
{
	check(CachedEventNameSelected.IsValid());

	return FText::FromName(*CachedEventNameSelected.Get());
}

void FFlowNode_CustomNodeBaseDetails::PinSelectionChanged(const TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo)
{
	ensure(ObjectsBeingEdited[0].IsValid());

	UFlowNode_CustomNodeBase* Node = Cast<UFlowNode_CustomNodeBase>(ObjectsBeingEdited[0].Get());
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
