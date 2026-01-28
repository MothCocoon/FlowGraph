// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

// Adapted from SGraphEditorActionMenuAI, changing UAIGraphNode to UEdGraphNode, and using UFlowGraphSchema

#pragma once

#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "GraphEditor.h"
#include "HAL/PlatformMath.h"
#include "Math/Vector2D.h"
#include "Templates/SharedPointer.h"
#include "Types/SlateEnums.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Layout/SBorder.h"

class SEditableTextBox;
class SGraphActionMenu;
class UEdGraphNode;
class UEdGraph;
class UEdGraphPin;
struct FEdGraphSchemaAction;
struct FGraphActionListBuilderBase;

/////////////////////////////////////////////////////////////////////////////////////////////////

class FLOWEDITOR_API SGraphEditorActionMenuFlow : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SGraphEditorActionMenuFlow)
		: _GraphObj(static_cast<UEdGraph*>(nullptr))
		, _GraphNode(nullptr)
		, _NewNodePosition(FVector2f::ZeroVector)
		, _AutoExpandActionMenu(false)
		, _SubNodeFlags(0)
		{
		}

		SLATE_ARGUMENT(UEdGraph*, GraphObj)
		SLATE_ARGUMENT(UEdGraphNode*, GraphNode)
		SLATE_ARGUMENT(FVector2f, NewNodePosition)
		SLATE_ARGUMENT(TArray<UEdGraphPin*>, DraggedFromPins)
		SLATE_ARGUMENT(SGraphEditor::FActionMenuClosed, OnClosedCallback)
		SLATE_ARGUMENT(bool, AutoExpandActionMenu)
		SLATE_ARGUMENT(int32, SubNodeFlags)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	~SGraphEditorActionMenuFlow();

	TSharedRef<SEditableTextBox> GetFilterTextBox();

protected:
	UEdGraph* GraphObj;
	UEdGraphNode* GraphNode;
	TArray<UEdGraphPin*> DraggedFromPins;
	FVector2f NewNodePosition;
	bool AutoExpandActionMenu;
	int32 SubNodeFlags;

	SGraphEditor::FActionMenuClosed OnClosedCallback;
	TSharedPtr<SGraphActionMenu> GraphActionMenu;

	void OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedAction, ESelectInfo::Type InSelectionType);

	/** Callback used to populate all actions list in SGraphActionMenu */
	void CollectAllActions(FGraphActionListBuilderBase& OutAllActions);
};
