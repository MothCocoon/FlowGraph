// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Find/FindInFlow.h"
#include "Asset/FlowAssetEditor.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Views/ITypedTableView.h"
#include "GraphEditor.h"
#include "HAL/PlatformMath.h"
#include "Input/Events.h"
#include "Internationalization/Internationalization.h"
#include "Layout/Children.h"
#include "Layout/WidgetPath.h"
#include "Math/Color.h"
#include "Misc/Attribute.h"
#include "SlotBase.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateColor.h"
#include "Templates/Casts.h"
#include "Types/SlateStructs.h"
#include "UObject/Class.h"
#include "UObject/ObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

class ITableRow;
class SWidget;
struct FSlateBrush;

#define LOCTEXT_NAMESPACE "FindInFlow"

//////////////////////////////////////////////////////////////////////////
// FFindInFlowResult

FFindInFlowResult::FFindInFlowResult(const FString& InValue) 
	: Value(InValue), GraphNode(nullptr)
{
}

FFindInFlowResult::FFindInFlowResult(const FString& InValue, TSharedPtr<FFindInFlowResult>& InParent, UEdGraphNode* InNode, bool bInIsSubGraphNode)
	: Value(InValue), GraphNode(InNode), Parent(InParent), bIsSubGraphNode(bInIsSubGraphNode)
{
}

TSharedRef<SWidget> FFindInFlowResult::CreateIcon() const
{
	const FSlateColor IconColor = FSlateColor::UseForeground();
	const FSlateBrush* Brush = FAppStyle::GetBrush(TEXT("GraphEditor.FIB_Event"));

	return SNew(SImage)
		.Image(Brush)
		.ColorAndOpacity(IconColor);
}

FReply FFindInFlowResult::OnClick(TWeakPtr<class FFlowAssetEditor> FlowAssetEditorPtr, TSharedPtr<FFindInFlowResult> Root)
{
	if (FlowAssetEditorPtr.IsValid() && GraphNode.IsValid())
	{
		if (Parent.IsValid() && !bIsSubGraphNode)
		{
			FlowAssetEditorPtr.Pin()->JumpToNode(GraphNode.Get());
		}
		else
		{
			FlowAssetEditorPtr.Pin()->JumpToNode(Parent.Pin()->GraphNode.Get());
		}
	}

	return FReply::Handled();
}

FReply FFindInFlowResult::OnDoubleClick(TSharedPtr<FFindInFlowResult> Root) const
{
	if (!Parent.IsValid() || !bIsSubGraphNode)
	{
		return FReply::Handled();
	}
	const UFlowGraphNode* ParentGraphNode = Cast<UFlowGraphNode>(Parent.Pin()->GraphNode);
	if (!ParentGraphNode || !ParentGraphNode->GetFlowNodeBase())
	{
		return FReply::Handled();
	}

	if (UFlowNode* FlowNode = Cast<UFlowNode>(ParentGraphNode->GetFlowNodeBase()))
	{
		if (UObject* AssetToEdit = FlowNode->GetAssetToEdit())
		{
			UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
			if (AssetEditorSubsystem->OpenEditorForAsset(AssetToEdit))
			{
				if (const TSharedPtr<FFlowAssetEditor> FlowAssetEditor = FFlowGraphUtils::GetFlowAssetEditor(GraphNode->GetGraph()))
				{
					FlowAssetEditor->JumpToNode(GraphNode.Get());
				}
			}
		}
	}
	
	return FReply::Handled();
}

FString FFindInFlowResult::GetDescriptionText() const
{
	if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(GraphNode.Get()))
	{
		return FlowGraphNode->GetNodeDescription();
	}

	return FString();
}

FString FFindInFlowResult::GetCommentText() const
{
	if (GraphNode.IsValid())
	{
		return GraphNode->NodeComment;
	}

	return FString();
}

FString FFindInFlowResult::GetNodeTypeText() const
{
	if (GraphNode.IsValid())
	{
		FString NodeClassName;
		const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(GraphNode.Get());
		if (FlowGraphNode && FlowGraphNode->GetFlowNodeBase())
		{
			NodeClassName = FlowGraphNode->GetFlowNodeBase()->GetClass()->GetName();
		}
		else
		{
			NodeClassName = GraphNode->GetClass()->GetName();
		}
		const int32 Pos = NodeClassName.Find("_");
		if (Pos == INDEX_NONE)
		{
			return NodeClassName;
		}
		else
		{
			return NodeClassName.RightChop(Pos + 1);
		}
	}

	return FString();
}

FText FFindInFlowResult::GetToolTipText() const
{
	FString ToolTipStr = TEXT("Click to focus on nodes.");
	if (bIsSubGraphNode)
	{
		ToolTipStr += TEXT("\nDouble click to focus on subgraph nodes");
	}
	return FText::FromString(ToolTipStr);
}

//////////////////////////////////////////////////////////////////////////
// SFindInFlow

void SFindInFlow::Construct( const FArguments& InArgs, TSharedPtr<FFlowAssetEditor> InFlowAssetEditor)
{
	FlowAssetEditorPtr = InFlowAssetEditor;

	this->ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SAssignNew(SearchTextField, SSearchBox)
					.HintText(LOCTEXT("FlowEditorSearchHint", "Enter text to find nodes..."))
					.OnTextChanged(this, &SFindInFlow::OnSearchTextChanged)
					.OnTextCommitted(this, &SFindInFlow::OnSearchTextCommitted)
				]
				+SHorizontalBox::Slot()
				.Padding(10,0,5,0)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FlowEditorSubGraphSearchText", "Find In SubGraph "))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged(this, &SFindInFlow::OnFindInSubGraphStateChanged)
					.ToolTipText(LOCTEXT("FlowEditorSubGraphSearchHint", "Checkin means search also in sub graph."))
				]
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("Menu.Background"))
				[
					SAssignNew(TreeView, STreeViewType)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
					.ItemHeight(24)
#endif
					.TreeItemsSource(&ItemsFound)
					.OnGenerateRow(this, &SFindInFlow::OnGenerateRow)
					.OnGetChildren(this, &SFindInFlow::OnGetChildren)
					.OnSelectionChanged(this, &SFindInFlow::OnTreeSelectionChanged)
					.OnMouseButtonDoubleClick(this, &SFindInFlow::OnTreeSelectionDoubleClicked)
					.SelectionMode(ESelectionMode::Multi)
				]
			]
		];
}

void SFindInFlow::FocusForUse() const
{
	// NOTE: Careful, GeneratePathToWidget can be reentrant in that it can call visibility delegates and such
	FWidgetPath FilterTextBoxWidgetPath;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked(SearchTextField.ToSharedRef(), FilterTextBoxWidgetPath);

	// Set keyboard focus directly
	FSlateApplication::Get().SetKeyboardFocus(FilterTextBoxWidgetPath, EFocusCause::SetDirectly);
}

void SFindInFlow::OnSearchTextChanged(const FText& Text)
{
	SearchValue = Text.ToString();
	
	InitiateSearch();
}

void SFindInFlow::OnSearchTextCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	OnSearchTextChanged(Text);
}

void SFindInFlow::InitiateSearch()
{
	TArray<FString> Tokens;
	SearchValue.ParseIntoArray(Tokens, TEXT(" "), true);

	for (auto It(ItemsFound.CreateIterator()); It; ++It)
	{
		TreeView->SetItemExpansion(*It, false);
	}
	ItemsFound.Empty();
	if (Tokens.Num() > 0)
	{
		HighlightText = FText::FromString(SearchValue);
		MatchTokens(Tokens);
	}

	// Insert a fake result to inform user if none found
	if (ItemsFound.Num() == 0)
	{
		ItemsFound.Add(MakeShared<FFindInFlowResult>(LOCTEXT("FlowEditorSearchNoResults", "No Results found").ToString()));
	}

	TreeView->RequestTreeRefresh();

	for (auto It(ItemsFound.CreateIterator()); It; ++It)
	{
		TreeView->SetItemExpansion(*It, true);
	}
}

void SFindInFlow::MatchTokens(const TArray<FString>& Tokens)
{
	RootSearchResult.Reset();
	
	const UEdGraph* Graph = nullptr;
	const TSharedPtr<SFlowGraphEditor> FocusedGraphEditor = FlowAssetEditorPtr.Pin()->GetFlowGraph();
	if (FocusedGraphEditor.IsValid())
	{
		Graph = FocusedGraphEditor->GetCurrentGraph();
	}

	if (Graph == nullptr)
	{
		return;
	}
	
	RootSearchResult = MakeShared<FFindInFlowResult>(FString("FlowEditorRoot"));

	for (auto It(Graph->Nodes.CreateConstIterator()); It; ++It)
	{
		UEdGraphNode* Node = *It;
		
		const FString NodeName = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
		FSearchResult NodeResult(new FFindInFlowResult(NodeName, RootSearchResult, Node));
		FString NodeSearchString = NodeName + Node->GetClass()->GetName() + Node->NodeComment;

		if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node))
		{
			FString NodeDescription = FlowGraphNode->GetNodeDescription();
			NodeSearchString += NodeDescription;
			
			UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(FlowGraphNode->GetFlowNodeBase());
			if (bFindInSubGraph && SubGraphNode)
			{
				if (const UFlowAsset* FlowAsset = Cast<UFlowAsset>(SubGraphNode->GetAssetToEdit()); FlowAsset && FlowAsset->GetGraph())
				{
					for (auto ChildIt(FlowAsset->GetGraph()->Nodes.CreateConstIterator()); ChildIt; ++ChildIt)
					{
						MatchTokensInChild(Tokens, *ChildIt, NodeResult);
					}
				}
			}
		}

		NodeSearchString = NodeSearchString.Replace(TEXT(" "), TEXT(""));
		const bool bNodeMatchesSearch = StringMatchesSearchTokens(Tokens, NodeSearchString);
		
		if ((NodeResult->Children.Num() > 0) || bNodeMatchesSearch)
		{
			ItemsFound.Add(NodeResult);
		}
	}
}

void SFindInFlow::MatchTokensInChild(const TArray<FString>& Tokens, UEdGraphNode* Child, FSearchResult ParentNode)
{
	if (Child == nullptr)
	{
		return;
	}

	const FString ChildName = Child->GetNodeTitle(ENodeTitleType::ListView).ToString();
	FString ChildSearchString = ChildName + Child->GetClass()->GetName() + Child->NodeComment;
	if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Child))
	{
		FString NodeDescription = FlowGraphNode->GetNodeDescription();
		ChildSearchString += NodeDescription;
	}
	ChildSearchString = ChildSearchString.Replace(TEXT(" "), TEXT(""));
	if (StringMatchesSearchTokens(Tokens, ChildSearchString))
	{
		const FSearchResult DecoratorResult(new FFindInFlowResult(ChildName, ParentNode, Child, true));
		ParentNode->Children.Add(DecoratorResult);
	}
}

TSharedRef<ITableRow> SFindInFlow::OnGenerateRow( FSearchResult InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(STableRow< TSharedPtr<FFindInFlowResult> >, OwnerTable)
		.ToolTip(SNew(SToolTip).Text(InItem->GetToolTipText()))
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SBox)
				.MinDesiredWidth(300)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						InItem->CreateIcon()
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(InItem->Value))
						.HighlightText(HighlightText)
					]
				]
			]
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->GetDescriptionText()))
				.HighlightText(HighlightText)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->GetNodeTypeText()))
				.HighlightText(HighlightText)
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->GetCommentText()))
				.ColorAndOpacity(FLinearColor::Yellow)
				.HighlightText(HighlightText)
			]
		];
}

void SFindInFlow::OnGetChildren(FSearchResult InItem, TArray< FSearchResult >& OutChildren)
{
	OutChildren += InItem->Children;
}

void SFindInFlow::OnTreeSelectionChanged(FSearchResult Item , ESelectInfo::Type)
{
	if (Item.IsValid())
	{
		Item->OnClick(FlowAssetEditorPtr, RootSearchResult);
	}
}

void SFindInFlow::OnTreeSelectionDoubleClicked(FSearchResult Item)
{
	if (Item.IsValid())
	{
		Item->OnDoubleClick(RootSearchResult);
	}
}

void SFindInFlow::OnFindInSubGraphStateChanged(ECheckBoxState CheckBoxState)
{
	bFindInSubGraph = CheckBoxState == ECheckBoxState::Checked;
	InitiateSearch();
}

bool SFindInFlow::StringMatchesSearchTokens(const TArray<FString>& Tokens, const FString& ComparisonString)
{
	bool bFoundAllTokens = true;

	//search the entry for each token, it must have all of them to pass
	for (auto TokItr(Tokens.CreateConstIterator()); TokItr; ++TokItr)
	{
		const FString& Token = *TokItr;
		if (!ComparisonString.Contains(Token))
		{
			bFoundAllTokens = false;
			break;
		}
	}
	return bFoundAllTokens;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
