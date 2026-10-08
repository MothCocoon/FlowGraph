// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphReconciler.h"
#include "FlowGraphImporter.h"
#include "FlowCourierConverter.h"
#include "FlowCourierDocument.h"
#include "FlowGraphRegrapher.h"
#include "FlowGraphLayout.h"
#include "FlowGraphExporter.h"
#include "FlowLogChannels.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/FlowPin.h"
#include "AddOns/FlowNodeAddOn.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Editor.h"
#include "ScopedTransaction.h"

// Builds the set of node GUIDs that will exist after the plan applies:
// existing nodes + adds - deletes (including scope-driven implicit deletes).
static TSet<FGuid> BuildPostPlanNodeSet(
	const TMap<FGuid, UFlowNode*>& CurrentNodes,
	const TArray<FFlowGraphParsedNode>& NodesToAdd,
	const TArray<FGuid>& NodesToDelete)
{
	TSet<FGuid> PostPlanGuids;
	PostPlanGuids.Reserve(CurrentNodes.Num());
	for (const TPair<FGuid, UFlowNode*>& Pair : CurrentNodes)
	{
		PostPlanGuids.Add(Pair.Key);
	}

	for (const FFlowGraphParsedNode& NodeToAdd : NodesToAdd)
	{
		PostPlanGuids.Add(NodeToAdd.NodeGuid);
	}

	for (const FGuid& DeletedGuid : NodesToDelete)
	{
		PostPlanGuids.Remove(DeletedGuid);
	}

	return PostPlanGuids;
}

// The Courier converter resolves every alias and GUID in the complete document before producing
// parsed connections. Reconciliation consumes resolved endpoints only.

// Appends every Courier converter issue (JSON parse/structural problems) as a validation finding,
// so a malformed document is reported the same way a semantic validation problem is - as data in
// ValidationFindings, never by raising. Code carries across directly: both types share one code set.
// Path is folded into Message instead, because FFlowValidationFinding deliberately has no path field -
// it addresses graph space (node/addon/pin), while a converter issue addresses document space, and a
// caller that wants the JSON pointer for a parse-time issue should read FFlowCourierIssue itself.
static void AppendCourierIssuesAsFindings(const TArray<FFlowCourierIssue>& Issues, TArray<FFlowValidationFinding>& OutFindings)
{
	for (const FFlowCourierIssue& Issue : Issues)
	{
		FFlowValidationFinding& Finding = OutFindings.AddDefaulted_GetRef();
		Finding.Severity = Issue.Severity;
		Finding.Code = Issue.Code;
		Finding.Message = FString::Printf(TEXT("[%s] %s: %s"), *Issue.Code, *Issue.Path, *Issue.Message);
	}
}

// Resolves an addon type string to a concrete UFlowNodeAddOn subclass. Delegates to the shared
// UFlowGraphImporter::ResolveNodeClass so class resolution stays byte-for-byte identical to what
// FFlowGraphValidation checked pre-commit - a divergent resolver here (e.g. one that only
// FindObject's the short-name form, or lacks the as-is last-resort lookup) could let a plan pass
// validation and then fail to resolve the same class at apply time, or vice versa. This matters
// most for project-specific/Blueprint/AngelScript addon type strings, exactly where the fallbacks differ.
static UClass* ResolveAddOnClass(const FString& AddOnType, FString& OutErrorMessage)
{
	UClass* AddOnClass = UFlowGraphImporter::ResolveNodeClass(AddOnType);
	if (!AddOnClass || !AddOnClass->IsChildOf(UFlowNodeAddOn::StaticClass()))
	{
		OutErrorMessage = FString::Printf(TEXT("ReconcileNodeAddOns: Could not find AddOn class: %s"), *AddOnType);
		return nullptr;
	}

	return AddOnClass;
}

// Returns true when SourcePinName is an exec output pin on SourceNode. Data connections are keyed
// on the target node's input pin; exec connections are keyed on the source node's output pin -
// querying the wrong side breaks idempotency detection for data pins. Checks SourceNode's declared
// output pins (including dynamically-added ones, not just CDO-static ones). Defaults to true
// (exec assumption) when SourceNode is null or the pin is not found - both are cases where
// bAlreadyPresent is correctly always false anyway, since a non-existent node has no connections.
static bool IsExecConnectionSourcePin(const UFlowNode* SourceNode, FName SourcePinName)
{
	if (SourceNode)
	{
		for (const FFlowPin& Pin : SourceNode->GetOutputPins())
		{
			if (Pin.PinName == SourcePinName)
			{
				return Pin.IsExecPin();
			}
		}
	}

	return true;
}

// Full mode only: an existing connection not re-declared by the document is implicitly removed,
// mirroring node deletion. Enumerates every connection exactly once - exec connections from the
// source node's output pin, data connections from the target node's input pin - matching the keying
// UFlowGraphExporter uses. Connections involving nodes that will be deleted are included so the
// plan reports the complete set of graph changes before node cleanup runs.
static void CollectExistingConnections(
	const TMap<FGuid, UFlowNode*>& CurrentNodes,
	TArray<FFlowGraphParsedConnection>& OutConnections)
{
	for (const TPair<FGuid, UFlowNode*>& NodePair : CurrentNodes)
	{
		UFlowNode* Node = NodePair.Value;
		if (!Node)
		{
			continue;
		}

		for (const FFlowPin& OutputPin : Node->GetOutputPins())
		{
			if (!OutputPin.IsExecPin())
			{
				continue;
			}

			const FConnectedPin Connection = Node->GetConnection(OutputPin.PinName);
			if (Connection.NodeGuid.IsValid())
			{
				FFlowGraphParsedConnection& Parsed = OutConnections.AddDefaulted_GetRef();
				Parsed.SourceNodeGuid = NodePair.Key;
				Parsed.SourcePinName = OutputPin.PinName;
				Parsed.TargetNodeGuid = Connection.NodeGuid;
				Parsed.TargetPinName = Connection.PinName;
			}
		}

		for (const FFlowPin& InputPin : Node->GetInputPins())
		{
			if (InputPin.IsExecPin())
			{
				continue;
			}

			const FConnectedPin Connection = Node->GetConnection(InputPin.PinName);
			if (Connection.NodeGuid.IsValid())
			{
				FFlowGraphParsedConnection& Parsed = OutConnections.AddDefaulted_GetRef();
				Parsed.SourceNodeGuid = Connection.NodeGuid;
				Parsed.SourcePinName = Connection.PinName;
				Parsed.TargetNodeGuid = NodePair.Key;
				Parsed.TargetPinName = InputPin.PinName;
			}
		}
	}
}

UFlowAsset* FFlowGraphReconciler::FindOrLoadFlowAsset(const FString& TargetAssetPath)
{
	// Check for an already-in-memory asset first (e.g. one open and unsaved in the editor, or just
	// created this session) - LoadObject alone only resolves packages that exist on disk and would
	// miss it, incorrectly falling through to the create-new path. FindObject needs the
	// fully-qualified dotted object path rather than the bare package path, so resolve the package
	// by name first, then find the object by name within it.
	UFlowAsset* Found = nullptr;
	const FString PackageName = FPackageName::ObjectPathToPackageName(TargetAssetPath);
	if (UPackage* ExistingPackage = FindPackage(nullptr, *PackageName))
	{
		// GetShortName only splits on '/', so it must run on the already-dot-stripped PackageName -
		// running it directly on TargetAssetPath leaves a literal ".ObjectName" suffix when the
		// caller passes a fully-qualified dotted object path (e.g. FTopLevelAssetPath::ToString()).
		Found = FindObject<UFlowAsset>(ExistingPackage, *FPackageName::GetShortName(PackageName));
	}

	if (!Found)
	{
		Found = LoadObject<UFlowAsset>(nullptr, *TargetAssetPath);
	}

	return Found;
}

// new-asset compute: populates plan summary and execution data from an already-converted
// document. There is no existing asset to validate class-allowance/attachment rules against, but
// pin existence and connection endpoints are fully determined by the document itself (every node
// referenced is either in ParsedNodes or the document is invalid), so ValidateDocument still runs
// with a null TargetAsset and the full set of added node GUIDs as PostPlanNodeGuids.
bool FFlowGraphReconciler::ComputePlanForNewAsset(
	const FString& AssetClassPath,
	bool bWorldBound,
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const TArray<FFlowGraphParsedConnection>& ParsedConnections,
	FFlowReconcileExecutionPlan& OutPlan,
	FString& OutErrorMessage)
{
	OutPlan.AllParsedNodes = ParsedNodes;
	OutPlan.AllParsedConnections = ParsedConnections;
	OutPlan.AssetClassPath = AssetClassPath;
	OutPlan.bWorldBound = bWorldBound;

	for (const FFlowGraphParsedNode& ParsedNode : OutPlan.AllParsedNodes)
	{
		if (!ParsedNode.bIsDeleteMarker)
		{
			OutPlan.NodesToAdd.Add(ParsedNode);
			OutPlan.Plan.NodesAdded.Add(ParsedNode.NodeGuid);
		}
	}

	// Every connection in the document is new on an asset that does not exist yet. Reporting them
	// here is what lets a dry-run create say what it would connect; ExecutePlanForNewAsset then
	// replaces this intent with what actually landed. Without it a create reports adding no
	// connections while silently making them - a claim contradicted by the asset itself.
	for (const FFlowGraphParsedConnection& ParsedConnection : OutPlan.AllParsedConnections)
	{
		if (!ParsedConnection.bIsDeleteMarker)
		{
			OutPlan.Plan.ConnectionsAdded.Add(FString::Printf(TEXT("%s.%s -> %s.%s"),
				*ParsedConnection.SourceNodeGuid.ToString(), *ParsedConnection.SourcePinName.ToString(),
				*ParsedConnection.TargetNodeGuid.ToString(), *ParsedConnection.TargetPinName.ToString()));
		}
	}

	OutPlan.NodesTouched = OutPlan.Plan.NodesAdded.Num();
	OutPlan.NodesPreserved = 0;
	OutPlan.bIsNewAsset = true;

	const TSet<FGuid> PostPlanNodeGuids = BuildPostPlanNodeSet(TMap<FGuid, UFlowNode*>(), OutPlan.NodesToAdd, TArray<FGuid>());
	FFlowGraphValidation::ValidateDocument(nullptr, OutPlan.AllParsedNodes, OutPlan.AllParsedConnections, OutPlan.ValidationFindings, &PostPlanNodeGuids);

	return true;
}

// new-asset execute: full import + regraph.
bool FFlowGraphReconciler::ExecutePlanForNewAsset(
	const FFlowReconcileExecutionPlan& ExecutionPlan,
	const FString& TargetAssetPath,
	FFlowReconcileResult& Result,
	FString& OutErrorMessage)
{
	UFlowAsset* CreatedAsset = UFlowGraphImporter::ImportFlowGraphFromDocument(
		TargetAssetPath,
		ExecutionPlan.AssetClassPath,
		ExecutionPlan.bWorldBound,
		ExecutionPlan.AllParsedNodes,
		ExecutionPlan.AllParsedConnections,
		OutErrorMessage);
	if (!CreatedAsset)
	{
		Result.Findings.Add(OutErrorMessage);
		return false;
	}

	// Extract positions from the already-parsed node list (same as ApplyPlanToExistingAsset does)
	// so aliased nodes keep the GUIDs already minted by the converter.
	TMap<FGuid, FIntPoint> NodePositions;
	for (const FFlowGraphParsedNode& ParsedNode : ExecutionPlan.AllParsedNodes)
	{
		if (ParsedNode.bHasPos && !ParsedNode.bIsDeleteMarker)
		{
			NodePositions.Add(ParsedNode.NodeGuid, ParsedNode.Pos);
		}
	}
	if (!UFlowGraphRegrapher::RegraphFlowAsset(CreatedAsset, NodePositions))
	{
		OutErrorMessage = TEXT("Created the asset but failed to regraph it");
		Result.Findings.Add(OutErrorMessage);
		return false;
	}

	// Hold the create path to the same standard as the patch path: report what the asset shows, not
	// what the import was asked to do. ImportFlowGraphFromDocument succeeding is not evidence that
	// every connection took.
	TArray<FFlowGraphParsedConnection> ExpectedConnections;
	ExpectedConnections.Reserve(ExecutionPlan.AllParsedConnections.Num());
	for (const FFlowGraphParsedConnection& ParsedConnection : ExecutionPlan.AllParsedConnections)
	{
		if (!ParsedConnection.bIsDeleteMarker)
		{
			ExpectedConnections.Add(ParsedConnection);
		}
	}

	TArray<FString> VerifiedConnectionsAdded, NotLandedConnections;
	FFlowGraphReconciler::VerifyConnectionsLanded(CreatedAsset, ExpectedConnections, VerifiedConnectionsAdded, NotLandedConnections);
	for (const FString& NotLandedLabel : NotLandedConnections)
	{
		Result.Findings.Add(FString::Printf(TEXT("Requested connection did not land: %s"), *NotLandedLabel));
	}
	Result.Plan.ConnectionsAdded = MoveTemp(VerifiedConnectionsAdded);

	return true;
}

void FFlowGraphReconciler::VerifyConnectionsLanded(
	UFlowAsset* FlowAsset,
	const TArray<FFlowGraphParsedConnection>& ExpectedConnections,
	TArray<FString>& OutVerified,
	TArray<FString>& OutNotLanded)
{
	OutVerified.Reset();
	OutNotLanded.Reset();
	if (!FlowAsset)
	{
		for (const FFlowGraphParsedConnection& ExpectedConnection : ExpectedConnections)
		{
			OutNotLanded.Add(FString::Printf(TEXT("%s.%s -> %s.%s"),
				*ExpectedConnection.SourceNodeGuid.ToString(), *ExpectedConnection.SourcePinName.ToString(),
				*ExpectedConnection.TargetNodeGuid.ToString(), *ExpectedConnection.TargetPinName.ToString()));
		}
		return;
	}

	OutVerified.Reserve(ExpectedConnections.Num());
	for (const FFlowGraphParsedConnection& ExpectedConnection : ExpectedConnections)
	{
		UFlowNode* ExpectedSourceNode = FlowAsset->GetNodes().FindRef(ExpectedConnection.SourceNodeGuid);
		UFlowNode* ExpectedTargetNode = FlowAsset->GetNodes().FindRef(ExpectedConnection.TargetNodeGuid);
		const bool bIsExecPin = IsExecConnectionSourcePin(ExpectedSourceNode, ExpectedConnection.SourcePinName);
		UFlowNode* KeyNode = bIsExecPin ? ExpectedSourceNode : ExpectedTargetNode;
		const FName KeyPinName = bIsExecPin ? ExpectedConnection.SourcePinName : ExpectedConnection.TargetPinName;
		const FGuid ExpectedOtherGuid = bIsExecPin ? ExpectedConnection.TargetNodeGuid : ExpectedConnection.SourceNodeGuid;
		const FName ExpectedOtherPinName = bIsExecPin ? ExpectedConnection.TargetPinName : ExpectedConnection.SourcePinName;

		const FString ConnectionLabel = FString::Printf(TEXT("%s.%s -> %s.%s"),
			*ExpectedConnection.SourceNodeGuid.ToString(), *ExpectedConnection.SourcePinName.ToString(),
			*ExpectedConnection.TargetNodeGuid.ToString(), *ExpectedConnection.TargetPinName.ToString());

		bool bConnectionVerified = false;
		if (KeyNode)
		{
			const FConnectedPin ActualConnection = KeyNode->GetConnection(KeyPinName);
			bConnectionVerified = ActualConnection.NodeGuid == ExpectedOtherGuid
				&& ActualConnection.PinName == ExpectedOtherPinName;
		}

		if (bConnectionVerified)
		{
			OutVerified.Add(ConnectionLabel);
		}
		else
		{
			OutNotLanded.Add(ConnectionLabel);
		}
	}
}

void FFlowGraphReconciler::ClassifyParsedNodes(
	const TArray<FFlowGraphParsedNode>& ParsedNodes,
	const TArray<FGuid>& ScopedNodeGuids,
	const TMap<FGuid, UFlowNode*>& CurrentNodes,
	TArray<FFlowGraphParsedNode>& OutNodesToAdd,
	TArray<FFlowGraphParsedNode>& OutNodesToUpdate,
	TArray<FGuid>& OutNodesToDelete,
	TMap<FString, FGuid>& OutAliasMap)
{
	for (const FFlowGraphParsedNode& ParsedNode : ParsedNodes)
	{
		if (ParsedNode.bIsDeleteMarker)
		{
			if (CurrentNodes.Contains(ParsedNode.NodeGuid))
			{
				OutNodesToDelete.Add(ParsedNode.NodeGuid);
			}
			continue;
		}

		if (ParsedNode.bIsNewAlias || !CurrentNodes.Contains(ParsedNode.NodeGuid))
		{
			OutNodesToAdd.Add(ParsedNode);
			if (ParsedNode.bIsNewAlias && !ParsedNode.Alias.IsEmpty())
			{
				OutAliasMap.Add(ParsedNode.Alias, ParsedNode.NodeGuid);
			}
		}
		else
		{
			OutNodesToUpdate.Add(ParsedNode);
		}
	}

	// A node named in scope but absent from this document's [NODES] section is treated as an
	// implicit delete, as if "-Node: <Guid>" had been written. A node not named in scope is
	// never affected by this loop - unscoped omission means "untouched".
	// Build the upsert set once so the per-scope-entry lookup is O(1), not O(ParsedNodes).
	TSet<FGuid> UpsertedGuids;
	UpsertedGuids.Reserve(ParsedNodes.Num());
	for (const FFlowGraphParsedNode& ParsedNode : ParsedNodes)
	{
		if (!ParsedNode.bIsDeleteMarker)
		{
			UpsertedGuids.Add(ParsedNode.NodeGuid);
		}
	}

	for (const FGuid& ScopedGuid : ScopedNodeGuids)
	{
		if (!CurrentNodes.Contains(ScopedGuid) || OutNodesToDelete.Contains(ScopedGuid))
		{
			continue;
		}

		if (!UpsertedGuids.Contains(ScopedGuid))
		{
			OutNodesToDelete.Add(ScopedGuid);
		}
	}
}

void FFlowGraphReconciler::ClassifyParsedConnections(
	const TArray<FFlowGraphParsedConnection>& ParsedConnections,
	const TMap<FGuid, UFlowNode*>& CurrentNodes,
	TArray<FFlowGraphParsedConnection>& OutConnectionsToAdd,
	TArray<FFlowGraphParsedConnection>& OutConnectionsToRemove,
	FFlowReconcilePlan& OutPlan)
{
	// Distinguish genuinely-new/removable connections from ones already in the requested state,
	// so both add and delete are idempotent and only real changes appear in the plan.
	for (const FFlowGraphParsedConnection& ParsedConnection : ParsedConnections)
	{
		UFlowNode* SourceNode = CurrentNodes.FindRef(ParsedConnection.SourceNodeGuid);
		UFlowNode* TargetNode = CurrentNodes.FindRef(ParsedConnection.TargetNodeGuid);
		const bool bIsExecPin = IsExecConnectionSourcePin(SourceNode, ParsedConnection.SourcePinName);

		// Exec connections key on SourceNode+SourcePinName; data connections key on
		// TargetNode+TargetPinName (see UFlowGraphImporter::WriteConnectionToNode).
		UFlowNode* KeyNode = bIsExecPin ? SourceNode : TargetNode;
		const FName KeyPinName = bIsExecPin ? ParsedConnection.SourcePinName : ParsedConnection.TargetPinName;
		const FGuid ExpectedOtherGuid = bIsExecPin ? ParsedConnection.TargetNodeGuid : ParsedConnection.SourceNodeGuid;
		const FName ExpectedOtherPinName = bIsExecPin ? ParsedConnection.TargetPinName : ParsedConnection.SourcePinName;

		FConnectedPin ExistingConnection;
		bool bAlreadyPresent = false;
		if (KeyNode)
		{
			ExistingConnection = KeyNode->GetConnection(KeyPinName);
			bAlreadyPresent = ExistingConnection.NodeGuid == ExpectedOtherGuid && ExistingConnection.PinName == ExpectedOtherPinName;
		}

		const FString ConnectionLabel = FString::Printf(TEXT("%s.%s -> %s.%s"),
			*ParsedConnection.SourceNodeGuid.ToString(), *ParsedConnection.SourcePinName.ToString(),
			*ParsedConnection.TargetNodeGuid.ToString(), *ParsedConnection.TargetPinName.ToString());

		if (ParsedConnection.bIsDeleteMarker)
		{
			if (bAlreadyPresent)
			{
				OutConnectionsToRemove.Add(ParsedConnection);
				OutPlan.ConnectionsRemoved.Add(ConnectionLabel);
			}

			continue;
		}

		if (!bAlreadyPresent)
		{
			OutConnectionsToAdd.Add(ParsedConnection);
			OutPlan.ConnectionsAdded.Add(ConnectionLabel);

			// A pin has at most one connection at runtime - adding a connection to a pin that
			// already points elsewhere implicitly retargets it. Report the superseded connection
			// as removed so the plan reflects what actually changed.
			if (ExistingConnection.NodeGuid.IsValid())
			{
				const FString SupersededLabel = bIsExecPin
					? FString::Printf(TEXT("%s.%s -> %s.%s"), *ParsedConnection.SourceNodeGuid.ToString(), *ParsedConnection.SourcePinName.ToString(), *ExistingConnection.NodeGuid.ToString(), *ExistingConnection.PinName.ToString())
					: FString::Printf(TEXT("%s.%s -> %s.%s"), *ExistingConnection.NodeGuid.ToString(), *ExistingConnection.PinName.ToString(), *ParsedConnection.TargetNodeGuid.ToString(), *ParsedConnection.TargetPinName.ToString());
				OutPlan.ConnectionsRemoved.Add(SupersededLabel);
			}
		}
	}
}

bool FFlowGraphReconciler::ApplyPlanToExistingAsset(
	UFlowAsset* ExistingAsset,
	const FFlowReconcileExecutionPlan& ExecutionPlan,
	FFlowReconcileResult& Result,
	FString& OutErrorMessage)
{
	FScopedTransaction Transaction(NSLOCTEXT("FlowGraphCourier", "ReconcileFlowGraph", "Reconcile Flow Graph"));

	FString ApplyError;

	// Apply order follows spec section 6's fixed kind order: UpsertNode, UpsertAddon,
	// AddConnection, RemoveConnection, DeleteAddon, DeleteNode. DeleteAddon is folded into the
	// UpsertAddon step below (ReconcileNodeAddOns reconciles a node's whole addon set - add,
	// update, and delete - in one pass), which does not create a cross-node hazard since addon
	// deletes only ever affect their own owning node. DeleteNode runs last so a document may
	// delete a node it also connected or addon'd earlier without ordering hazards;
	// RemoveNodeAndReferencingConnections also cleans up any connections still pointing at it.

	// 1. UpsertNode: create new nodes, then apply properties/pins to existing nodes.
	TMap<FGuid, UFlowNode*> NewNodeMap;
	if (!ExecutionPlan.NodesToAdd.IsEmpty() && !UFlowGraphImporter::CreateFlowNodes(ExistingAsset, ExecutionPlan.NodesToAdd, NewNodeMap, ApplyError))
	{
		OutErrorMessage = ApplyError;
		Result.Findings.Add(ApplyError);
		return false;
	}

	for (const FFlowGraphParsedNode& NodeToUpdate : ExecutionPlan.NodesToUpdate)
	{
		UFlowNode* ExistingNode = ExistingAsset->GetNodes().FindRef(NodeToUpdate.NodeGuid);
		if (!ExistingNode)
		{
			continue;
		}

		if (!UFlowGraphImporter::SetNodeProperties(ExistingNode, NodeToUpdate.Properties, ApplyError))
		{
			OutErrorMessage = ApplyError;
			Result.Findings.Add(ApplyError);
			return false;
		}

#if WITH_EDITOR
		// A patch that changes a descriptor property has to regenerate the pins derived from it, or the
		// node keeps the pin set the old value produced. Same call CreateFlowNodes makes for a new node,
		// and same ordering: after the properties it reads, before ApplyDeclaredPins fills in the rest.
		ExistingNode->TryUpdateAutoDataPins();
#endif

		// Mirrors CreateFlowNodes' call for new nodes: an updated node's declared InputPins/OutputPins
		// (e.g. a dynamic pin re-declared on an existing node) must exist before SetupConnections runs
		// below, or ResolveSourcePin/EnsureTargetDataPinExists fall back to mistyped wildcard stand-ins.
		UFlowGraphImporter::ApplyDeclaredPins(ExistingNode, NodeToUpdate);
	}

	// Re-fetch after node adds so the addon and connection steps below can resolve newly-created
	// nodes alongside pre-existing ones.
	TMap<FGuid, UFlowNode*> CombinedNodeMap = ExistingAsset->GetNodes();

	// 2. UpsertAddon (and DeleteAddon, folded in - see note above): new nodes' addons, then
	// existing nodes' addon reconciliation.
	if (!ExecutionPlan.NodesToAdd.IsEmpty() && !UFlowGraphImporter::CreateNodeAddOns(CombinedNodeMap, ExecutionPlan.NodesToAdd, ApplyError))
	{
		OutErrorMessage = ApplyError;
		Result.Findings.Add(ApplyError);
		return false;
	}

	for (const FFlowGraphParsedNode& NodeToUpdate : ExecutionPlan.NodesToUpdate)
	{
		UFlowNode* ExistingNode = ExistingAsset->GetNodes().FindRef(NodeToUpdate.NodeGuid);
		if (!ExistingNode)
		{
			continue;
		}

		if ((!NodeToUpdate.AddOns.IsEmpty() || NodeToUpdate.bIsFullBlock)
			&& !ReconcileNodeAddOns(ExistingNode, NodeToUpdate.NodeGuid, NodeToUpdate.AddOns, Result.Plan.AddOnsAdded, Result.Plan.AddOnsUpdated, Result.Plan.AddOnsDeleted, ApplyError, NodeToUpdate.bIsFullBlock))
		{
			OutErrorMessage = ApplyError;
			Result.Findings.Add(ApplyError);
			return false;
		}
	}

#if WITH_EDITOR
	// Auto data pins depend on a node's addons, so step 2 above (addon reconcile) can change what a
	// node's pin set should be. Regenerate before connections are wired below, so a pin an addon just
	// contributed is there to connect to. Covers new nodes too, whose initial auto pins in step 1
	// predate CreateNodeAddOns.
	for (const TPair<FGuid, UFlowNode*>& NewNodeEntry : NewNodeMap)
	{
		if (UFlowNode* NewNode = NewNodeEntry.Value)
		{
			NewNode->TryUpdateAutoDataPins();
		}
	}
	for (const FFlowGraphParsedNode& NodeToUpdate : ExecutionPlan.NodesToUpdate)
	{
		if (NodeToUpdate.AddOns.IsEmpty() && !NodeToUpdate.bIsFullBlock)
		{
			continue;
		}
		if (UFlowNode* ExistingNode = ExistingAsset->GetNodes().FindRef(NodeToUpdate.NodeGuid))
		{
			ExistingNode->TryUpdateAutoDataPins();
		}
	}
#endif

	// 3. AddConnection
	if (!ExecutionPlan.ConnectionsToAdd.IsEmpty() && !UFlowGraphImporter::SetupConnections(ExistingAsset, CombinedNodeMap, ExecutionPlan.ConnectionsToAdd, ExecutionPlan.AllParsedNodes, ApplyError))
	{
		OutErrorMessage = ApplyError;
		Result.Findings.Add(ApplyError);
		return false;
	}

	// 4. RemoveConnection
	for (const FFlowGraphParsedConnection& ConnectionToRemove : ExecutionPlan.ConnectionsToRemove)
	{
		UFlowNode* RemoveSourceNode = ExistingAsset->GetNodes().FindRef(ConnectionToRemove.SourceNodeGuid);
		UFlowNode* RemoveTargetNode = ExistingAsset->GetNodes().FindRef(ConnectionToRemove.TargetNodeGuid);
		const bool bRemoveIsExecPin = IsExecConnectionSourcePin(RemoveSourceNode, ConnectionToRemove.SourcePinName);

		// RemoveConnectionFromSourceNode clears whichever node's Connections map entry actually
		// holds this connection - for a data connection that's the target node (see the keying
		// comment on IsExecConnectionSourcePin above), despite the function's name.
		if (UFlowNode* KeyNodeToClear = bRemoveIsExecPin ? RemoveSourceNode : RemoveTargetNode)
		{
			const FName KeyPinToClear = bRemoveIsExecPin ? ConnectionToRemove.SourcePinName : ConnectionToRemove.TargetPinName;
			RemoveConnectionFromSourceNode(KeyNodeToClear, KeyPinToClear);
		}
	}

	// 6. DeleteNode - last (5, DeleteAddon, is folded into step 2 above).
	for (const FGuid& NodeGuidToDelete : ExecutionPlan.NodesToDelete)
	{
		if (!RemoveNodeAndReferencingConnections(ExistingAsset, NodeGuidToDelete))
		{
			OutErrorMessage = FString::Printf(TEXT("Failed to delete node %s"), *NodeGuidToDelete.ToString());
			Result.Findings.Add(OutErrorMessage);
			return false;
		}
	}

	if (UPackage* Package = ExistingAsset->GetOutermost())
	{
		Package->MarkPackageDirty();
	}

	// Extract positions from the already-parsed node list rather than re-calling ParseNodePositions,
	// which would mint fresh GUIDs for new:<alias> nodes - different from those already in
	// NodesToAdd/AliasMap, silently dropping any explicit Pos: on an aliased node.
	TMap<FGuid, FIntPoint> NodePositions;
	for (const FFlowGraphParsedNode& ParsedNode : ExecutionPlan.AllParsedNodes)
	{
		if (ParsedNode.bHasPos && !ParsedNode.bIsDeleteMarker)
		{
			NodePositions.Add(ParsedNode.NodeGuid, ParsedNode.Pos);
		}
	}

	// Auto-place new nodes that had no explicit Pos: line. Merges computed positions into
	// NodePositions before regraphing so RegraphFlowAsset applies them the same way.
	if (!ExecutionPlan.NodesToAdd.IsEmpty())
	{
		TArray<FGuid> NewNodeGuidsForLayout;
		NewNodeGuidsForLayout.Reserve(ExecutionPlan.NodesToAdd.Num());
		for (const FFlowGraphParsedNode& AddedNode : ExecutionPlan.NodesToAdd)
		{
			NewNodeGuidsForLayout.Add(AddedNode.NodeGuid);
		}
		FFlowGraphLayout::ComputeAutoPlacedPositions(ExistingAsset, NewNodeGuidsForLayout, ExecutionPlan.AllParsedConnections, NodePositions);
	}

	TMap<FGuid, FString> NodeComments;
	for (const FFlowGraphParsedNode& ParsedNode : ExecutionPlan.AllParsedNodes)
	{
		if (ParsedNode.bHasComment && !ParsedNode.bIsDeleteMarker)
		{
			NodeComments.Add(ParsedNode.NodeGuid, ParsedNode.NodeComment);
		}
	}

	if (!UFlowGraphRegrapher::RegraphFlowAsset(ExistingAsset, NodePositions, NodeComments))
	{
		OutErrorMessage = TEXT("Applied the mutation but failed to regraph the asset");
		Result.Findings.Add(OutErrorMessage);
		return false;
	}

	// Verify only after every destructive phase and the final regraph. Earlier verification can
	// observe a connection that a later remove or delete phase legitimately removes.
	TArray<FString> VerifiedConnectionsAdded;
	TArray<FString> NotLandedConnections;
	FFlowGraphReconciler::VerifyConnectionsLanded(
		ExistingAsset,
		ExecutionPlan.ConnectionsToAdd,
		VerifiedConnectionsAdded,
		NotLandedConnections);
	for (const FString& NotLandedLabel : NotLandedConnections)
	{
		Result.Findings.Add(FString::Printf(TEXT("Requested connection did not land: %s"), *NotLandedLabel));
	}
	Result.Plan.ConnectionsAdded = MoveTemp(VerifiedConnectionsAdded);

	TArray<FString> GraphIntegrityIssues;
	UFlowGraphRegrapher::CollectGraphParityIssues(ExistingAsset, GraphIntegrityIssues);
	for (const FString& GraphIntegrityIssue : GraphIntegrityIssues)
	{
		Result.Findings.Add(GraphIntegrityIssue);
	}
	if (!NotLandedConnections.IsEmpty() || !GraphIntegrityIssues.IsEmpty())
	{
		OutErrorMessage = TEXT("Final Flow graph postcondition verification failed");
		return false;
	}
	return true;
}

TSharedPtr<FFlowReconcileExecutionPlan> FFlowGraphReconciler::ComputeReconcilePlan(
	const FString& TargetAssetPath,
	const FString& CourierDocumentJson,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	if (CourierDocumentJson.IsEmpty())
	{
		OutErrorMessage = TEXT("Courier document is empty");
		return nullptr;
	}

	TSharedPtr<FFlowReconcileExecutionPlan> Plan = MakeShared<FFlowReconcileExecutionPlan>();
	Plan->InputSizeBytes = CourierDocumentJson.Len();

	// Hard failure only for a document too malformed to reason about at all (bad JSON, wrong
	// formatVersion). Every other problem the converter finds (identity ambiguity, bad GUIDs,
	// unresolved aliases, field legality) is appended to ValidationFindings below and handled the
	// same way a semantic FFlowGraphValidation finding is - as data, never by failing this call.
	FFlowCourierDocument Document;
	TArray<FFlowCourierIssue> CourierIssues;
	if (!FFlowCourierConverter::ParseDocument(CourierDocumentJson, Document, CourierIssues, OutErrorMessage))
	{
		return nullptr;
	}

	TArray<FFlowGraphParsedNode> ParsedNodes;
	TArray<FFlowGraphParsedConnection> ParsedConnections;
	TArray<FGuid> ScopedNodeGuids;
	TMap<FString, FGuid> AliasMap;
	FFlowCourierConverter::ConvertToParsedGraph(Document, ParsedNodes, ParsedConnections, ScopedNodeGuids, AliasMap, CourierIssues);

	AppendCourierIssuesAsFindings(CourierIssues, Plan->ValidationFindings);

	UFlowAsset* ExistingAsset = FindOrLoadFlowAsset(TargetAssetPath);

	// No asset yet: all nodes are adds, no validation possible.
	if (!ExistingAsset)
	{
		if (!ComputePlanForNewAsset(Document.AssetClass, Document.bWorldBound, ParsedNodes, ParsedConnections, *Plan, OutErrorMessage))
		{
			return nullptr;
		}
		Plan->AliasMap = AliasMap;
		return Plan;
	}

	// Asset exists: classify, validate. Aliasing/GUID resolution already happened in ConvertToParsedGraph.
	Plan->AllParsedNodes = ParsedNodes;
	Plan->AllParsedConnections = ParsedConnections;
	Plan->AssetClassPath = Document.AssetClass;
	Plan->bWorldBound = Document.bWorldBound;

	// Spec section 4: on an existing asset, assetClass must match the real class or it is an
	// error. An empty assetClass is not a mismatch - it means the document is silent on class,
	// which is legal on a patch against an asset that already exists.
	if (!Document.AssetClass.IsEmpty() && !Document.AssetClass.Equals(ExistingAsset->GetClass()->GetPathName(), ESearchCase::CaseSensitive))
	{
		FFlowValidationFinding& Finding = Plan->ValidationFindings.AddDefaulted_GetRef();
		Finding.Severity = EFlowValidationSeverity::Error;
		Finding.Code = TEXT("ClassMismatch");
		Finding.Message = FString::Printf(TEXT("[ClassMismatch] document assetClass '%s' does not match existing asset class '%s'"), *Document.AssetClass, *ExistingAsset->GetClass()->GetPathName());
	}

	const TMap<FGuid, UFlowNode*>& CurrentNodes = ExistingAsset->GetNodes();
	const int32 TotalExistingBeforeApply = CurrentNodes.Num();

	// Full mode is authoritative over the whole asset: the effective scope is every existing node,
	// not just ScopedNodeGuids (which the converter forces empty in Full mode - ScopeWithFullMode
	// is a validation error otherwise). Patch mode keeps the document-declared scope unchanged.
	TArray<FGuid> EffectiveScopedNodeGuids = ScopedNodeGuids;
	if (Document.Mode == EFlowCourierMode::Full)
	{
		EffectiveScopedNodeGuids.Reset();
		CurrentNodes.GenerateKeyArray(EffectiveScopedNodeGuids);
	}

	ClassifyParsedNodes(Plan->AllParsedNodes, EffectiveScopedNodeGuids, CurrentNodes,
		Plan->NodesToAdd, Plan->NodesToUpdate, Plan->NodesToDelete, Plan->AliasMap);

	// The converter's alias map is authoritative because it covers both node and addon aliases.
	// ClassifyParsedNodes is a node-only helper retained from v1 and doesn't track addon aliases.
	Plan->AliasMap = AliasMap;

	Plan->Plan.NodesDeleted = Plan->NodesToDelete;
	for (const FFlowGraphParsedNode& NodeToAdd : Plan->NodesToAdd)
	{
		Plan->Plan.NodesAdded.Add(NodeToAdd.NodeGuid);
	}
	for (const FFlowGraphParsedNode& NodeToUpdate : Plan->NodesToUpdate)
	{
		Plan->Plan.NodesUpdated.Add(NodeToUpdate.NodeGuid);
	}

	// Full mode also implicitly removes any existing connection the document doesn't re-declare.
	// Appended as synthetic delete markers so ClassifyParsedConnections' existing idempotency
	// check (only report a removal if the connection is actually present) applies unchanged.
	if (Document.Mode == EFlowCourierMode::Full)
	{
		TArray<FFlowGraphParsedConnection> ExistingConnections;
		CollectExistingConnections(CurrentNodes, ExistingConnections);

		TSet<FString> DeclaredConnectionKeys;
		DeclaredConnectionKeys.Reserve(Plan->AllParsedConnections.Num());
		for (const FFlowGraphParsedConnection& ParsedConnection : Plan->AllParsedConnections)
		{
			if (!ParsedConnection.bIsDeleteMarker)
			{
				DeclaredConnectionKeys.Add(FString::Printf(TEXT("%s.%s -> %s.%s"),
					*ParsedConnection.SourceNodeGuid.ToString(), *ParsedConnection.SourcePinName.ToString(),
					*ParsedConnection.TargetNodeGuid.ToString(), *ParsedConnection.TargetPinName.ToString()));
			}
		}

		for (const FFlowGraphParsedConnection& ExistingConnection : ExistingConnections)
		{
			const FString Key = FString::Printf(TEXT("%s.%s -> %s.%s"),
				*ExistingConnection.SourceNodeGuid.ToString(), *ExistingConnection.SourcePinName.ToString(),
				*ExistingConnection.TargetNodeGuid.ToString(), *ExistingConnection.TargetPinName.ToString());
			if (!DeclaredConnectionKeys.Contains(Key))
			{
				FFlowGraphParsedConnection& DeleteMarker = Plan->AllParsedConnections.Add_GetRef(ExistingConnection);
				DeleteMarker.bIsDeleteMarker = true;
			}
		}
	}

	ClassifyParsedConnections(Plan->AllParsedConnections, CurrentNodes,
		Plan->ConnectionsToAdd, Plan->ConnectionsToRemove, Plan->Plan);

	Plan->NodesTouched = Plan->NodesToAdd.Num() + Plan->NodesToUpdate.Num() + Plan->NodesToDelete.Num();
	Plan->NodesPreserved = TotalExistingBeforeApply - (Plan->NodesToUpdate.Num() + Plan->NodesToDelete.Num());

	// Pre-commit validation on the fully-resolved document.
	const TSet<FGuid> PostPlanNodeGuids = BuildPostPlanNodeSet(CurrentNodes, Plan->NodesToAdd, Plan->NodesToDelete);
	FFlowGraphValidation::ValidateDocument(ExistingAsset, Plan->AllParsedNodes, Plan->AllParsedConnections, Plan->ValidationFindings, &PostPlanNodeGuids);

	return Plan;
}

FFlowReconcileResult FFlowGraphReconciler::ExecuteReconcilePlan(
	const FFlowReconcileExecutionPlan& ExecutionPlan,
	const FString& TargetAssetPath,
	FString& OutErrorMessage)
{
	FFlowReconcileResult Result;
	OutErrorMessage.Empty();

	Result.Plan = ExecutionPlan.Plan;
	Result.AliasMap = ExecutionPlan.AliasMap;
	Result.ValidationFindings = ExecutionPlan.ValidationFindings;
	Result.NodesTouched = ExecutionPlan.NodesTouched;
	Result.NodesPreserved = ExecutionPlan.NodesPreserved;
	Result.InputSizeBytes = ExecutionPlan.InputSizeBytes;

	// Refuse to apply if validation found any blocking errors.
	const bool bHasBlockingError = ExecutionPlan.ValidationFindings.ContainsByPredicate(
		[](const FFlowValidationFinding& F) { return F.Severity == EFlowValidationSeverity::Error; });

	if (bHasBlockingError)
	{
		OutErrorMessage = TEXT("Validation failed - see ValidationFindings; the asset was not modified");
		Result.Findings.Add(OutErrorMessage);
		return Result;
	}

	if (ExecutionPlan.bIsNewAsset)
	{
		if (ExecutePlanForNewAsset(ExecutionPlan, TargetAssetPath, Result, OutErrorMessage))
		{
			Result.bSuccess = true;
		}
		return Result;
	}

	UFlowAsset* ExistingAsset = FindOrLoadFlowAsset(TargetAssetPath);
	if (!ExistingAsset)
	{
		OutErrorMessage = FString::Printf(TEXT("Asset no longer exists at %s; cannot apply plan"), *TargetAssetPath);
		Result.Findings.Add(OutErrorMessage);
		return Result;
	}

	if (ApplyPlanToExistingAsset(ExistingAsset, ExecutionPlan, Result, OutErrorMessage))
	{
		Result.bSuccess = true;
	}

	return Result;
}

bool FFlowGraphReconciler::RemoveNodeAndReferencingConnections(UFlowAsset* FlowAsset, const FGuid& NodeGuidToRemove)
{
	if (!FlowAsset)
	{
		return false;
	}

	FMapProperty* NodesProperty = FindFProperty<FMapProperty>(UFlowAsset::StaticClass(), TEXT("Nodes"));
	if (!NodesProperty)
	{
		UE_LOG(LogFlow, Error, TEXT("FFlowGraphReconciler::RemoveNodeAndReferencingConnections: Could not find Nodes property on UFlowAsset"));
		return false;
	}

	FStructProperty* KeyProperty = CastField<FStructProperty>(NodesProperty->KeyProp);
	FObjectProperty* ValueProperty = CastField<FObjectProperty>(NodesProperty->ValueProp);
	if (!KeyProperty || !ValueProperty)
	{
		return false;
	}

	void* NodesPtr = NodesProperty->ContainerPtrToValuePtr<void>(FlowAsset);
	FScriptMapHelper NodesMapHelper(NodesProperty, NodesPtr);

	FMapProperty* ConnectionsProperty = FindFProperty<FMapProperty>(UFlowNode::StaticClass(), TEXT("Connections"));
	FStructProperty* ConnectionsValueProperty = ConnectionsProperty ? CastField<FStructProperty>(ConnectionsProperty->ValueProp) : nullptr;

	// Pass 1: find the internal index of the node to remove and drop any connection entries on
	// other nodes that reference it. Collect first, mutate after - FScriptMapHelper is sparse,
	// so removing a captured index after the scan is safe.
	int32 InternalIndexToRemove = INDEX_NONE;

	for (int32 Index = 0; Index < NodesMapHelper.GetMaxIndex(); ++Index)
	{
		if (!NodesMapHelper.IsValidIndex(Index))
		{
			continue;
		}

		uint8* PairPtr = NodesMapHelper.GetPairPtr(Index);
		const FGuid* KeyGuid = reinterpret_cast<const FGuid*>(KeyProperty->ContainerPtrToValuePtr<void>(PairPtr));
		if (KeyGuid && *KeyGuid == NodeGuidToRemove)
		{
			InternalIndexToRemove = Index;
		}

		UFlowNode* Node = Cast<UFlowNode>(ValueProperty->GetObjectPropertyValue(ValueProperty->ContainerPtrToValuePtr<void>(PairPtr)));
		if (!Node || !ConnectionsProperty || !ConnectionsValueProperty)
		{
			continue;
		}

		void* ConnectionsPtr = ConnectionsProperty->ContainerPtrToValuePtr<void>(Node);
		FScriptMapHelper ConnMapHelper(ConnectionsProperty, ConnectionsPtr);

		TArray<int32> ConnIndicesToRemove;
		for (int32 ConnIndex = 0; ConnIndex < ConnMapHelper.GetMaxIndex(); ++ConnIndex)
		{
			if (!ConnMapHelper.IsValidIndex(ConnIndex))
			{
				continue;
			}

			uint8* ConnPairPtr = ConnMapHelper.GetPairPtr(ConnIndex);
			const FConnectedPin* ConnectedPin = reinterpret_cast<const FConnectedPin*>(ConnectionsValueProperty->ContainerPtrToValuePtr<void>(ConnPairPtr));
			if (ConnectedPin && ConnectedPin->NodeGuid == NodeGuidToRemove)
			{
				ConnIndicesToRemove.Add(ConnIndex);
			}
		}

		for (int32 ConnIndex : ConnIndicesToRemove)
		{
			ConnMapHelper.RemoveAt(ConnIndex);
		}

		if (!ConnIndicesToRemove.IsEmpty())
		{
			ConnMapHelper.Rehash();
		}
	}

	if (InternalIndexToRemove == INDEX_NONE)
	{
		return true;
	}

	NodesMapHelper.RemoveAt(InternalIndexToRemove);
	NodesMapHelper.Rehash();

	// A node has two halves and both have to go. Dropping only the runtime map entry leaves the
	// UFlowGraphNode in the editor graph, where it is still drawn, still holds the removed UFlowNode
	// alive in the package, and still carries pin links that the regraph at the end of this apply
	// would harvest straight back into the runtime Connections map - silently undoing this delete.
	UFlowGraphRegrapher::DestroyEditorNodeForRuntimeNode(FlowAsset, NodeGuidToRemove);
	return true;
}

bool FFlowGraphReconciler::RemoveConnectionFromSourceNode(UFlowNode* SourceNode, FName SourcePinName)
{
	if (!SourceNode)
	{
		return false;
	}

	FMapProperty* ConnectionsProperty = FindFProperty<FMapProperty>(UFlowNode::StaticClass(), TEXT("Connections"));
	if (!ConnectionsProperty)
	{
		UE_LOG(LogFlow, Error, TEXT("FFlowGraphReconciler::RemoveConnectionFromSourceNode: Could not find Connections property on UFlowNode"));
		return false;
	}

	void* ConnectionsPtr = ConnectionsProperty->ContainerPtrToValuePtr<void>(SourceNode);
	FScriptMapHelper ConnMapHelper(ConnectionsProperty, ConnectionsPtr);

	const int32 Index = ConnMapHelper.FindMapIndexWithKey(&SourcePinName);
	if (Index == INDEX_NONE)
	{
		return true;
	}

	ConnMapHelper.RemoveAt(Index);
	ConnMapHelper.Rehash();
	return true;
}

bool FFlowGraphReconciler::ReconcileNodeAddOns(UFlowNodeBase* OwnerNode, const FGuid& OwnerNodeGuid, const TArray<FFlowGraphParsedNodeAddOn>& ParsedAddOns,
	TArray<FFlowReconcileAddonEntry>& OutAdded, TArray<FFlowReconcileAddonEntry>& OutUpdated, TArray<FFlowReconcileAddonEntry>& OutDeleted, FString& OutErrorMessage, bool bFullBlock)
{
	if (!OwnerNode)
	{
		OutErrorMessage = TEXT("ReconcileNodeAddOns: OwnerNode is null");
		return false;
	}

	FArrayProperty* AddOnsProperty = FindFProperty<FArrayProperty>(UFlowNodeBase::StaticClass(), TEXT("AddOns"));
	if (!AddOnsProperty)
	{
		OutErrorMessage = TEXT("ReconcileNodeAddOns: Could not find AddOns property on UFlowNodeBase");
		return false;
	}

	FObjectProperty* ElemProp = CastField<FObjectProperty>(AddOnsProperty->Inner);
	if (!ElemProp)
	{
		OutErrorMessage = TEXT("ReconcileNodeAddOns: AddOns array element property is not an object property");
		return false;
	}

	void* AddOnsPtr = AddOnsProperty->ContainerPtrToValuePtr<void>(OwnerNode);

	for (const FFlowGraphParsedNodeAddOn& ParsedAddOn : ParsedAddOns)
	{
		// Re-acquire the helper each iteration - a prior Add/RemoveValues call may have
		// reallocated the underlying array.
		FScriptArrayHelper AddOnsHelper(AddOnsProperty, AddOnsPtr);

		int32 ExistingIndex = INDEX_NONE;
		UFlowNodeAddOn* ExistingAddOn = nullptr;
		for (int32 Index = 0; Index < AddOnsHelper.Num(); ++Index)
		{
			UFlowNodeAddOn* Candidate = Cast<UFlowNodeAddOn>(ElemProp->GetObjectPropertyValue(AddOnsHelper.GetRawPtr(Index)));
			if (Candidate && Candidate->GetGuid() == ParsedAddOn.AddOnGuid)
			{
				ExistingIndex = Index;
				ExistingAddOn = Candidate;
				break;
			}
		}

		if (ParsedAddOn.bIsDeleteMarker)
		{
			if (ExistingIndex != INDEX_NONE)
			{
				FFlowReconcileAddonEntry DeletedEntry;
				DeletedEntry.OwnerNodeGuid = OwnerNodeGuid;
				DeletedEntry.AddOnGuid = ParsedAddOn.AddOnGuid;
				// Type is read from the live object being removed - the delete marker only carries a GUID.
				DeletedEntry.AddOnType = ExistingAddOn ? ExistingAddOn->GetClass()->GetPathName() : FString();

				AddOnsHelper.RemoveValues(ExistingIndex, 1);
				OutDeleted.Add(MoveTemp(DeletedEntry));
			}
			continue;
		}

		if (ExistingAddOn)
		{
			// Preserve identity - update properties in place, same as node-level update.
			if (!UFlowGraphImporter::SetAddOnProperties(ExistingAddOn, ParsedAddOn.Properties, OutErrorMessage))
			{
				return false;
			}
			FFlowReconcileAddonEntry UpdatedEntry;
			UpdatedEntry.OwnerNodeGuid = OwnerNodeGuid;
			UpdatedEntry.AddOnGuid = ParsedAddOn.AddOnGuid;
			UpdatedEntry.AddOnType = ExistingAddOn->GetClass()->GetPathName();
			UpdatedEntry.Properties = ParsedAddOn.Properties;
			OutUpdated.Add(MoveTemp(UpdatedEntry));

			// Recurse into nested children so an update to a deeper addon doesn't require
			// touching every ancestor. The existing addon becomes the owner for its children, so
			// nested entries are keyed by this addon's GUID.
			if (!ParsedAddOn.AddOns.IsEmpty()
				&& !ReconcileNodeAddOns(ExistingAddOn, ParsedAddOn.AddOnGuid, ParsedAddOn.AddOns, OutAdded, OutUpdated, OutDeleted, OutErrorMessage, bFullBlock))
			{
				return false;
			}
			continue;
		}

		// Not present yet - create it, mirroring UFlowGraphImporter::CreateNodeAddOns.
		UClass* AddOnClass = ResolveAddOnClass(ParsedAddOn.AddOnType, OutErrorMessage);
		if (!AddOnClass)
		{
			return false;
		}

		UFlowNodeAddOn* NewAddOn = NewObject<UFlowNodeAddOn>(OwnerNode, AddOnClass, NAME_None, RF_Transactional);
		if (!NewAddOn)
		{
			OutErrorMessage = FString::Printf(TEXT("ReconcileNodeAddOns: Failed to create AddOn of type: %s"), *ParsedAddOn.AddOnType);
			return false;
		}

		const FGuid NewAddOnGuid = ParsedAddOn.AddOnGuid.IsValid() ? ParsedAddOn.AddOnGuid : FGuid::NewGuid();
		NewAddOn->SetGuid(NewAddOnGuid);

		if (!UFlowGraphImporter::SetAddOnProperties(NewAddOn, ParsedAddOn.Properties, OutErrorMessage))
		{
			return false;
		}

		const int32 NewIndex = AddOnsHelper.AddValue();
		ElemProp->SetObjectPropertyValue(AddOnsHelper.GetRawPtr(NewIndex), NewAddOn);

		FFlowReconcileAddonEntry AddedEntry;
		AddedEntry.OwnerNodeGuid = OwnerNodeGuid;
		AddedEntry.AddOnGuid = NewAddOnGuid;
		AddedEntry.AddOnType = AddOnClass->GetPathName();
		AddedEntry.Properties = ParsedAddOn.Properties;
		OutAdded.Add(MoveTemp(AddedEntry));

		// A brand-new addon has no existing children - create its nested subtree wholesale.
		// Only the subtree root is reported into OutAdded, not individual nested children.
		if (!ParsedAddOn.AddOns.IsEmpty() && !UFlowGraphImporter::CreateAddOnsRecursive(NewAddOn, ParsedAddOn.AddOns, OutErrorMessage))
		{
			return false;
		}
	}

	// !full: ParsedAddOns is the complete authoritative list - delete any existing addon with no
	// corresponding entry (added, updated, or explicitly delete-marked all count as matched).
	if (bFullBlock)
	{
		FScriptArrayHelper FullBlockHelper(AddOnsProperty, AddOnsPtr);
		for (int32 Index = FullBlockHelper.Num() - 1; Index >= 0; --Index)
		{
			UFlowNodeAddOn* Candidate = Cast<UFlowNodeAddOn>(ElemProp->GetObjectPropertyValue(FullBlockHelper.GetRawPtr(Index)));
			if (!Candidate)
			{
				continue;
			}

			const bool bMentioned = ParsedAddOns.ContainsByPredicate([&Candidate](const FFlowGraphParsedNodeAddOn& ParsedAddOn)
			{
				return ParsedAddOn.AddOnGuid == Candidate->GetGuid();
			});

			if (!bMentioned)
			{
				FFlowReconcileAddonEntry DeletedEntry;
				DeletedEntry.OwnerNodeGuid = OwnerNodeGuid;
				DeletedEntry.AddOnGuid = Candidate->GetGuid();
				DeletedEntry.AddOnType = Candidate->GetClass()->GetPathName();

				FullBlockHelper.RemoveValues(Index, 1);
				OutDeleted.Add(MoveTemp(DeletedEntry));
			}
		}
	}

	return true;
}

bool FFlowGraphReconciler::NormalizePropertyValueString(FProperty* Property, const FString& RawValue, FString& OutNormalizedValue, FString& OutErrorMessage)
{
	void* ScratchValue = FMemory::Malloc(Property->GetSize(), Property->GetMinAlignment());
	Property->InitializeValue(ScratchValue);

	FString InnerError;
	// No owner on purpose: ScratchValue is raw malloc'd memory, not a live object's property, so
	// there is nothing an instanced subobject could be outered to. Passing an owner here would
	// construct a UObject that this function immediately destroys. A class-plus-fields value
	// therefore fails to normalize, which only affects FindOrCreateNode's match comparison.
	const bool bParsed = UFlowGraphImporter::SetPropertyFromString(Property, ScratchValue, RawValue, InnerError, nullptr);
	if (bParsed)
	{
		OutNormalizedValue = UFlowGraphExporter::GetPropertyValueAsString(Property, ScratchValue);
	}
	else
	{
		OutErrorMessage = FString::Printf(TEXT("could not parse value '%s' for property '%s': %s"), *RawValue, *Property->GetName(), *InnerError);
	}

	Property->DestroyValue(ScratchValue);
	FMemory::Free(ScratchValue);
	return bParsed;
}

bool FFlowGraphReconciler::FindOrCreateNode(
	const FString& TargetAssetPath,
	const FString& NodeType,
	const TMap<FString, FString>& MatchProperties,
	bool bDryRun,
	FGuid& OutNodeGuid,
	bool& OutCreated,
	FString& OutErrorMessage)
{
	OutNodeGuid.Invalidate();
	OutCreated = false;

	UClass* ResolvedClass = UFlowGraphImporter::ResolveNodeClass(NodeType);
	if (!ResolvedClass)
	{
		OutErrorMessage = FString::Printf(TEXT("FindOrCreateNode: could not resolve NodeType: %s"), *NodeType);
		return false;
	}

	// RS13: an unknown property name is a hard error, checked once against the class up front -
	// independent of whether any node instance exists yet to compare against. Each raw value is
	// also normalized here (parse -> re-stringify through the same path the create path uses) so
	// the match comparison below compares canonical forms on both sides - otherwise a caller
	// passing e.g. "123" for a float property would never match a node created from "123.0".
	TArray<FProperty*> MatchFProperties;
	TArray<FString> NormalizedMatchValues;
	MatchFProperties.Reserve(MatchProperties.Num());
	NormalizedMatchValues.Reserve(MatchProperties.Num());
	for (const TPair<FString, FString>& Pair : MatchProperties)
	{
		FProperty* Property = ResolvedClass->FindPropertyByName(FName(*Pair.Key));
		if (!Property)
		{
			OutErrorMessage = FString::Printf(TEXT("FindOrCreateNode: %s has no property named '%s'"), *ResolvedClass->GetName(), *Pair.Key);
			return false;
		}

		FString NormalizedValue;
		FString NormalizeError;
		if (!NormalizePropertyValueString(Property, Pair.Value, NormalizedValue, NormalizeError))
		{
			OutErrorMessage = FString::Printf(TEXT("FindOrCreateNode: %s"), *NormalizeError);
			return false;
		}

		MatchFProperties.Add(Property);
		NormalizedMatchValues.Add(NormalizedValue);
	}

	UFlowAsset* ExistingAsset = FindOrLoadFlowAsset(TargetAssetPath);
	if (!ExistingAsset)
	{
		OutErrorMessage = FString::Printf(TEXT("FindOrCreateNode: no existing asset at %s (this helper does not create the asset itself)"), *TargetAssetPath);
		return false;
	}

	// Exact-class match only (not IsA) - a subclass is a different type as far as this helper is
	// concerned, matching RS13's exact-equality philosophy applied to identity as well as values.
	TArray<FGuid> MatchingGuids;
	for (const TPair<FGuid, UFlowNode*>& NodePair : ExistingAsset->GetNodes())
	{
		UFlowNode* Node = NodePair.Value;
		if (!Node || Node->GetClass() != ResolvedClass)
		{
			continue;
		}

		bool bAllMatch = true;
		for (int32 PropIndex = 0; PropIndex < MatchFProperties.Num(); ++PropIndex)
		{
			FProperty* Property = MatchFProperties[PropIndex];
			const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Node);
			const FString CurrentValue = UFlowGraphExporter::GetPropertyValueAsString(Property, ValuePtr);
			if (CurrentValue != NormalizedMatchValues[PropIndex])
			{
				bAllMatch = false;
				break;
			}
		}

		if (bAllMatch)
		{
			MatchingGuids.Add(NodePair.Key);
		}
	}

	if (!MatchingGuids.IsEmpty())
	{
		// Deterministic tiebreak (RS13): lowest FGuid::ToString() wins, same convention the
		// exporter already uses for stable ordering - never an ambiguity error, so a repeated
		// idempotent call never starts failing just because an unrelated later change added a
		// second coincidentally-matching node.
		MatchingGuids.Sort([](const FGuid& A, const FGuid& B) { return A.ToString() < B.ToString(); });
		OutNodeGuid = MatchingGuids[0];
		OutCreated = false;
		return true;
	}

	// No match: create exactly one new node via the same ComputeReconcilePlan/ExecuteReconcilePlan
	// pipeline apply_flow_patch uses, so the same pre-commit validation, auto-placement, and
	// regraph apply - never a hand-rolled parallel creation path.
	static const FString NewNodeAlias = TEXT("findorcreate");

	TSharedRef<FJsonObject> PropertiesObject = MakeShared<FJsonObject>();
	for (const TPair<FString, FString>& Pair : MatchProperties)
	{
		PropertiesObject->SetStringField(Pair.Key, Pair.Value);
	}

	TSharedRef<FJsonObject> OpObject = MakeShared<FJsonObject>();
	OpObject->SetStringField(TEXT("kind"), TEXT("UpsertNode"));
	OpObject->SetStringField(TEXT("newAlias"), NewNodeAlias);
	OpObject->SetStringField(TEXT("type"), ResolvedClass->GetPathName());
	OpObject->SetObjectField(TEXT("properties"), PropertiesObject);

	TArray<TSharedPtr<FJsonValue>> OpsArray;
	OpsArray.Add(MakeShared<FJsonValueObject>(OpObject));

	UClass* ExpectedOwnerClass = ExistingAsset->GetExpectedOwnerClass();
	TSharedRef<FJsonObject> DocumentObject = MakeShared<FJsonObject>();
	DocumentObject->SetNumberField(TEXT("formatVersion"), 2);
	DocumentObject->SetStringField(TEXT("mode"), TEXT("Patch"));
	DocumentObject->SetStringField(TEXT("assetClass"), ExistingAsset->GetClass()->GetPathName());
	DocumentObject->SetBoolField(TEXT("bWorldBound"), ExistingAsset->bWorldBound);
	DocumentObject->SetStringField(TEXT("expectedOwnerClass"), ExpectedOwnerClass ? ExpectedOwnerClass->GetName() : FString());
	DocumentObject->SetArrayField(TEXT("ops"), OpsArray);

	FString MutationDocumentJson;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&MutationDocumentJson);
	FJsonSerializer::Serialize(DocumentObject, Writer);

	FString ComputeError;
	TSharedPtr<FFlowReconcileExecutionPlan> ExecutionPlan = ComputeReconcilePlan(TargetAssetPath, MutationDocumentJson, ComputeError);
	if (!ExecutionPlan)
	{
		OutErrorMessage = ComputeError.IsEmpty() ? TEXT("FindOrCreateNode: failed to compute creation plan") : ComputeError;
		return false;
	}

	const FGuid* MintedGuid = ExecutionPlan->AliasMap.Find(NewNodeAlias);
	if (!MintedGuid)
	{
		OutErrorMessage = TEXT("FindOrCreateNode: creation plan did not mint a GUID for the new node");
		return false;
	}

	if (bDryRun)
	{
		if (!FFlowGraphValidation::HasNoErrors(ExecutionPlan->ValidationFindings))
		{
			OutErrorMessage = TEXT("FindOrCreateNode: dry-run plan has blocking validation errors");
			return false;
		}

		OutNodeGuid = *MintedGuid;
		OutCreated = true;
		return true;
	}

	FString ExecuteError;
	FFlowReconcileResult Result = ExecuteReconcilePlan(*ExecutionPlan, TargetAssetPath, ExecuteError);
	if (!Result.bSuccess)
	{
		OutErrorMessage = ExecuteError.IsEmpty() ? TEXT("FindOrCreateNode: failed to execute creation plan") : ExecuteError;
		return false;
	}

	const FGuid* ExecutedGuid = Result.AliasMap.Find(NewNodeAlias);
	OutNodeGuid = ExecutedGuid ? *ExecutedGuid : *MintedGuid;
	OutCreated = true;
	return true;
}
