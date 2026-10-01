// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Object.h"
#include "FlowGraphDiff.generated.h"

class UFlowAsset;

/**
 * A single added or removed connection in a diff result. Not the same as FFlowGraphParsedConnection,
 * which carries mutation-grammar fields (delete markers, alias resolution) that have no meaning here.
 */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowGraphDiffConnection
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid SourceNodeGuid;

	UPROPERTY()
	FName SourcePinName;

	UPROPERTY()
	FGuid TargetNodeGuid;

	UPROPERTY()
	FName TargetPinName;

	// True when this edge is an exec connection (stored on the source output pin); false when it
	// is a data connection (stored on the target input pin). Derived from the source pin's type
	// in the parsed document's OutputPins declarations - if the document omits pin type info for
	// the source node, this field defaults to true (exec assumption).
	UPROPERTY()
	bool bIsExecPin = true;
};

/**
 * A single addon-level difference under a node, keyed by AddOnGuid. Recursive: an addon that
 * gained, lost, or changed nested addon-of-addon children carries those deltas in its own
 * Added/Removed/ChangedChildren arrays. An "added" or "removed" addon reports its full subtree
 * as AddedChildren (added case) - the top-level entry plus its descendants describe the whole
 * inserted/deleted tree. A "changed" addon is one present on both sides with a different type, a
 * changed property, or a non-empty child delta.
 */
struct FLOWGRAPHCOURIER_API FFlowGraphDiffAddOn
{
	FGuid AddOnGuid;

	// Resolved class path for the addon. For a "removed" addon this comes from the old document;
	// for "added"/"changed" it comes from the new document. Empty only if the source line omitted it.
	FString AddOnType;

	// Same "OldValue -> NewValue" rendering as FFlowGraphDiffChangedNode::ChangedProperties.
	// For an added addon, every property renders as "<absent> -> Value"; for a removed one, the
	// reverse. For a changed addon, only the keys that actually differ appear.
	TMap<FString, FString> ChangedProperties;

	// Nested addon-of-addon deltas. Populated recursively; empty for a leaf with no child changes.
	TArray<FFlowGraphDiffAddOn> AddedChildren;
	TArray<FFlowGraphDiffAddOn> RemovedChildren;
	TArray<FFlowGraphDiffAddOn> ChangedChildren;
};

/**
 * A node present in both documents with a property, Pos, or addon difference. Carries only the
 * changed property keys (old/new value pairs), whether Pos changed, and any addon-tree deltas -
 * not every property, so a caller doesn't have to diff the full property maps itself to find what
 * actually changed.
 */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowGraphDiffChangedNode
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid NodeGuid;

	// Property keys present in either document with a different (or newly-present/newly-absent)
	// value. Maps to "OldValue -> NewValue" as a single display string, since this is a reporting
	// struct, not something a caller re-parses.
	UPROPERTY()
	TMap<FString, FString> ChangedProperties;

	UPROPERTY()
	bool bPosChanged = false;

	UPROPERTY()
	FIntPoint OldPos = FIntPoint::ZeroValue;

	UPROPERTY()
	FIntPoint NewPos = FIntPoint::ZeroValue;

	// Addons present in the new document but not the old, keyed by AddOnGuid (each carries its own
	// nested subtree as AddedChildren).
	TArray<FFlowGraphDiffAddOn> AddedAddOns;

	// Addons present in the old document but not the new.
	TArray<FFlowGraphDiffAddOn> RemovedAddOns;

	// Addons present in both with a type change, property change, or nested child delta.
	TArray<FFlowGraphDiffAddOn> ChangedAddOns;

	// True when this node carries at least one addon-level difference. Lets the diff decide whether
	// a node with no property/Pos change still belongs in ChangedNodes.
	bool HasAddOnChange() const
	{
		return !AddedAddOns.IsEmpty() || !RemovedAddOns.IsEmpty() || !ChangedAddOns.IsEmpty();
	}
};

/**
 * Structured result of diffing two FlowCourier documents. Provides a headless, text-based
 * comparison that produces serializable output usable from code, MCP ops, or automated tests.
 * Uses UFlowGraphImporter's parse-only functions to turn each document into structured data,
 * then performs a GUID-keyed structural comparison.
 */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowGraphDiffResult
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGuid> AddedNodeGuids;

	UPROPERTY()
	TArray<FGuid> RemovedNodeGuids;

	UPROPERTY()
	TArray<FFlowGraphDiffChangedNode> ChangedNodes;

	UPROPERTY()
	TArray<FFlowGraphDiffConnection> AddedConnections;

	UPROPERTY()
	TArray<FFlowGraphDiffConnection> RemovedConnections;

	bool HasAnyDifference() const
	{
		return !AddedNodeGuids.IsEmpty() || !RemovedNodeGuids.IsEmpty() || !ChangedNodes.IsEmpty()
			|| !AddedConnections.IsEmpty() || !RemovedConnections.IsEmpty();
	}
};

UCLASS()
class FLOWGRAPHCOURIER_API UFlowGraphDiff : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Parses OldText and NewText (each a full Courier v2 JSON document, same acceptance rules as
	 * FFlowCourierConverter) and returns a structural diff between them. Returns false
	 * (with OutErrorMessage set) if either document fails to parse or convert; the diff itself
	 * never fails once parsing succeeds - "no differences" is a valid, successful result (check
	 * OutDiff.HasAnyDifference()).
	 */
	static bool ComputeDiff(
		const FString& OldText,
		const FString& NewText,
		FFlowGraphDiffResult& OutDiff,
		FString& OutErrorMessage);
};
