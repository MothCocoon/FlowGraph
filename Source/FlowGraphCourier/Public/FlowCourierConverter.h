// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Object.h"
#include "FlowCourierDocument.h"
#include "Containers/Set.h"

class FJsonObject;
struct FFlowGraphParsedNode;
struct FFlowGraphParsedNodeAddOn;
struct FFlowGraphParsedConnection;

/**
 * Parses Courier v2 JSON text into FFlowCourierDocument and converts a structurally-valid
 * document into the FFlowGraphParsedNode/FFlowGraphParsedConnection representation consumed by
 * the reconciler and importer. Format guide: agent-docs/CourierTextFormat.md.
 *
 * Two-step contract, mirroring FFlowGraphValidation::ValidateDocument's "issues reported, caller
 * checks severity" pattern rather than early-out exceptions:
 *  1. ParseDocument - JSON syntax + structural validation (identity/parent ambiguity, bad GUIDs,
 *     unresolved aliases, field legality per op Kind, unknown fields). Returns false only on a
 *     hard failure that makes conversion meaningless (malformed JSON, bad FormatVersion).
 *  2. ConvertToParsedGraph - only call once ParseDocument's OutIssues contains no Error-severity
 *     entries. Resolves every alias, flattens addon ops into the nested FFlowGraphParsedNodeAddOn
 *     shape CreateAddOnsRecursive/ReconcileNodeAddOns expect, and fully resolves connection
 *     endpoints from the complete in-memory document.
 *
 * Plain static-method class, not a UObject: it has no UFUNCTIONs and is never instantiated.
 */
class FLOWGRAPHCOURIER_API FFlowCourierConverter
{
public:
	static bool ParseDocument(
		const FString& JsonText,
		FFlowCourierDocument& OutDocument,
		TArray<FFlowCourierIssue>& OutIssues,
		FString& OutErrorMessage);

	static void ConvertToParsedGraph(
		const FFlowCourierDocument& Document,
		TArray<FFlowGraphParsedNode>& OutNodes,
		TArray<FFlowGraphParsedConnection>& OutConnections,
		TArray<FGuid>& OutScopedNodeGuids,
		TMap<FString, FGuid>& OutAliasMap,
		TArray<FFlowCourierIssue>& OutIssues);

private:
	// Recursively compares JsonObject's keys against Struct's reflected properties (using the
	// engine's default camelCase JSON key convention - first character of the authored property
	// name lowercased). Recurses into nested struct and struct-array properties; TMap-typed
	// properties (e.g. Properties) are intentionally not recursed into, since their JSON keys are
	// arbitrary designer property names, not part of the Courier schema.
	static void CheckUnknownFields(
		const TSharedPtr<FJsonObject>& JsonObject,
		UStruct* Struct,
		const FString& JsonPath,
		TArray<FFlowCourierIssue>& OutIssues);

	static FString GetJsonKeyForProperty(const FProperty* Property);

	// Structural validation pass 1: for every op, checks the identity/parent-identity field
	// combination (exactly one of a pair set) and per-Kind field legality, and parses every
	// literal GUID string. Does not resolve aliases (that needs every
	// NewAlias to have been seen first) - see ResolveIdentities.
	static void ValidateOpStructure(const FFlowCourierDocument& Document, TArray<FFlowCourierIssue>& OutIssues);

	// Mints a GUID for every NewAlias across all ops (nodes and addons share one alias namespace),
	// then resolves every Guid/NewAlias, ParentGuid/ParentAlias, and connection endpoint reference
	// against it. Emits BadGuid/UnresolvedAlias issues. ResolvedSelfGuid/ResolvedParentGuid are
	// parallel arrays to Document.Ops; invalid FGuid where resolution failed.
	static void ResolveIdentities(
		const FFlowCourierDocument& Document,
		TMap<FString, FGuid>& OutAliasMap,
		TArray<FGuid>& OutResolvedSelfGuid,
		TArray<FGuid>& OutResolvedParentGuid,
		TArray<FFlowCourierIssue>& OutIssues);

	// Builds the nested FFlowGraphParsedNodeAddOn list for every UpsertAddon/DeleteAddon op whose
	// resolved parent is OwnerGuid, recursing so an addon-of-addon becomes a nested entry rather
	// than a sibling - Courier v2's flat parentGuid representation is rebuilt into the tree shape
	// CreateAddOnsRecursive/ReconcileNodeAddOns already consume.
	static TArray<FFlowGraphParsedNodeAddOn> BuildAddonChildren(
		const FFlowCourierDocument& Document,
		const TArray<FGuid>& ResolvedSelfGuid,
		const TArray<FGuid>& ResolvedParentGuid,
		const FGuid& OwnerGuid,
		TSet<FGuid>& AncestorGuids,
		TArray<FFlowCourierIssue>& OutIssues);
};
