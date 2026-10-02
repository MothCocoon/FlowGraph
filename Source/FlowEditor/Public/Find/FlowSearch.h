// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/SoftObjectPath.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Misc/Guid.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"

#include "Find/FindInFlowEnums.h"

class UFlowAsset;
class UEdGraphNode;
class UStruct;

/**
 * A single node hit returned by FFlowSearch::Search.
 * Contains enough information for an agent or test to identify and act on the result
 * without touching any Slate or editor-UI types.
 */
struct FLOWEDITOR_API FFlowSearchResultItem
{
	// Soft path to the asset that owns this node.
	FSoftObjectPath AssetPath;

	// Runtime GUID of the matched node. Invalid when the result is an asset-level placeholder.
	FGuid NodeGuid;

	// Display title of the node (UEdGraphNode::GetNodeTitle(ListView), falls back to class name).
	FString NodeTitle;

	// C++ class name of the underlying UFlowNodeBase (GetClass()->GetName()).
	FString NodeTypeName;

	// Which search flag categories actually produced a hit for this node.
	EFlowSearchFlags MatchedFlags = EFlowSearchFlags::None;

	// A representative "Key: Value" snippet from the first matched property, for agent display.
	FString MatchedSnippet;

	// True when this result was found inside a subgraph (Depth > 0 during recursion).
	bool bIsSubGraphNode = false;

	// When bIsSubGraphNode is true: the asset that contains the FlowNode_SubGraph entry point.
	FSoftObjectPath SubgraphOwnerAssetPath;
};

/**
 * Headless search query. Mirrors the inputs the SFindInFlow widget exposes.
 * ContextAsset is required for ThisAssetOnly and AllOfThisType scopes; ignored for AllFlowAssets.
 */
struct FLOWEDITOR_API FFlowSearchQuery
{
	// Whitespace-delimited tokens; all tokens must match (AND semantics, case-insensitive).
	FString SearchText;

	// Which node fields to search.
	EFlowSearchFlags Flags = EFlowSearchFlags::DefaultSearchFlags;

	// How broadly to search.
	EFlowSearchScope Scope = EFlowSearchScope::ThisAssetOnly;

	// Max recursion depth into inline UObject properties (default matches widget default of 3).
	int32 MaxDepth = 3;

	// Required for ThisAssetOnly / AllOfThisType. Ignored for AllFlowAssets.
	TWeakObjectPtr<UFlowAsset> ContextAsset;
};

/**
 * Shared cache of per-node category-string maps (flag -> set of strings).
 * Built lazily by FFlowSearch::Search and invalidated when a FlowAsset changes.
 * Lives here (service layer) so both FFlowSearch and SFindInFlow share one cache
 * without SFindInFlow needing to know about the service internals.
 */
struct FLOWEDITOR_API FFindInFlowCache
{
	/* Removes all cached data for the changed flow asset. */
	static void OnFlowAssetChanged(UFlowAsset& ChangedFlowAsset);

	/* Category-string map keyed by editor graph node pointer. */
	static TMap<TWeakObjectPtr<UEdGraphNode>, TMap<EFlowSearchFlags, TSet<FString>>> CategoryStringCache;
};

/**
 * Stateless, headless search service. Owns the full search algorithm previously embedded in
 * SFindInFlow. No Slate, no FFlowAssetEditor coupling, no INI reads - all configuration is
 * passed in via FFlowSearchQuery.
 *
 * SFindInFlow wraps this with UI state and result-tree presentation.
 * FlowGraphCourier's UFlowMCPToolset wraps it as the SearchFlowAssets MCP op.
 * FlowSearch.spec.cpp tests it directly without an editor session.
 */
class FLOWEDITOR_API FFlowSearch
{
public:
	/**
	 * Run a search and append hits to OutResults.
	 * Returns true if at least one result was appended.
	 * Empty SearchText returns false immediately (no results).
	 */
	static bool Search(const FFlowSearchQuery& Query, TArray<FFlowSearchResultItem>& OutResults);

private:
	// Per-search traversal state. Passed by reference through the recursion so the service
	// remains stateless at the class level.
	struct FSearchContext
	{
		TArray<FString> Tokens;         // Upper-cased AND tokens
		EFlowSearchFlags Flags;
		int32 MaxDepth;
		TSet<UFlowAsset*> VisitedAssets; // Cycle guard for subgraph recursion
	};

	static bool ProcessAsset(
		UFlowAsset* Asset,
		FSearchContext& Ctx,
		bool bIsSubGraphNode,
		const FSoftObjectPath& SubgraphOwnerPath,
		TArray<FFlowSearchResultItem>& OutResults);

	static bool RecurseIntoSubgraphsIfEnabled(
		UEdGraphNode* EdNode,
		const FSoftObjectPath& OwnerAssetPath,
		FSearchContext& Ctx,
		TArray<FFlowSearchResultItem>& OutResults);

	// Builds a per-node category->strings map. Result is cached keyed by EdGraphNode pointer
	// (same cache SFindInFlow used) to avoid rebuilding on repeated searches.
	static const TMap<EFlowSearchFlags, TSet<FString>>* BuildCategoryStrings(
		UEdGraphNode* EdNode,
		FSearchContext& Ctx);

	static void UpdateCategoryStringsForEdGraphNode(
		const UEdGraphNode& EdGraphNode,
		const FSearchContext& Ctx,
		TMap<EFlowSearchFlags, TSet<FString>>& OutMap);

	static void UpdateCategoryStringsForFlowNodeBase(
		const class UFlowNodeBase& FlowNodeBase,
		const FSearchContext& Ctx,
		int32 Depth,
		TMap<EFlowSearchFlags, TSet<FString>>& OutMap);

	static void AppendPropertyValues(
		const void* Container,
		const UStruct* Struct,
		const UObject* ParentObject,
		const FSearchContext& Ctx,
		int32 Depth,
		TMap<EFlowSearchFlags, TSet<FString>>& OutMap);

	static bool StringMatchesTokens(const TArray<FString>& Tokens, const FString& Str);
	static bool StringSetMatchesTokens(const TArray<FString>& Tokens, const TSet<FString>& StringSet);

	// Returns the first "Key: Value" snippet from a matched property, or empty string.
	static FString BuildMatchedSnippet(const TMap<EFlowSearchFlags, TSet<FString>>& CategoryStrings,
	                                   EFlowSearchFlags MatchedFlags,
	                                   const TArray<FString>& Tokens);
};
