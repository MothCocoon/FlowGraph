// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Object.h"
#include "FlowGraphValidation.h"
#include "FlowCourierDocument.generated.h"

// Courier v2 JSON document model. See agent-docs/CourierTextFormat.md for format semantics.
// One document type supports complete graph exports and scoped mutation patches.

UENUM()
enum class EFlowCourierMode : uint8
{
	Full,
	Patch,
};

UENUM()
enum class EFlowCourierOpKind : uint8
{
	UpsertNode,
	DeleteNode,
	UpsertAddon,
	DeleteAddon,
	AddConnection,
	RemoveConnection,
};

// A single pin declaration. Name is carried verbatim and never trimmed or special-cased - some
// nodes deliberately use a whitespace-only name to render a blank pin label, and JSON quoting
// makes that unambiguous to the parser.
USTRUCT()
struct FLOWGRAPHCOURIER_API FFlowCourierPin
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	// "Exec", "Vector", "InstancedStruct", etc. Case-insensitive "Exec" denotes an exec pin.
	UPROPERTY()
	FString Type;

	// Object path from a typed pin's sub-category (e.g. an InstancedStruct's script struct path).
	// Empty when the pin type carries no sub-category.
	UPROPERTY()
	FString SubCategoryPath;
};

// One endpoint of a connection op. Exactly one of NodeGuid/NodeAlias must be set.
USTRUCT()
struct FLOWGRAPHCOURIER_API FFlowCourierEndpoint
{
	GENERATED_BODY()

	UPROPERTY()
	FString NodeGuid;

	UPROPERTY()
	FString NodeAlias;

	UPROPERTY()
	FString Pin;
};

// One mutation. Kind determines which of the remaining fields are legal - see the spec's field
// legality table (Section 5). Supplying a field the op's Kind does not use is a validation error,
// not silently ignored.
USTRUCT()
struct FLOWGRAPHCOURIER_API FFlowCourierOp
{
	GENERATED_BODY()

	UPROPERTY()
	EFlowCourierOpKind Kind = EFlowCourierOpKind::UpsertNode;

	// Identity of the node/addon this op targets. Exactly one of Guid/NewAlias must be set on any
	// op that targets an object (Upsert*/Delete*). NewAlias names an object being created in this
	// document; Guid names one that already exists.
	UPROPERTY()
	FString Guid;

	UPROPERTY()
	FString NewAlias;

	// Addon ops only: the owning node GUID, or the parent addon's GUID for a nested addon-of-addon.
	// Exactly one of ParentGuid/ParentAlias must be set. Addons are always flat - there is no
	// nested-array representation; arbitrary depth is expressed by chaining ParentGuid/ParentAlias.
	UPROPERTY()
	FString ParentGuid;

	UPROPERTY()
	FString ParentAlias;

	// Class path. Required when Kind creates an object (Guid empty, NewAlias set). On an update
	// (Guid set) it is optional; if present it must equal the existing object's class - a mismatch
	// is a hard ClassMismatch error, since the reconciler cannot swap a live object's class while
	// preserving its GUID.
	UPROPERTY()
	FString Type;

	// UpsertNode/UpsertAddon only. Merge semantics: an omitted key leaves the existing value
	// unchanged. Values are UE property export-text strings.
	UPROPERTY()
	TMap<FString, FString> Properties;

	UPROPERTY()
	TArray<FFlowCourierPin> InputPins;

	UPROPERTY()
	TArray<FFlowCourierPin> OutputPins;

	// UpsertNode only. Optional/advisory editor position.
	UPROPERTY()
	bool bHasPosition = false;

	UPROPERTY()
	FIntPoint Position = FIntPoint::ZeroValue;

	// UpsertNode only. Optional designer comment (WITH_EDITOR).
	UPROPERTY()
	FString Comment;

	// UpsertNode only. When true, this op's UpsertAddon ops (parented to this node, directly or
	// transitively) are the complete authoritative addon list for this node - any existing addon
	// not reached that way is deleted. Default false is merge: unmentioned addons are left alone.
	UPROPERTY()
	bool bReplaceAddons = false;

	// AddConnection/RemoveConnection only.
	UPROPERTY()
	FFlowCourierEndpoint Source;

	UPROPERTY()
	FFlowCourierEndpoint Target;
};

// Top-level Courier v2 document. One type for both a full graph export and a mutation patch.
USTRUCT()
struct FLOWGRAPHCOURIER_API FFlowCourierDocument
{
	GENERATED_BODY()

	// Must be 2. Any other value is a hard error - there is no silent version coercion.
	UPROPERTY()
	int32 FormatVersion = 2;

	UPROPERTY()
	EFlowCourierMode Mode = EFlowCourierMode::Patch;

	// Full class path. Required when creating an asset; on an existing asset it must match, or it
	// is an error.
	UPROPERTY()
	FString AssetClass;

	UPROPERTY()
	bool bWorldBound = false;

	UPROPERTY()
	FString ExpectedOwnerClass;

	// Patch mode only. Nodes this document is authoritative over: a listed node with no
	// corresponding UpsertNode op is deleted. Empty means nothing is implicitly deleted. Illegal
	// (ScopeWithFullMode) in Full mode, where the whole document is already authoritative.
	UPROPERTY()
	TArray<FString> ScopeNodeGuids;

	UPROPERTY()
	TArray<FFlowCourierOp> Ops;
};

// One validation/apply issue. Every issue carries a Path so a caller can locate the offending
// field without re-deriving it from the message text.
USTRUCT()
struct FLOWGRAPHCOURIER_API FFlowCourierIssue
{
	GENERATED_BODY()

	UPROPERTY()
	EFlowValidationSeverity Severity = EFlowValidationSeverity::Error;

	// Path into the document, e.g. "ops[3].source.pin".
	UPROPERTY()
	FString Path;

	// Stable machine-readable identifier, e.g. "BadGuid", "UnresolvedAlias". See spec Section 8
	// for the full set.
	UPROPERTY()
	FString Code;

	UPROPERTY()
	FString Message;
};
