// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Widgets/SGraphEditorActionMenuFlow.h"
#include "Graph/FlowGraphSchema.h"

#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/Platform.h"
#include "HAL/PlatformCrt.h"
#include "Layout/Margin.h"
#include "Misc/Attribute.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SGraphActionMenu.h"
#include "Styling/AppStyle.h"
#include "Templates/Casts.h"
#include "Types/SlateStructs.h"
#include "Widgets/Layout/SBox.h"

SGraphEditorActionMenuFlow::~SGraphEditorActionMenuFlow()
{
	OnClosedCallback.ExecuteIfBound();
}

void SGraphEditorActionMenuFlow::Construct(const FArguments& InArgs)
{
	this->GraphObj = InArgs._GraphObj;
	this->GraphNode = InArgs._GraphNode;
	this->DraggedFromPins = InArgs._DraggedFromPins;
	this->NewNodePosition = InArgs._NewNodePosition;
	this->OnClosedCallback = InArgs._OnClosedCallback;
	this->AutoExpandActionMenu = InArgs._AutoExpandActionMenu;
	this->SubNodeFlags = InArgs._SubNodeFlags;

	// Build the widget layout
	SBorder::Construct(SBorder::FArguments()
        .BorderImage(FAppStyle::GetBrush("Menu.Background"))
        .Padding(5.f)
		[
			// Achieving fixed width by nesting items within a fixed width box.
			SNew(SBox)
			.WidthOverride(400.f)
			[
				SAssignNew(GraphActionMenu, SGraphActionMenu)
				.OnActionSelected(this, &SGraphEditorActionMenuFlow::OnActionSelected)
				.OnCollectAllActions(this, &SGraphEditorActionMenuFlow::CollectAllActions)
				.AutoExpandActionMenu(AutoExpandActionMenu)
			]
		]
	);
}

void SGraphEditorActionMenuFlow::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	// Build up the context object
	FGraphContextMenuBuilder ContextMenuBuilder(GraphObj);
	if (GraphNode != nullptr)
	{
		ContextMenuBuilder.SelectedObjects.Add(GraphNode);
	}
	if (DraggedFromPins.Num() > 0)
	{
		ContextMenuBuilder.FromPin = DraggedFromPins[0];
	}

	// Determine all possible actions
	const UFlowGraphSchema* MySchema = Cast<const UFlowGraphSchema>(GraphObj->GetSchema());
	if (MySchema)
	{
		MySchema->GetGraphNodeContextActions(ContextMenuBuilder, SubNodeFlags);
	}

	// Copy the added options back to the main list
	//@TODO: Avoid this copy
	OutAllActions.Append(ContextMenuBuilder);
}

TSharedRef<SEditableTextBox> SGraphEditorActionMenuFlow::GetFilterTextBox()
{
	return GraphActionMenu->GetFilterTextBox();
}

void SGraphEditorActionMenuFlow::OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedAction, ESelectInfo::Type InSelectionType)
{
	if (InSelectionType == ESelectInfo::OnMouseClick || InSelectionType == ESelectInfo::OnKeyPress || SelectedAction.Num() == 0)
	{
		bool bDoDismissMenus = false;

		if (GraphObj)
		{
			for (int32 ActionIndex = 0; ActionIndex < SelectedAction.Num(); ActionIndex++)
			{
				TSharedPtr<FEdGraphSchemaAction> CurrentAction = SelectedAction[ActionIndex];

				if (CurrentAction.IsValid())
				{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
					CurrentAction->PerformAction(GraphObj, DraggedFromPins, FVector2D(NewNodePosition));
#else
					CurrentAction->PerformAction(GraphObj, DraggedFromPins, NewNodePosition);
#endif
					bDoDismissMenus = true;
				}
			}
		}

		if (bDoDismissMenus)
		{
			FSlateApplication::Get().DismissAllMenus();
		}
	}
}
