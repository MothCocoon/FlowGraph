// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "CoreTypes.h"
#include "Find/FindInFlowEnums.h"
#include "FlowCatalogQuery.h"
#include "FlowMCPMutationContext.h"
#include "Math/IntPoint.h"
#include "UObject/ObjectMacros.h"

#include "FlowMCPToolsetTypes.generated.h"

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPExportFlowAssetRequest
{
	GENERATED_BODY()

	/** Full object path, e.g. "/Game/MyFlows/MyFlow.MyFlow". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString AssetPath;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPExportFlowAssetResult
{
	GENERATED_BODY()

	/** Echo of the requested asset path. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	/** The full Courier v2 JSON document. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ExportedText;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 TextLength = 0;

	/** One entry per disagreement between the editor graph and the runtime node map: a node drawn
	 * but not executable, a node executable but not drawn, or a connection whose endpoint is not in
	 * the node map. Empty on a healthy asset.
	 *
	 * ExportedText above is generated from the runtime node map alone, so it cannot show any of
	 * these - the map agrees with itself by construction. Check this before treating an export as
	 * proof that a mutation landed correctly. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> GraphIntegrityIssues;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPImportAndRegraphFlowAssetRequest
{
	GENERATED_BODY()

	/** A full Courier v2 JSON document (formatVersion 2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString FlowGraphText;

	/** Full object path for the new asset; fails if one already exists there. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString AssetPath;

	/** Save and transaction policy for creating the asset. Dry runs are unsupported. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FFlowMCPMutationOptions Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPImportAndRegraphFlowAssetResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString PackageName;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetName;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 NodeCount = 0;

	/** Requested connections that did not land on the asset after import; the
	  * mutation still reports as applied, but these specific wires need investigation. Empty
	  * when every requested connection is verified present. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> Findings;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPMutationReport Mutation;
};

/** No parameters - lists every FlowAsset subclass in the project. */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPListFlowAssetTypesRequest
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFlowAssetTypeSummary
{
	GENERATED_BODY()

	/** Bare stem, e.g. "MyProjectFlowAsset". */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ClassName;

	/** The authoritative identifier - use this in AssetClassName filters elsewhere. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ClassPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Keywords;

	/** Node classes legal on this FlowAsset subclass - count only; call FindFlowNodeTypes with AssetClassName to see the actual classes. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 AllowedNodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 AllowedAddonCount = 0;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPListFlowAssetTypesResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPFlowAssetTypeSummary> AssetTypes;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 FoundCount = 0;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCheckAddonAttachmentEligibilityRequest
{
	GENERATED_BODY()

	/** Stem, short class name, or full class path of the node or addon the addon would attach to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString ParentClassName;

	/** Stem, short class name, or full class path of the addon to check. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString AddonClassName;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCheckAddonAttachmentEligibilityResult
{
	GENERATED_BODY()

	/** Resolved full path of ParentClassName. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ParentClass;

	/** Resolved full path of AddonClassName. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AddonClass;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bEligible = false;

	/** The raw EFlowAddOnAcceptResult handshake outcome (e.g. "Accept", "Reject", "Undetermined"). */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Result;

	/** Populated only on rejection. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Reason;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPPropertyDescriptor
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Type;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString PinBinding;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Description;

	/** The class that actually declares this property - the queried class itself, or an ancestor. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString DeclaringClass;

	/** Terse stringified current default (bare enum name, not fully scoped; "N elements" for arrays). Empty for opaque struct types. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Value;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPPinDescriptor
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Type;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Description;
};

/** The hand-authored documentation layer for one class. */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPAgentDoc
{
	GENERATED_BODY()

	bool IsEmpty() const { return Guidance.IsEmpty() && Tags.IsEmpty(); }

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Guidance;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> Tags;
};

/** One resolved node or addon class. Which fields are populated depends on the requested Sections. */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPClassDetail
{
	GENERATED_BODY()

	/** The class's bare stem, or its full short name when the stem collides within this response. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Name;

	/** The only authoritative identifier - use this for round-tripping into other ops. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ClassPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Description;

	/** How the class was authored. Derived, never supplied by a caller. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	EFlowClassOrigin Origin = EFlowClassOrigin::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bDeprecated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPAgentDoc Doc;

	/** Knowledge-base article slugs this class is a member of. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> Articles;

	/**
	 * Every configurable or pin-bound property across the class's full ancestor chain, each tagged
	 * with its DeclaringClass - the queried class's own knobs and its inherited ones are both listed
	 * in full, never collapsed into an inherited-only count.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPPropertyDescriptor> Properties;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPPinDescriptor> InputPins;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPPinDescriptor> OutputPins;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCatalogFacet
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Value;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 Count = 0;
};

/**
 * Search the project catalog of Flow node and addon CLASSES by asset type, keyword, article, or name.
 * ClassNames is exclusive - when non-empty, it is the only filter applied (all others below are
 * ignored), and it resolves deprecated classes regardless of bIncludeDeprecated. Sections controls
 * what each row carries; the default, "shape", returns histograms and no rows at all.
 */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFindFlowNodeTypesRequest
{
	GENERATED_BODY()

	/** Restrict to classes legal on this FlowAsset subclass (short name or full path); empty = whole project. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString AssetClassName;

	/** Stems, short names, or full class paths. When non-empty, this is the ONLY filter applied. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TArray<FString> ClassNames;

	/** Search nodes, addons, or both. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	EFlowCatalogKind Kind = EFlowCatalogKind::Any;

	/** Case-insensitive substring match over class name & description. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString Query;

	/** Exact, lower-cased. A class matches if it carries ANY listed keyword or authored tag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TArray<FString> Keywords;

	/** Exact. A class matches if it is in ANY listed category. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TArray<FString> Categories;

	/** Article slugs ("pattern:<slug>"). A class matches if it carries ANY listed slug. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TArray<FString> Articles;

	/** A class matches if it declares a pin of ANY listed type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TArray<FString> PinTypes;

	/** Restrict to one origin; defaults to Any. Where a class's doc has to be written differs per origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	EFlowClassOrigin Origin = EFlowClassOrigin::Any;

	/**
	 * Comma-separated sections: names, doc, properties, pins, articles, usage, origin, deprecation,
	 * signature - or a preset: "shape" (default, histograms only), "brief" (names + description), "full".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString Sections;

	/** Deprecated classes are hidden by default; set this to include them in a filtered query. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	bool bIncludeDeprecated = false;

	/** 0 = unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	int32 Limit = 0;

	/** Number of matching rows to skip before returning results. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	int32 Offset = 0;
};

/** Category/tag/article histograms over the matched scope - what the "shape" section returns. */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCatalogShape
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 NodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 AddonCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 DocumentedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 DeprecatedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPCatalogFacet> Categories;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPCatalogFacet> Keywords;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPCatalogFacet> Tags;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPCatalogFacet> Articles;
};

/** A requested stem that more than one class could have meant. Deliberately left unresolved. */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPAmbiguousClassName
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Stem;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> Candidates;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFindFlowNodeTypesResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPClassDetail> Nodes;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPClassDetail> Addons;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 NodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 AddonCount = 0;

	/** Pre-paging match counts (i.e. before Limit/Offset are applied). */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 TotalNodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 TotalAddonCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bTruncated = false;

	/** Echo of the sections actually applied. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Sections;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPCatalogShape Shape;

	/** Names in ClassNames[] that resolved to nothing. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> UnresolvedClassNames;

	/** Stems in ClassNames[] that matched more than one class. Nothing was guessed on your behalf. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPAmbiguousClassName> AmbiguousClassNames;
};

/**
 * Write the hand-authored documentation layer onto one node or addon class.
 *
 * Guidance/Tags/Articles only - AgentDoc has no description field. A class's "what does this do for
 * me?" text lives on the class itself (Blueprint description / native or script doc comment, read
 * back via GetNodeToolTip()) and is edited there directly, not through this op.
 *
 * Origin-aware by necessity: a Blueprint class stores its doc as a saved default, while a native or
 * script class returns a compiled-in constant, so for those the op reports Persisted false and hands
 * back the source to paste rather than performing a write that vanishes on restart.
 */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPSetFlowAgentDocRequest
{
	GENERATED_BODY()

	/** Stem, short class name, or full class path of the class to document. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString ClassName;

	/** When to use, when NOT to use, and gotchas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString Guidance;

	/** Intent words a designer would search for. Lower-case. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TArray<FString> Tags;

	/** Article slugs of the form "pattern:<slug>" or "concept:<slug>". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TArray<FString> Articles;

	/** Save and transaction policy for a Blueprint class documentation write. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FFlowMCPMutationOptions Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPSetFlowAgentDocResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ClassPath;

	/** How the class was authored, which decides where its doc can live. Derived, not supplied. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	EFlowClassOrigin Origin = EFlowClassOrigin::Unknown;

	/** True only when the write will survive an editor restart. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bPersisted = false;

	/** For a class whose defaults are compiled in, the exact GetAgentDoc() override to paste. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString SourceSnippet;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Note;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPMutationReport Mutation;
};

/** Where a node or addon class is actually used in shipped content. */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFindFlowNodeUsageRequest
{
	GENERATED_BODY()

	/** Stem, short class name, or full class path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString ClassName;

	/** Maximum example assets to return; 0 uses a small default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	int32 Limit = 0;

	/** Snippets are the expensive part of the payload, so they are opt-in only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	bool bIncludeSnippets = false;
};

/** One containing asset, smallest graph first so the most readable example leads. */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFlowNodeUsageExample
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 InstanceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 AssetNodeCount = 0;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFindFlowNodeUsageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ClassPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 InstanceCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 AssetCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPFlowNodeUsageExample> Examples;

	/** Classes commonly attached alongside this one - the only view of addon-contributed pins. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPCatalogFacet> CoAttachedAddOns;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> Snippets;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPNodeSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Title;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ClassPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bIsEntryPoint = false;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFindFlowNodesRequest
{
	GENERATED_BODY()

	/** Full object path of the FlowAsset to search. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString AssetPath;

	/** Case-insensitive substring of the live editor node title; empty matches any title. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString TitleFilter;

	/** Node class short name or full path; empty matches any class. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString ClassFilter;

	/** Return only nodes with no incoming connections. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	bool bEntryPointsOnly = false;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFindFlowNodesResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPNodeSummary> Nodes;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 NodeCount = 0;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPExportFlowSubgraphRequest
{
	GENERATED_BODY()

	/** Full object path of the FlowAsset containing the start node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString AssetPath;

	/** GUID of the node the undirected reachability search starts from - must exist on AssetPath. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString StartNodeGuid;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPExportFlowSubgraphResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString StartNodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ExportedText;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 NodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 TextLength = 0;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPPlanFlowSubgraphFromSelectionRequest
{
	GENERATED_BODY()

	/** Full object path of the FlowAsset containing the selected nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString TargetAssetPath;

	/** GUIDs of the top-level nodes to move into the new sub-graph asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TArray<FString> SelectionGuids;

	/** New asset name, without a package path. Flow requires the Subgraph_ prefix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString NewAssetName;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFlowSubgraphSelectionPlan
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bCanApply = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 SelectedNodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 EntryCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 ExitCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> Errors;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> Warnings;

	/** Names exposed as inputs on the generated SubGraph node. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> InterfaceInputs;

	/** Names exposed as outputs on the generated SubGraph node. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> InterfaceOutputs;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPPlanFlowSubgraphFromSelectionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString TargetAssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NewAssetName;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPFlowSubgraphSelectionPlan Plan;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCreateFlowSubgraphFromSelectionRequest
{
	GENERATED_BODY()

	/** Full object path of the FlowAsset containing the selected nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString TargetAssetPath;

	/** GUIDs of top-level nodes to move into the child asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TArray<FString> SelectionGuids;

	/** New child asset name without a package path; requires the Subgraph_ prefix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString NewAssetName;

	/** Save and transaction policy; this operation requires its own transaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FFlowMCPMutationOptions Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCreateFlowSubgraphFromSelectionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString TargetAssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NewAssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString SubgraphNodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bApplySucceeded = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPFlowSubgraphSelectionPlan Plan;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPMutationReport Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFindOrCreateFlowNodeRequest
{
	GENERATED_BODY()

	/** Full object path of the FlowAsset to search or update. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString TargetAssetPath;

	/** Class path or short name of the node to find or create. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString NodeType;

	/** Exact-match property name/value pairs (export-text strings). A node of NodeType matches only if every listed property is equal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TMap<FString, FString> MatchProperties;

	/** Save and transaction policy when a node needs to be created. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FFlowMCPMutationOptions Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPFindOrCreateFlowNodeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bCreated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPMutationReport Mutation;
};

/**
 * Creates a Blueprint that Flow will actually recognise as a node or add-on.
 *
 * The asset class is not a choice the caller makes - it is decided by the parent class, because both
 * the Flow palette and the Courier catalog gather by asset class rather than by generated class. A
 * plain Blueprint parented to a Flow node compiles, and is then invisible to both.
 */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCreateFlowNodeBlueprintRequest
{
	GENERATED_BODY()

	/** Destination package folder, e.g. "/Game/FlowNodes". No asset name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString PackagePath;

	/** Asset name without a path or extension. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString AssetName;

	/**
	 * Short name or full path of the class to derive from. Must be under UFlowNode or under
	 * UFlowNodeAddOn - which one decides the asset class, so UFlowNodeBase itself is rejected as
	 * ambiguous rather than guessed at.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString ParentClass;

	/** Palette title. Optional: when empty the node falls back to its class name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString DisplayName;

	/** Palette tooltip. Optional: when empty the node falls back to its class tooltip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString Description;

	/** Save and transaction policy for creating the Blueprint asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FFlowMCPMutationOptions Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCreateFlowNodeBlueprintResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	/**
	 * The asset class created from the parent. Check that it is a Flow node or addon Blueprint class;
	 * a plain "/Script/Engine.Blueprint" is not discoverable through the Flow catalog.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetClassPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString GeneratedClassPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ParentClassPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bCompiled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> CompileErrors;

	/**
	 * Whether the new class resolves through the same catalog path FindFlowNodeTypes uses. False
	 * means the asset exists but the palette will not offer it.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bResolvedInCatalog = false;

	/** Non-fatal observations, such as a display name or description left to fall back. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Note;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPMutationReport Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPAutoFormatFlowGraphRequest
{
	GENERATED_BODY()

	/** Full object path of the FlowAsset whose editor nodes will be arranged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString TargetAssetPath;

	/** Empty formats the whole graph. Non-empty formats only these nodes, anchored to their own pre-format bounding box rather than the graph origin, so unselected nodes are never overlapped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TArray<FString> SelectionGuids;

	/** Preview, save, and transaction policy for the layout change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FFlowMCPMutationOptions Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPAutoFormatFlowGraphResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 NodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TMap<FString, FIntPoint> Positions;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> UnresolvedGuids;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPMutationReport Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPReconstructFlowGraphRequest
{
	GENERATED_BODY()

	/** A FlowAsset whose editor graph should be rebuilt from its runtime nodes/connections. Use
	 * this after DuplicateFlowAsset/AssetTools.duplicate to repoint a fresh duplicate's connections
	 * at its own node GUIDs, without opening the asset in the Flow editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString TargetAssetPath;

	/** Preview, save, and transaction policy for rebuilding the editor graph. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FFlowMCPMutationOptions Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPReconstructFlowGraphResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 NodeCount = 0;

	/** Editor nodes destroyed because their runtime node was no longer registered. A non-zero count
	 * means this call repaired existing damage rather than merely rebuilding. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 OrphanedEditorNodesRemoved = 0;

	/** Remaining disagreements between the editor graph and the runtime node map after the rebuild.
	 * Empty is the expected result; anything here survived a full reconstruct. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> GraphIntegrityIssues;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPMutationReport Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPExportFlowCatalogRequest
{
	GENERATED_BODY()

	/** Absolute path of the UTF-8 JSON file to write. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString OutputFilePath;

	/** Optional FlowAsset class to scope the export to (e.g. "/Script/Flow.FlowAsset"). Empty
	 * exports every node/addon class known to the catalog regardless of asset type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString AssetClassName;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPExportFlowCatalogResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString OutputFilePath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bSucceeded = false;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPConnectionRef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString SourceGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString SourcePin;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString TargetGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString TargetPin;
};

/**
 * One addon-level difference under a changed node, flattened out of the recursive C++-only
 * FFlowGraphDiffAddOn tree (FlowGraphDiff.h) so it can be reflected: UHT forbids a USTRUCT
 * holding a TArray of itself. ParentGuid is the owning node's GUID for a top-level addon, or the
 * parent addon's GUID for a nested addon-of-addon (mirrors FFlowReconcileAddonEntry::OwnerNodeGuid).
 */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPAddonDiffEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AddonGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ParentGuid;

	// "added" | "removed" | "changed"
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString ChangeType;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AddonType;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TMap<FString, FString> ChangedProperties;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPChangedNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TMap<FString, FString> ChangedProperties;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bPosChanged = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString OldPos;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NewPos;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPAddonDiffEntry> AddonDiffs;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPDiffFlowAssetRequest
{
	GENERATED_BODY()

	/** A FlowAsset object path, or a raw Courier v2 JSON document - either is accepted for both sides. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString Old;

	/** Same rules as Old. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString New;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPDiffFlowAssetResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString OldSource;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NewSource;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bHasDifferences = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> AddedNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> RemovedNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPChangedNode> ChangedNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPConnectionRef> AddedConnections;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPConnectionRef> RemovedConnections;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPReconcileAddonEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString OwnerNodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AddonGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AddonType;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TMap<FString, FString> Properties;
};

// Every array here is a verified claim about post-apply state, checked against the asset after
// applying, never populated from the request. An item requested but not achieved appears as a
// Findings entry instead.
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPReconcilePlan
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> NodesAdded;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> NodesUpdated;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> NodesDeleted;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> ConnectionsAdded;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> ConnectionsRemoved;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPReconcileAddonEntry> AddonsAdded;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPReconcileAddonEntry> AddonsUpdated;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPReconcileAddonEntry> AddonsDeleted;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bIsEmpty = true;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPValidationFinding
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Severity;

	/**
	 * Stable machine-readable identifier for what went wrong, e.g. "ExecFanOut", "DanglingEndpoint",
	 * "PinNotFound", "UnresolvedAlias". Branch on this rather than matching against Message - the
	 * message wording is free text and may be reworded, the code is a contract.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Code;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Message;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AddonGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString PinName;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPReconcileMetrics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 NodesTouched = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 NodesPreserved = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 InputSizeBytes = 0;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPReplaceFlowNodeClassRequest
{
	GENERATED_BODY()

	/** Full object path of the FlowAsset containing the node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString AssetPath;

	/** GUID of the node to replace; the replacement retains this identity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString NodeGuid;

	/** Full class path of the replacement Flow node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString NewNodeClass;

	/** Source property name to replacement property name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TMap<FString, FString> PropertyMappings;

	/** Source pin name to replacement pin name for affected connections. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TMap<FString, FString> PinMappings;

	/** Addon GUID string to replacement addon class path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TMap<FString, FString> AddOnClassMappings;

	/** "AddonGuid.SourceProperty" to replacement addon property name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	TMap<FString, FString> AddOnPropertyMappings;

	/** Permit a save to remint duplicate addon GUIDs elsewhere in the asset. Duplicates on the
	 * target node still block replacement. Use only when those identity changes are accepted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	bool bAllowDuplicateAddonGuidRepair = false;

	/** Preview, save, and transaction policy for the class replacement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FFlowMCPMutationOptions Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPReplaceFlowNodeClassResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString PreviousClass;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NewClass;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bCanReplace = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bReplaced = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> ReplacedAddOnGuids;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> Findings;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPMutationReport Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPApplyFlowPatchRequest
{
	GENERATED_BODY()

	/** Existing asset to patch, or a new one to create if AssetClass is set in MutationText and nothing exists at this path yet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString TargetAssetPath;

	/** A Courier v2 JSON document. Patch mode changes only what it mentions; Full mode is authoritative over the whole asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString MutationText;

	/** Preview, save, and transaction policy for applying the patch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FFlowMCPMutationOptions Mutation;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPApplyFlowPatchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	/** Check this, not the absence of an error - a blocked apply still returns a populated Result rather than raising. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bApplySucceeded = false;

	/** Always populated, even when blocked - what the patch would have changed. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPReconcilePlan Plan;

	/** newAlias -> minted GUID for every alias the document defined. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TMap<FString, FString> AliasMap;

	/** Free-text messages; ValidationFindings below is the structured equivalent and should be preferred. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> Findings;

	/** Per-issue severity, message, and node/addon/pin location. Any Error-severity entry means bHasBlockingError is true and nothing was applied. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPValidationFinding> ValidationFindings;

	/** True when ValidationFindings contains an Error - the whole apply was blocked and the asset was not modified. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bHasBlockingError = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPReconcileMetrics Metrics;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FFlowMCPMutationReport Mutation;
};

/** How a Courier v2 op field relates to a given op Kind. */
UENUM(BlueprintType)
enum class EFlowMCPCourierFieldLegality : uint8
{
	/** Must always be set on this Kind. */
	Required,
	/** Must be set only when the op creates a new node/addon (Guid empty, NewAlias set). */
	RequiredOnCreate,
	/** May be set on this Kind; omitting it is always legal. */
	Optional,
	/** Setting this field on this Kind is a validation error (FieldNotLegalForKind). */
	Illegal,
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCourierFieldGrammar
{
	GENERATED_BODY()

	/** FFlowCourierOp field name, in the JSON's camelCase spelling (e.g. "parentGuid"). */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString FieldName;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	EFlowMCPCourierFieldLegality Legality = EFlowMCPCourierFieldLegality::Optional;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Notes;
};

/**
 * A set of two or more field names where exactly one must be set; the individual fields'
 * Legality in FFlowMCPCourierOpKindGrammar.Fields is Optional for each member of the group, since
 * no single one of them is independently required. This group is what actually carries the
 * requirement: at least one member must be set, and setting more than one is ambiguous.
 */
USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCourierFieldGroup
{
	GENERATED_BODY()

	/** Field names in the group, e.g. ["guid", "newAlias"] or ["source.nodeGuid", "source.nodeAlias"]. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FString> FieldNames;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Notes;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPCourierOpKindGrammar
{
	GENERATED_BODY()

	/** FFlowCourierOp.Kind value, e.g. "UpsertNode". */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Kind;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Description;

	/** Legality of every FFlowCourierOp field for this Kind. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPCourierFieldGrammar> Fields;

	/** Exactly-one-of constraints across two or more fields in this Kind, e.g. guid/newAlias. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPCourierFieldGroup> ExactlyOneOfGroups;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPDescribeCourierGrammarRequest
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPDescribeCourierGrammarResult
{
	GENERATED_BODY()

	/** Every legal FFlowCourierOp.Kind value and its per-field legality. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPCourierOpKindGrammar> Kinds;

	/** Legality notes for the top-level FFlowCourierDocument fields (formatVersion, mode, assetClass, etc.), independent of Kind. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPCourierFieldGrammar> DocumentFields;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPSearchMatchedPin
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString PinName;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bConnected = false;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPSearchResultItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NodeTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString NodeType;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 MatchedFlags = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString MatchedSnippet;

	/** Populated when the query matched the Pin Names category, qualified by the request's PinDirection/PinConnection filters. */
	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPSearchMatchedPin> MatchedPins;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	bool bIsSubgraphNode = false;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString SubgraphOwnerAssetPath;
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPSearchFlowAssetsRequest
{
	GENERATED_BODY()

	/** Whitespace-delimited tokens, ANDed together - every token must match for a node to be a hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString Query;

	/** EFlowSearchFlags bitmask of what to search (titles, comments, classes, property values, ...). 0 means the default flag set, not "search nothing". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier", Meta = (Bitmask, BitmaskEnum = "/Script/FlowEditor.EFlowSearchFlags"))
	int32 Flags = 0;

	/** ThisAssetOnly and AllOfThisType both require ContextAssetPath to be set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	EFlowSearchScope Scope = EFlowSearchScope::AllFlowAssets;

	/** How many subgraph levels FlowNode_SubGraph recursion follows before it stops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	int32 MaxDepth = 3;

	/** Required when Scope is ThisAssetOnly or AllOfThisType; ignored for AllFlowAssets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString ContextAssetPath;

	/** "any"/"input"/"output" - qualifies a Pin Names category match by pin direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString PinDirection = TEXT("any");

	/** "any"/"connected"/"unconnected" - qualifies a Pin Names category match by connection state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Courier")
	FString PinConnection = TEXT("any");
};

USTRUCT(BlueprintType)
struct FLOWGRAPHCOURIER_API FFlowMCPSearchFlowAssetsResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	TArray<FFlowMCPSearchResultItem> Results;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 ResultCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Query;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString Scope;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	int32 Flags = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString PinDirection;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Courier")
	FString PinConnection;
};
