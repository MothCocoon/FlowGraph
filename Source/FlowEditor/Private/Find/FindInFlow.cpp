// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Find/FindInFlow.h"
#include "Asset/FlowAssetEditor.h"
#include "Find/SFindInFlowFilterPopup.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "FlowAsset.h"
#include "FlowEditorModule.h"
#include "Graph/FlowGraphEditorSettings.h"
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
#include "Misc/EnumRange.h"
#include "SlotBase.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateColor.h"
#include "Templates/Casts.h"
#include "Types/SlateStructs.h"
#include "UObject/Class.h"
#include "UObject/ObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "FindInFlow"

//////////////////////////////////////////////////////////////////////////
// FFindInFlowResult

FFindInFlowResult::FFindInFlowResult(const FString& InValue, UFlowAsset* InOwningFlowAsset)
	: Value(InValue)
	, OwningFlowAsset(InOwningFlowAsset)
{
}

FFindInFlowResult::FFindInFlowResult(const FString& InValue, TSharedPtr<FFindInFlowResult> InParent, UEdGraphNode* InNode, bool bInIsSubGraphNode, UFlowAsset* InOwningFlowAsset)
	: Value(InValue)
	, GraphNode(InNode)
	, OwningFlowAsset(InOwningFlowAsset)
	, Parent(InParent)
	, bIsSubGraphNode(bInIsSubGraphNode)
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

FReply FFindInFlowResult::OnClick(TWeakPtr<class FFlowAssetEditor> FlowAssetEditorPtr)
{
	if (GraphNode.IsValid())
	{
		if (UEdGraph* Graph = GraphNode->GetGraph())
		{
			if (UFlowAsset* Asset = Cast<UFlowAsset>(Graph->GetOuter()))
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
				if (TSharedPtr<FFlowAssetEditor> Editor = FFlowGraphUtils::GetFlowAssetEditor(Graph))
				{
					Editor->JumpToNode(GraphNode.Get());
				}
			}
		}
	}
	else if (OwningFlowAsset.IsValid())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(OwningFlowAsset.Get());
	}
	return FReply::Handled();
}

FReply FFindInFlowResult::OnDoubleClick() const
{
	if (bIsSubGraphNode && Parent.IsValid())
	{
		if (const UFlowGraphNode* ParentNode = Cast<UFlowGraphNode>(Parent.Pin()->GraphNode.Get()))
		{
			if (UFlowNode_SubGraph* SubGraph = Cast<UFlowNode_SubGraph>(ParentNode->GetFlowNodeBase()))
			{
				if (UObject* Target = SubGraph->GetAssetToEdit())
				{
					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Target);
					if (TSharedPtr<FFlowAssetEditor> Editor = FFlowGraphUtils::GetFlowAssetEditor(GraphNode->GetGraph()))
					{
						Editor->JumpToNode(GraphNode.Get());
					}
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
	return GraphNode.IsValid() ? GraphNode->NodeComment : FString();
}

FString FFindInFlowResult::GetNodeTypeText() const
{
	if (!GraphNode.IsValid())
	{
		return FString();
	}

	if (const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(GraphNode.Get()))
	{
		if (UFlowNodeBase* Base = FlowGraphNode->GetFlowNodeBase())
		{
			return Base->GetClass()->GetDisplayNameText().ToString();
		}
	}

	return GraphNode->GetClass()->GetDisplayNameText().ToString();
}

FText FFindInFlowResult::GetToolTipText() const
{
	FString Tip = GetNodeTypeText() + TEXT("\n") + GetDescriptionText();

	if (!GetCommentText().IsEmpty())
	{
		Tip += TEXT("\n") + GetCommentText();
	}

	if (!MatchedPropertySnippet.IsEmpty())
	{
		Tip += TEXT("\n\nMatched: ") + MatchedPropertySnippet;
	}

	return FText::FromString(Tip);
}

FText FFindInFlowResult::GetMatchedSnippet() const
{
	return FText::FromString(MatchedPropertySnippet);
}

FText FFindInFlowResult::GetMatchedCategoriesText() const
{
	if (MatchedFlags == EFlowSearchFlags::None)
	{
		return FText::GetEmpty();
	}

	TArray<FText> DisplayNames;

	for (EFlowSearchFlags Flag : MakeFlagsRange(EFlowSearchFlags::All))
	{
		if (EnumHasAnyFlags(MatchedFlags, Flag))
		{
			FText DisplayName = UEnum::GetDisplayValueAsText(Flag);
			if (!DisplayName.IsEmpty())
			{
				DisplayNames.Add(DisplayName);
			}
		}
	}

	if (DisplayNames.Num() == 0)
	{
		return FText::GetEmpty();
	}

	return FText::Join(FText::FromString(TEXT(", ")), DisplayNames);
}

//////////////////////////////////////////////////////////////////////////
// SFindInFlow

void SFindInFlow::Construct(const FArguments& InArgs, TSharedPtr<class FFlowAssetEditor> InFlowAssetEditor)
{
	FlowAssetEditorPtr = InFlowAssetEditor;
	SearchResults.Setup();

	// Load INI settings
	const UFlowGraphEditorSettings* Settings = GetDefault<UFlowGraphEditorSettings>();
	if (ensure(Settings))
	{
		MaxSearchDepth = Settings->DefaultMaxSearchDepth;
		SearchFlags = static_cast<EFlowSearchFlags>(Settings->DefaultSearchFlags);
	}

	// Populate scope options
	FLOW_ASSERT_ENUM_MAX(EFlowSearchScope, 3);
	for (EFlowSearchScope Scope : TEnumRange<EFlowSearchScope>())
	{
		if (FlowEnum::IsValidEnumValue(Scope))
		{
			ScopeOptionList.Add(MakeShareable(new EFlowSearchScope(Scope)));
		}
	}
	SelectedScopeOption = ScopeOptionList[0];

	SAssignNew(SearchTextField, SSearchBox)
		.OnTextCommitted(this, &SFindInFlow::OnSearchTextCommitted);

	SAssignNew(SearchButton, SButton)
		.Text(LOCTEXT("SearchButton", "Search"))
		.OnClicked(this, &SFindInFlow::OnSearchButtonClicked);

	SAssignNew(MaxDepthSpinBox, SSpinBox<int32>)
		.MinValue(0)
		.MaxValue(10)
		.Value(MaxSearchDepth)
		.OnValueChanged(this, &SFindInFlow::OnMaxDepthChanged)
		.ToolTipText(LOCTEXT("MaxDepthTooltip", "Maximum recursion depth when searching inside objects"));

	SAssignNew(TreeView, STreeViewType)
		.TreeItemsSource(&SearchResults.ItemsFound)
		.OnGenerateRow(this, &SFindInFlow::OnGenerateRow)
		.OnGetChildren(this, &SFindInFlow::OnGetChildren)
		.OnSelectionChanged(this, &SFindInFlow::OnTreeSelectionChanged)
		.OnMouseButtonDoubleClick(this, &SFindInFlow::OnTreeSelectionDoubleClicked);

	ChildSlot
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SearchTextField.ToSharedRef()
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SearchButton.ToSharedRef()
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("FiltersLabel", "Filters:"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
								.ToolTipText(LOCTEXT("EditFiltersTooltip", "Edit search filters"))
								.OnClicked_Lambda([this]()
									{
										const FFindInFlowApplyDelegate OnSaveAsDefault = FFindInFlowApplyDelegate::CreateLambda([this](EFlowSearchFlags Flags)
											{
												if (UFlowGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UFlowGraphEditorSettings>())
												{
													GraphEditorSettings->DefaultSearchFlags = static_cast<uint32>(Flags);
													GraphEditorSettings->SaveConfig();
												}
											});

										const TSharedRef<SFindInFlowFilterPopup> FilterPopup = SNew(SFindInFlowFilterPopup)
											.OnApply(FFindInFlowApplyDelegate::CreateLambda([this](const EFlowSearchFlags NewSearchFlags)
												{
													SearchFlags = NewSearchFlags;
													InitiateSearch();
												}))
											.OnSaveAsDefault(OnSaveAsDefault)
											.InitialFlags(SearchFlags);

										FSlateApplication::Get().PushMenu(
											AsShared(),
											FWidgetPath(),
											FilterPopup,
											FSlateApplication::Get().GetCursorPos(),
											FPopupTransitionEffect::ContextMenu);

										return FReply::Handled();
									})
								[
									SNew(STextBlock)
										.Text_Lambda([this]()
											{
												int32 ActiveCount = FMath::CountBits(static_cast<uint32>(SearchFlags));
												return FText::Format(LOCTEXT("ActiveFilters", "{0} Active"), FText::AsNumber(ActiveCount));
											})
								]
						]
					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SNew(SComboBox<TSharedPtr<EFlowSearchScope>>)
								.OptionsSource(&ScopeOptionList)
								.OnGenerateWidget(this, &SFindInFlow::GenerateScopeWidget)
								.OnSelectionChanged(this, &SFindInFlow::OnScopeChanged)
								[
									SNew(STextBlock).Text(this, &SFindInFlow::GetCurrentScopeText)
								]
						]
					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("MaxDepthLabel", "Max Depth:"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							MaxDepthSpinBox.ToSharedRef()
						]
				]
			+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							TreeView.ToSharedRef()
						]
				]
		];
}

void SFindInFlow::FocusForUse() const
{
	if (SearchTextField.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(SearchTextField.ToSharedRef());
		SearchTextField->SelectAllText();
	}
}

void SFindInFlow::OnSearchTextChanged(const FText& Text)
{
	SearchValue = Text.ToString();
}

void SFindInFlow::OnSearchTextCommitted(const FText& Text, ETextCommit::Type)
{
	SearchValue = Text.ToString();

	InitiateSearch();
}

FReply SFindInFlow::OnSearchButtonClicked()
{
	InitiateSearch();

	return FReply::Handled();
}

void SFindInFlow::OnScopeChanged(TSharedPtr<EFlowSearchScope> NewSelection, ESelectInfo::Type)
{
	SelectedScopeOption = NewSelection;
	SearchScope = *NewSelection;
}

void SFindInFlow::OnMaxDepthChanged(int32 NewDepth)
{
	MaxSearchDepth = NewDepth;

	// Save to INI
	if (UFlowGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UFlowGraphEditorSettings>())
	{
		GraphEditorSettings->DefaultMaxSearchDepth = NewDepth;
		GraphEditorSettings->SaveConfig();
	}
}

TSharedRef<SWidget> SFindInFlow::GenerateScopeWidget(TSharedPtr<EFlowSearchScope> Item) const
{
	return SNew(STextBlock)
		.Text(UEnum::GetDisplayValueAsText(*Item.Get()));
}

FText SFindInFlow::GetCurrentScopeText() const
{
	return UEnum::GetDisplayValueAsText(*SelectedScopeOption.Get());
}

void SFindInFlow::InitiateSearch()
{
	FFlowEditorModule* FlowEditorModule = &FModuleManager::LoadModuleChecked<FFlowEditorModule>("FlowEditor");
	if (ensure(FlowEditorModule))
	{
		FlowEditorModule->RegisterForAssetChanges();
	}

	SearchResults.Reset();
	HighlightText = FText::FromString(SearchValue);
	TreeView->RequestTreeRefresh();

	if (SearchValue.IsEmpty())
	{
		return;
	}

	TSharedPtr<FFlowAssetEditor> Editor = FlowAssetEditorPtr.Pin();
	if (!Editor.IsValid())
	{
		return;
	}

	UFlowAsset* CurrentAsset = Editor->GetFlowAsset();
	if (!CurrentAsset || !CurrentAsset->GetGraph())
	{
		return;
	}

	FFlowSearchQuery Query;
	Query.SearchText   = SearchValue;
	Query.Flags        = SearchFlags;
	Query.Scope        = SearchScope;
	Query.MaxDepth     = MaxSearchDepth;
	Query.ContextAsset = CurrentAsset;

	TArray<FFlowSearchResultItem> RawResults;
	FFlowSearch::Search(Query, RawResults);

	// Group flat results by asset into per-asset root nodes (same visual layout as before).
	TMap<FSoftObjectPath, FSearchResult> AssetRoots;
	for (const FFlowSearchResultItem& Item : RawResults)
	{
		FSearchResult& AssetRoot = AssetRoots.FindOrAdd(Item.AssetPath);
		if (!AssetRoot.IsValid())
		{
			UFlowAsset* Asset = Cast<UFlowAsset>(Item.AssetPath.TryLoad());
			const FString AssetName = Asset ? Asset->GetName() : Item.AssetPath.GetAssetName();
			AssetRoot = MakeShareable(new FFindInFlowResult(AssetName, Asset));
		}

		FSearchResult ResultItem = MakeResultItem(Item);
		ResultItem->Parent = AssetRoot;
		AssetRoot->Children.Add(ResultItem);
	}

	for (auto& KV : AssetRoots)
	{
		SearchResults.ItemsFound.Add(KV.Value);

		// Auto-expand the current asset's group.
		if (KV.Key == FSoftObjectPath(CurrentAsset))
		{
			TreeView->SetItemExpansion(KV.Value, true);
		}
	}

	if (SearchResults.ItemsFound.IsEmpty())
	{
		FSearchResult NoResults = MakeShareable(new FFindInFlowResult(TEXT("No results found")));
		SearchResults.ItemsFound.Add(NoResults);
	}

	TreeView->RequestTreeRefresh();
}

SFindInFlow::FSearchResult SFindInFlow::MakeResultItem(const FFlowSearchResultItem& Item) const
{
	UFlowAsset* Asset   = Cast<UFlowAsset>(Item.AssetPath.TryLoad());
	UEdGraphNode* EdNode = nullptr;
	if (Asset && Asset->GetGraph())
	{
		for (UEdGraphNode* Node : Asset->GetGraph()->Nodes)
		{
			if (Node && Node->NodeGuid == Item.NodeGuid)
			{
				EdNode = Node;
				break;
			}
		}
	}

	FSearchResult Result = MakeShareable(
		new FFindInFlowResult(Item.NodeTitle, nullptr, EdNode, Item.bIsSubGraphNode, Asset));
	Result->MatchedFlags          = Item.MatchedFlags;
	Result->MatchedPropertySnippet = Item.MatchedSnippet;
	return Result;
}

TSharedRef<ITableRow> SFindInFlow::OnGenerateRow(FSearchResult InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FSearchResult>, OwnerTable)
		.ToolTip(SNew(SToolTip).Text(InItem->GetToolTipText()))
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2, 0)
				[
					InItem->CreateIcon()
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(4, 0)
				[
					SNew(STextBlock)
						.Text(FText::FromString(InItem->Value))
						.HighlightText(HighlightText)
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(4, 0)
				[
					SNew(STextBlock)
						.Text(FText::FromString(InItem->GetNodeTypeText()))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.8f, 1.0f)))
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(4, 0)
				[
					SNew(STextBlock)
						.Text(InItem->GetMatchedCategoriesText())
						.ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)))
				]
		];
}

void SFindInFlow::OnGetChildren(FSearchResult InItem, TArray<FSearchResult>& OutChildren)
{
	OutChildren = InItem->Children;
}

void SFindInFlow::OnTreeSelectionChanged(FSearchResult Item, ESelectInfo::Type)
{
	if (Item.IsValid())
	{
		Item->OnClick(FlowAssetEditorPtr);
	}
}

void SFindInFlow::OnTreeSelectionDoubleClicked(FSearchResult Item)
{
	if (Item.IsValid())
	{
		Item->OnDoubleClick();
	}
}

#undef LOCTEXT_NAMESPACE
