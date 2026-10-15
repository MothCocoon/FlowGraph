// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphDiff.h"
#include "FlowGraphImporter.h"
#include "FlowCourierConverter.h"
#include "FlowCourierDocument.h"

// Exec pin lookup keyed by (source node GUID, source pin name). True = exec, false = data.
// Built from the union of both documents' OutputPins declarations so added and removed connections
// both resolve correctly. Source documents omitting pin type info leave this empty; in that case
// bIsExecPin defaults to true (exec assumption, matching the reconciler convention).
using FExecPinLookup = TMap<FGuid, TMap<FName, bool>>;

static FExecPinLookup BuildExecPinLookup(
	const TArray<FFlowGraphParsedNode>& NodesA,
	const TArray<FFlowGraphParsedNode>& NodesB)
{
	FExecPinLookup Result;

	auto AddPins = [&Result](const TArray<FFlowGraphParsedNode>& Nodes)
	{
		for (const FFlowGraphParsedNode& Node : Nodes)
		{
			if (Node.bIsDeleteMarker || !Node.NodeGuid.IsValid())
			{
				continue;
			}

			TMap<FName, bool>& PinMap = Result.FindOrAdd(Node.NodeGuid);
			for (const FString& PinDecl : Node.OutputPins)
			{
				FName PinName;
				FString PinType;
				FString SubCategoryPath;
				if (UFlowGraphImporter::ParsePinDecl(PinDecl, PinName, PinType, SubCategoryPath))
				{
					PinMap.FindOrAdd(PinName) = PinType.Equals(TEXT("Exec"), ESearchCase::IgnoreCase);
				}
			}
		}
	};

	AddPins(NodesA);
	AddPins(NodesB);
	return Result;
}

// Stable key for a connection, used for order-independent set-membership comparison.
// Delete-marker and alias entries are always skipped by callers before reaching this.
static FString MakeConnectionKey(const FFlowGraphParsedConnection& Connection)
{
	return FString::Printf(TEXT("%s.%s->%s.%s"),
		*Connection.SourceNodeGuid.ToString(), *Connection.SourcePinName.ToString(),
		*Connection.TargetNodeGuid.ToString(), *Connection.TargetPinName.ToString());
}

static FFlowGraphDiffConnection ToDiffConnection(const FFlowGraphParsedConnection& Connection, const FExecPinLookup& ExecLookup)
{
	FFlowGraphDiffConnection DiffConnection;
	DiffConnection.SourceNodeGuid = Connection.SourceNodeGuid;
	DiffConnection.SourcePinName = Connection.SourcePinName;
	DiffConnection.TargetNodeGuid = Connection.TargetNodeGuid;
	DiffConnection.TargetPinName = Connection.TargetPinName;

	// Derive exec/data from the source node's declared output pin type. Default true (exec) when
	// no type information is available (pin not listed).
	const TMap<FName, bool>* NodePins = ExecLookup.Find(Connection.SourceNodeGuid);
	if (NodePins)
	{
		const bool* bIsExec = NodePins->Find(Connection.SourcePinName);
		DiffConnection.bIsExecPin = bIsExec ? *bIsExec : true;
	}
	else
	{
		DiffConnection.bIsExecPin = true;
	}

	return DiffConnection;
}

// Renders "OldValue -> NewValue" for a property present in either map. Same convention the node
// property diff uses, so an absent side reads as "<absent>".
static void CollectPropertyDelta(
	const TMap<FString, FString>& OldProperties,
	const TMap<FString, FString>& NewProperties,
	TMap<FString, FString>& OutChangedProperties)
{
	TSet<FString> AllKeys;
	OldProperties.GetKeys(AllKeys);
	TSet<FString> NewKeys;
	NewProperties.GetKeys(NewKeys);
	AllKeys.Append(NewKeys);

	for (const FString& Key : AllKeys)
	{
		const FString* OldValue = OldProperties.Find(Key);
		const FString* NewValue = NewProperties.Find(Key);
		const FString OldValueStr = OldValue ? *OldValue : FString(TEXT("<absent>"));
		const FString NewValueStr = NewValue ? *NewValue : FString(TEXT("<absent>"));
		if (OldValueStr != NewValueStr)
		{
			OutChangedProperties.Add(Key, FString::Printf(TEXT("%s -> %s"), *OldValueStr, *NewValueStr));
		}
	}
}

// Builds a FFlowGraphDiffAddOn describing a fully-added (bAsAdded=true) or fully-removed
// (bAsAdded=false) addon subtree: every property is a delta against "<absent>", and every nested
// child is recursively reported into AddedChildren/RemovedChildren. Used when an addon exists on
// only one side of the diff.
static FFlowGraphDiffAddOn MakeOneSidedAddOn(const FFlowGraphParsedNodeAddOn& AddOn, bool bAsAdded)
{
	FFlowGraphDiffAddOn Result;
	Result.AddOnGuid = AddOn.AddOnGuid;
	Result.AddOnType = AddOn.AddOnType;

	static const TMap<FString, FString> EmptyProperties;
	if (bAsAdded)
	{
		CollectPropertyDelta(EmptyProperties, AddOn.Properties, Result.ChangedProperties);
	}
	else
	{
		CollectPropertyDelta(AddOn.Properties, EmptyProperties, Result.ChangedProperties);
	}

	for (const FFlowGraphParsedNodeAddOn& Child : AddOn.AddOns)
	{
		if (Child.bIsDeleteMarker || !Child.AddOnGuid.IsValid())
		{
			continue;
		}
		if (bAsAdded)
		{
			Result.AddedChildren.Add(MakeOneSidedAddOn(Child, true));
		}
		else
		{
			Result.RemovedChildren.Add(MakeOneSidedAddOn(Child, false));
		}
	}

	return Result;
}

// Diffs two sibling addon lists by AddOnGuid, recursing into nested children. An addon present on
// only one side is reported as a fully-added/removed subtree; one present on both with a type
// change, property change, or non-empty child delta is reported as changed. Delete-marker and
// invalid-GUID entries are skipped (never present in a plain export, guarded defensively).
static void DiffAddOnsRecursive(
	const TArray<FFlowGraphParsedNodeAddOn>& OldAddOns,
	const TArray<FFlowGraphParsedNodeAddOn>& NewAddOns,
	TArray<FFlowGraphDiffAddOn>& OutAdded,
	TArray<FFlowGraphDiffAddOn>& OutRemoved,
	TArray<FFlowGraphDiffAddOn>& OutChanged)
{
	TMap<FGuid, const FFlowGraphParsedNodeAddOn*> OldByGuid;
	for (const FFlowGraphParsedNodeAddOn& AddOn : OldAddOns)
	{
		if (!AddOn.bIsDeleteMarker && AddOn.AddOnGuid.IsValid())
		{
			OldByGuid.Add(AddOn.AddOnGuid, &AddOn);
		}
	}

	TMap<FGuid, const FFlowGraphParsedNodeAddOn*> NewByGuid;
	for (const FFlowGraphParsedNodeAddOn& AddOn : NewAddOns)
	{
		if (!AddOn.bIsDeleteMarker && AddOn.AddOnGuid.IsValid())
		{
			NewByGuid.Add(AddOn.AddOnGuid, &AddOn);
		}
	}

	for (const TPair<FGuid, const FFlowGraphParsedNodeAddOn*>& Pair : NewByGuid)
	{
		if (!OldByGuid.Contains(Pair.Key))
		{
			OutAdded.Add(MakeOneSidedAddOn(*Pair.Value, true));
		}
	}

	for (const TPair<FGuid, const FFlowGraphParsedNodeAddOn*>& Pair : OldByGuid)
	{
		if (!NewByGuid.Contains(Pair.Key))
		{
			OutRemoved.Add(MakeOneSidedAddOn(*Pair.Value, false));
		}
	}

	for (const TPair<FGuid, const FFlowGraphParsedNodeAddOn*>& OldPair : OldByGuid)
	{
		const FFlowGraphParsedNodeAddOn** NewAddOnPtr = NewByGuid.Find(OldPair.Key);
		if (!NewAddOnPtr)
		{
			continue;
		}

		const FFlowGraphParsedNodeAddOn& OldAddOn = *OldPair.Value;
		const FFlowGraphParsedNodeAddOn& NewAddOn = **NewAddOnPtr;

		FFlowGraphDiffAddOn ChangedAddOn;
		ChangedAddOn.AddOnGuid = OldPair.Key;
		ChangedAddOn.AddOnType = NewAddOn.AddOnType;

		CollectPropertyDelta(OldAddOn.Properties, NewAddOn.Properties, ChangedAddOn.ChangedProperties);

		const bool bTypeChanged = OldAddOn.AddOnType != NewAddOn.AddOnType;
		if (bTypeChanged)
		{
			ChangedAddOn.ChangedProperties.Add(TEXT("Type"),
				FString::Printf(TEXT("%s -> %s"), *OldAddOn.AddOnType, *NewAddOn.AddOnType));
		}

		DiffAddOnsRecursive(OldAddOn.AddOns, NewAddOn.AddOns,
			ChangedAddOn.AddedChildren, ChangedAddOn.RemovedChildren, ChangedAddOn.ChangedChildren);

		const bool bHasChildDelta = !ChangedAddOn.AddedChildren.IsEmpty()
			|| !ChangedAddOn.RemovedChildren.IsEmpty() || !ChangedAddOn.ChangedChildren.IsEmpty();

		if (!ChangedAddOn.ChangedProperties.IsEmpty() || bHasChildDelta)
		{
			OutChanged.Add(MoveTemp(ChangedAddOn));
		}
	}
}

namespace
{
	// Parses a Courier v2 JSON document into flat node/connection lists for diffing. Returns
	// false on a hard parse failure (malformed JSON, wrong formatVersion) or if the converter
	// reports any Error-severity issue - a document that fails to convert cleanly cannot be
	// diffed meaningfully.
	bool ParseDocumentForDiff(
		const FString& DocumentJson,
		TArray<FFlowGraphParsedNode>& OutNodes,
		TArray<FFlowGraphParsedConnection>& OutConnections,
		FString& OutErrorMessage)
	{
		FFlowCourierDocument Document;
		TArray<FFlowCourierIssue> Issues;
		if (!FFlowCourierConverter::ParseDocument(DocumentJson, Document, Issues, OutErrorMessage))
		{
			return false;
		}

		TArray<FGuid> ScopedNodeGuidsUnused;
		TMap<FString, FGuid> AliasMapUnused;
		FFlowCourierConverter::ConvertToParsedGraph(Document, OutNodes, OutConnections, ScopedNodeGuidsUnused, AliasMapUnused, Issues);

		for (const FFlowCourierIssue& Issue : Issues)
		{
			if (Issue.Severity == EFlowValidationSeverity::Error)
			{
				OutErrorMessage = FString::Printf(TEXT("[%s] %s: %s"), *Issue.Code, *Issue.Path, *Issue.Message);
				return false;
			}
		}

		return true;
	}
}

bool UFlowGraphDiff::ComputeDiff(
	const FString& OldText,
	const FString& NewText,
	FFlowGraphDiffResult& OutDiff,
	FString& OutErrorMessage)
{
	OutDiff = FFlowGraphDiffResult();
	OutErrorMessage.Empty();

	TArray<FFlowGraphParsedNode> OldNodes;
	TArray<FFlowGraphParsedNode> NewNodes;
	TArray<FFlowGraphParsedConnection> OldConnections;
	TArray<FFlowGraphParsedConnection> NewConnections;

	if (!ParseDocumentForDiff(OldText, OldNodes, OldConnections, OutErrorMessage))
	{
		OutErrorMessage = FString::Printf(TEXT("Failed to parse old document: %s"), *OutErrorMessage);
		return false;
	}
	if (!ParseDocumentForDiff(NewText, NewNodes, NewConnections, OutErrorMessage))
	{
		OutErrorMessage = FString::Printf(TEXT("Failed to parse new document: %s"), *OutErrorMessage);
		return false;
	}

	// Node diff - keyed by GUID, skipping any delete-marker/unresolved-alias entry (never present
	// in a plain export document, but skipped defensively rather than assumed impossible).
	TMap<FGuid, const FFlowGraphParsedNode*> OldNodesByGuid;
	for (const FFlowGraphParsedNode& Node : OldNodes)
	{
		if (!Node.bIsDeleteMarker && !Node.bIsNewAlias && Node.NodeGuid.IsValid())
		{
			OldNodesByGuid.Add(Node.NodeGuid, &Node);
		}
	}

	TMap<FGuid, const FFlowGraphParsedNode*> NewNodesByGuid;
	for (const FFlowGraphParsedNode& Node : NewNodes)
	{
		if (!Node.bIsDeleteMarker && !Node.bIsNewAlias && Node.NodeGuid.IsValid())
		{
			NewNodesByGuid.Add(Node.NodeGuid, &Node);
		}
	}

	for (const TPair<FGuid, const FFlowGraphParsedNode*>& Pair : NewNodesByGuid)
	{
		if (!OldNodesByGuid.Contains(Pair.Key))
		{
			OutDiff.AddedNodeGuids.Add(Pair.Key);
		}
	}

	for (const TPair<FGuid, const FFlowGraphParsedNode*>& Pair : OldNodesByGuid)
	{
		if (!NewNodesByGuid.Contains(Pair.Key))
		{
			OutDiff.RemovedNodeGuids.Add(Pair.Key);
		}
	}

	for (const TPair<FGuid, const FFlowGraphParsedNode*>& OldPair : OldNodesByGuid)
	{
		const FFlowGraphParsedNode** NewNodePtr = NewNodesByGuid.Find(OldPair.Key);
		if (!NewNodePtr)
		{
			continue;
		}

		const FFlowGraphParsedNode& OldNode = *OldPair.Value;
		const FFlowGraphParsedNode& NewNode = **NewNodePtr;

		FFlowGraphDiffChangedNode ChangedNode;
		ChangedNode.NodeGuid = OldPair.Key;

		// Union of both documents' property keys - a property present in only one document is a
		// change too (added or removed), not just a value swap.
		CollectPropertyDelta(OldNode.Properties, NewNode.Properties, ChangedNode.ChangedProperties);
		if (OldNode.NodeType != NewNode.NodeType)
		{
			ChangedNode.ChangedProperties.Add(TEXT("$class"),
				FString::Printf(TEXT("%s -> %s"), *OldNode.NodeType, *NewNode.NodeType));
		}

		if (OldNode.bHasPos && NewNode.bHasPos && OldNode.Pos != NewNode.Pos)
		{
			ChangedNode.bPosChanged = true;
			ChangedNode.OldPos = OldNode.Pos;
			ChangedNode.NewPos = NewNode.Pos;
		}

		DiffAddOnsRecursive(OldNode.AddOns, NewNode.AddOns,
			ChangedNode.AddedAddOns, ChangedNode.RemovedAddOns, ChangedNode.ChangedAddOns);

		if (!ChangedNode.ChangedProperties.IsEmpty() || ChangedNode.bPosChanged || ChangedNode.HasAddOnChange())
		{
			OutDiff.ChangedNodes.Add(MoveTemp(ChangedNode));
		}
	}

	// Connection diff - set-membership by stable key, order-independent.
	TMap<FString, FFlowGraphParsedConnection> OldConnectionsByKey;
	for (const FFlowGraphParsedConnection& Connection : OldConnections)
	{
		if (!Connection.bIsDeleteMarker && Connection.SourceNodeGuid.IsValid() && Connection.TargetNodeGuid.IsValid())
		{
			OldConnectionsByKey.Add(MakeConnectionKey(Connection), Connection);
		}
	}

	TMap<FString, FFlowGraphParsedConnection> NewConnectionsByKey;
	for (const FFlowGraphParsedConnection& Connection : NewConnections)
	{
		if (!Connection.bIsDeleteMarker && Connection.SourceNodeGuid.IsValid() && Connection.TargetNodeGuid.IsValid())
		{
			NewConnectionsByKey.Add(MakeConnectionKey(Connection), Connection);
		}
	}

	const FExecPinLookup ExecLookup = BuildExecPinLookup(OldNodes, NewNodes);

	for (const TPair<FString, FFlowGraphParsedConnection>& Pair : NewConnectionsByKey)
	{
		if (!OldConnectionsByKey.Contains(Pair.Key))
		{
			OutDiff.AddedConnections.Add(ToDiffConnection(Pair.Value, ExecLookup));
		}
	}

	for (const TPair<FString, FFlowGraphParsedConnection>& Pair : OldConnectionsByKey)
	{
		if (!NewConnectionsByKey.Contains(Pair.Key))
		{
			OutDiff.RemovedConnections.Add(ToDiffConnection(Pair.Value, ExecLookup));
		}
	}

	return true;
}
