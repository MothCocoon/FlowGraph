// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/Array.h"
#include "Containers/BitArray.h"
#include "Containers/Set.h"
#include "Containers/SparseArray.h"
#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "HAL/PlatformCrt.h"
#include "Input/Reply.h"
#include "Internationalization/Text.h"
#include "Misc/Optional.h"
#include "Templates/SharedPointer.h"
#include "Templates/TypeHash.h"
#include "Templates/UnrealTemplate.h"
#include "Types/SlateEnums.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STreeView.h"

#include "Types/FlowEnumUtils.h"
#include "FindInFlowEnums.h"

class ITableRow;
class SWidget;
class UFlowGraphNode;
class UEdGraphNode;
class UFlowAsset;
class UFlowNodeBase;

/** Item that matched the search results */
class FFindInFlowResult
{
public:
	/** Create a root (or only text) result */
	FFindInFlowResult(const FString& InValue, UFlowAsset* InOwningFlowAsset = nullptr);

	/** Create a flow node result */
	FFindInFlowResult(const FString& InValue, TSharedPtr<FFindInFlowResult> InParent, UEdGraphNode* InNode, bool bInIsSubGraphNode = false, UFlowAsset* InOwningFlowAsset = nullptr);

	/** Called when user clicks on the search item */
	FReply OnClick(TWeakPtr<class FFlowAssetEditor> FlowAssetEditorPtr);

	/** Called when user double clicks on the search item */
	FReply OnDoubleClick() const;

	/** Create an icon to represent the result */
	TSharedRef<SWidget> CreateIcon() const;

	/** Gets the description on flow node if any */
	FString GetDescriptionText() const;

	/** Gets the comment on this node if any */
	FString GetCommentText() const;

	/** Gets the node type */
	FString GetNodeTypeText() const;

	/** Gets the node tool tip */
	FText GetToolTipText() const;

	/** Returns a snippet of the matched property/value for tooltip */
	FText GetMatchedSnippet() const;

	/** Human-readable list of categories this result matched in */
	FText GetMatchedCategoriesText() const;

	/** Any children listed under this flow node (decorators, services, addons, subnodes) */
	TArray< TSharedPtr<FFindInFlowResult> > Children;

	/** The string value for this result */
	FString Value;

	/** Stores a snippet of the matched property/value (e.g. "Damage:50") */
	FString MatchedPropertySnippet;

	/** Which search categories actually produced a hit for this item */
	EFlowSearchFlags MatchedFlags = EFlowSearchFlags::None;

	/** The graph node that this search result refers to */
	TWeakObjectPtr<UEdGraphNode> GraphNode;

	/** The owning flow asset for this result */
	TWeakObjectPtr<UFlowAsset> OwningFlowAsset;

	/** Search result parent */
	TWeakPtr<FFindInFlowResult> Parent;

	/** Whether this item is a subgraph node */
	bool bIsSubGraphNode = false;
};

struct FFindInFlowCache
{
	/** Removes all cached data for the changed flow asset */
	static void OnFlowAssetChanged(UFlowAsset& ChangedFlowAsset);

	/** Cache searchable strings per node (for repeat searches) */
	static TMap<TWeakObjectPtr<UEdGraphNode>, TMap<EFlowSearchFlags, TSet<FString>>> CategoryStringCache;
};

struct FFindInFlowAllResults
{
	typedef TSharedPtr<FFindInFlowResult> FSearchResult;

	/** we need to keep a handle on the root result, because it won't show up in the tree */
	FSearchResult RootSearchResult;

	/** This buffer stores the currently displayed results */
	TArray<FSearchResult> ItemsFound;

	/** Visited assets to prevent cycles in subgraph recursion */
	TSet<UFlowAsset*> VisitedAssets;

	void Setup()
	{
		RootSearchResult = MakeShareable(new FFindInFlowResult(TEXT("Root")));
	}

	void Reset()
	{
		ItemsFound.Empty();
		RootSearchResult->Children.Empty();
		VisitedAssets.Empty();
	}
};

/** Widget for searching for (Flow nodes) across focused FlowNodes */
class SFindInFlow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFindInFlow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<class FFlowAssetEditor> InFlowAssetEditor);

	/** Focuses this widget's search box */
	void FocusForUse() const;

protected:

	typedef TSharedPtr<FFindInFlowResult> FSearchResult;
	typedef STreeView<FSearchResult> STreeViewType;

	/** Called when user changes the text they are searching for */
	void OnSearchTextChanged(const FText& Text);

	/** Called when user commits text */
	void OnSearchTextCommitted(const FText& Text, ETextCommit::Type CommitType);

	/** Called when search button is clicked */
	FReply OnSearchButtonClicked();

	/** Get the children of a row */
	void OnGetChildren(FSearchResult InItem, TArray<FSearchResult>& OutChildren);

	/** Called when user clicks on a new result */
	void OnTreeSelectionChanged(FSearchResult Item, ESelectInfo::Type SelectInfo);

	/* Called when user double clicks on a new result */
	void OnTreeSelectionDoubleClicked(FSearchResult Item);

	/** Called when scope selection changed */
	void OnScopeChanged(TSharedPtr<EFlowSearchScope> NewSelection, ESelectInfo::Type SelectInfo);

	/** Called when max depth changed */
	void OnMaxDepthChanged(int32 NewDepth);

	/** Called when a new row is being generated */
	TSharedRef<ITableRow> OnGenerateRow(FSearchResult InItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** Begins the search based on the SearchValue */
	void InitiateSearch();

	/** Build searchable string from node and its FlowNodeBase + AddOns */
	const TMap<EFlowSearchFlags, TSet<FString>>* BuildCategoryStrings(UEdGraphNode* Node, int32 Depth) const;

	/** Determines if a string matches the search tokens */
	static bool StringMatchesSearchTokens(const TArray<FString>& Tokens, const FString& ComparisonString);
	static bool StringSetMatchesSearchTokens(const TArray<FString>& Tokens, const TSet<FString>& StringSet);

	/** Generate widget for scope combo */
	TSharedRef<SWidget> GenerateScopeWidget(TSharedPtr<EFlowSearchScope> Item) const;

	/** Get current scope display text */
	FText GetCurrentScopeText() const;

	bool ProcessAsset(UFlowAsset* Asset, FSearchResult ParentResult, const TArray<FString>& Tokens, int32 Depth);

	bool RecurseIntoSubgraphsIfEnabled(UEdGraphNode* EdNode, FSearchResult ParentResult, const TArray<FString>& Tokens, int32 Depth);

	void UpdateSearchFlagToStringMapForEdGraphNode(const UEdGraphNode& EdGraphNode, TMap<EFlowSearchFlags, TSet<FString>>& SearchFlagToStringMap, int32 Depth) const;
	void UpdateSearchFlagToStringMapForFlowNodeBase(const UFlowNodeBase& FlowNodeBase, TMap<EFlowSearchFlags, TSet<FString>>& SearchFlagToStringMap, int32 Depth) const;
	void AppendPropertyValues(const void* Container, const UStruct* Struct, const UObject* ParentObject, TMap<EFlowSearchFlags, TSet<FString>>& SearchFlagToStringMap, int32 Depth) const;

protected:
	/** Pointer back to the flow editor that owns us */
	TWeakPtr<class FFlowAssetEditor> FlowAssetEditorPtr;

	/** The tree view displays the results */
	TSharedPtr<STreeViewType> TreeView;

	/** The search text box */
	TSharedPtr<class SSearchBox> SearchTextField;

	/** The search button */
	TSharedPtr<SButton> SearchButton;

	/** Struct with all of the search results */
	FFindInFlowAllResults SearchResults;

	/** Repeat Search Caching */
	FFindInFlowCache SearchCache;

	/** The string to highlight in the results */
	FText HighlightText;

	/** The string to search for */
	FString SearchValue;

	/** Search configuration */
	EFlowSearchFlags SearchFlags = EFlowSearchFlags::DefaultSearchFlags;

	TSharedPtr<SSpinBox<int32>> MaxDepthSpinBox;
	int32 MaxSearchDepth = 3;

	/** Scope selection */
	TArray<TSharedPtr<EFlowSearchScope>> ScopeOptionList;
	TSharedPtr<EFlowSearchScope> SelectedScopeOption;
	EFlowSearchScope SearchScope = EFlowSearchScope::ThisAssetOnly;
};