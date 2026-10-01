// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Object.h"
#include "FlowCourierDocument.h"
#include "FlowGraphExporter.generated.h"

class UFlowAsset;
class UFlowNodeBase;
struct FFlowPin;

UCLASS()
class FLOWGRAPHCOURIER_API UFlowGraphExporter : public UObject
{
	GENERATED_BODY()

	// FFlowGraphSubgraphQuery reuses ExportNodes/ExportConnections' NodeGuidFilter overloads so a
	// bounded subgraph read shares the exact same Flow Courier serialization as a full export - no
	// second parser/writer surface for the mutation grammar to drift against.
	friend class FFlowGraphSubgraphQuery;

	// FFlowGraphReconciler::FindOrCreateNode reuses GetPropertyValueAsString for its exact-match
	// comparison (RS13), so a match check compares the same canonical value form the export/patch
	// grammar already relies on rather than a second, potentially-divergent stringification.
	friend class FFlowGraphReconciler;

public:
	UFUNCTION(BlueprintCallable, Category = "Flow Courier")
	static bool ExportFlowGraphToText(UFlowAsset* FlowAsset, const FString& OutputFilePath);

	// Returns a Courier v2 JSON document (formatVersion 2, mode Full) describing the entire asset.
	UFUNCTION(BlueprintCallable, Category = "Flow Courier")
	static FString ExportFlowGraphToString(UFlowAsset* FlowAsset);

	/**
	 * Exports only the nodes named in NodeGuidFilter (plus their addon trees) and only the
	 * connections whose source AND target are both in NodeGuidFilter - same Courier v2 JSON
	 * document shape as ExportFlowGraphToString, just a filtered node/connection set. Used by
	 * FFlowGraphSubgraphQuery::ExportSubgraph so a bounded read is a valid, independently
	 * re-importable document rather than a hand-rolled slice.
	 */
	static FString ExportFlowGraphSubsetToString(UFlowAsset* FlowAsset, const TSet<FGuid>& NodeGuidFilter);

private:
	static void BuildDocumentHeader(const UFlowAsset* FlowAsset, FFlowCourierDocument& OutDocument);

	// NodeGuidFilter: nullptr (default) includes every node, matching the pre-existing behavior
	// exactly. Non-null restricts output to guids present in the set. Appends one UpsertNode op
	// per node, with bReplaceAddons set - a full export is authoritative over each node's addon
	// list, so BuildAddonOps' UpsertAddon ops for that node are the complete list.
	static void BuildDocumentNodes(const UFlowAsset* FlowAsset, FFlowCourierDocument& OutDocument, const TSet<FGuid>* NodeGuidFilter = nullptr);

	// Appends one UpsertAddon op per addon under OwnerNode, parented via ParentGuid = OwnerGuid,
	// recursing into each addon's own children (an addon-of-addon is parented to its immediate
	// addon owner, not the top-level node) - Courier v2's flat parentGuid representation, not the
	// v1 grammar's indentation-nested one.
	static void BuildAddonOps(const UFlowNodeBase* OwnerNode, const FGuid& OwnerGuid, FFlowCourierDocument& OutDocument);

	// NodeGuidFilter: nullptr (default) includes every connection, matching the pre-existing
	// behavior exactly. Non-null skips any connection whose source or target node is not in the
	// set, so a filtered subgraph export never references a node outside its own filtered node list.
	static void BuildDocumentConnections(const UFlowAsset* FlowAsset, FFlowCourierDocument& OutDocument, const TSet<FGuid>* NodeGuidFilter = nullptr);
	static FString SerializeDocumentToJson(const FFlowCourierDocument& Document);
	static FString GetPropertyValueAsString(const FProperty* Property, const void* ValuePtr);

	// True when Object is a subobject this property owns rather than a shared asset it points at.
	// An owned subobject's path leads inside its owning asset, so exporting the bare path yields a
	// document that cannot be imported elsewhere - those export as class-plus-fields instead.
	static bool IsInstancedSubobjectValue(const FObjectProperty& ObjectProperty, const UObject& Object);

	// Emits "ClassPath(Field=Value,...)", listing only properties that differ from the class default.
	static FString ExportInstancedSubobject(const UObject& Object);
	static FString GetPinTypeName(const FFlowPin& Pin);

	// Object path from a typed pin's sub-category (e.g. an InstancedStruct's script struct path).
	// Empty when the pin type carries no sub-category.
	static FString GetPinSubCategoryPath(const FFlowPin& Pin);
};
