// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Containers/Array.h"
#include "Containers/Set.h"
#include "Internationalization/Text.h"
#include "Misc/Guid.h"
#include "UObject/NameTypes.h"

class UEdGraphPin;
class UFlowAsset;
class UFlowGraphNode;

/**
 * Collapses a selection of Flow graph nodes into a newly created Flow Asset, replacing the selection
 * with a SubGraph node that runs that asset.
 *
 * The sub-graph interface is derived from the connections that cross the selection boundary. A pin is
 * on the boundary when at least one of its links terminates on a node outside the selection, which is
 * the same test Blueprint's collapse uses (FBlueprintEditor::CollapseNodesIntoGraph).
 *
 * Flow's sub-graph interface is not a Blueprint tunnel, and three of its runtime rules shape what can
 * be collapsed:
 *
 * 1. Only the Start pin instantiates the child. UFlowNode_SubGraph::ExecuteInput calls CreateSubFlow
 *    for Start alone; any other input pin routes to UFlowAsset::TriggerCustomInput_FromSubGraph, which
 *    requires an already-running instance. A collapsible selection therefore has exactly one incoming
 *    execution boundary, and that boundary maps to Start.
 *
 * 2. The output interface is named after the pins it replaces. An exit crossing an unnamed default pin
 *    (UFlowNode::DefaultOutputPin) has nothing to name an event after, so it maps to the built-in Finish.
 *    An exit on a named pin becomes a Custom Output carrying that name, so the parent's wiring reconnects
 *    to a pin matching the original. Finish can be claimed only once because it is a single exec pin and
 *    an exec output drives exactly one target; later unnamed boundaries fall back to a node-derived name.
 *    A Finish node is always placed, and is simply left unconnected when nothing routes to it. Reaching
 *    a Finish is what ends the child instance, so a sub-graph with only Custom Outputs stays active
 *    until the parent tears it down in UFlowNode_SubGraph::Cleanup. That is normal for a long-running
 *    sub-graph and matches shipped content.
 *
 * 3. Data pins cannot cross the boundary at all. The interface mirrors only the exec-only CustomInputs
 *    and CustomOutputs arrays, so a crossing data link is a hard error.
 */
namespace FlowCollapseToSubGraph
{
	/** A pin inside the selection that has at least one link leaving it. */
	struct FBoundaryPin
	{
		/** Pin on a selected node. Valid only while planning; the node is destroyed by Apply. */
		UEdGraphPin* InnerPin = nullptr;

		/** Pins on non-selected nodes that InnerPin links to. These survive the collapse. */
		TArray<UEdGraphPin*> OuterPins;

		/** Guid of the selected node owning InnerPin, used to find its clone in the new asset. */
		FGuid InnerNodeGuid;

		/** Name of InnerPin, used to find the matching pin on the clone. */
		FName InnerPinName;

		/** Pin name exposed on the SubGraph node: Start, Finish, or a custom event name taken from the pin. */
		FName InterfaceName;

		/** True when this boundary maps to the built-in Start or Finish pin rather than a custom event. */
		bool bUsesBuiltInPin = false;
	};

	/**
	 * A description of the collapse, produced before anything is modified so the caller can
	 * refuse or warn without having touched the graph.
	 */
	struct FCollapsePlan
	{
		/** Builds and validates a collapse plan from the selected graph nodes. */
		explicit FCollapsePlan(const TSet<UFlowGraphNode*>& InSelectedNodes);

		/** Nodes that will move into the new asset. */
		TSet<UFlowGraphNode*> SelectedNodes;

		/** Boundary pins that execution enters the selection through. */
		TArray<FBoundaryPin> Entries;

		/** Boundary pins that execution leaves the selection through. */
		TArray<FBoundaryPin> Exits;

		/** Conditions that make the collapse impossible. Non-empty means the action must not run. */
		TArray<FText> Errors;

		/** Conditions the user should confirm before proceeding. */
		TArray<FText> Warnings;

		bool CanApply() const { return Errors.IsEmpty() && !SelectedNodes.IsEmpty(); }

	private:
		/** Collects valid top-level nodes. */
		void GatherSelectedNodes(const TSet<UFlowGraphNode*>& InSelectedNodes);

		/** Records errors for nodes that do not share a graph or cannot move into a sub-graph. */
		void ValidateSelectedNodes();

		/** Finds connections crossing the selection boundary and classifies them as entries or exits. */
		void GatherBoundaryPins();

		/** Requires one incoming execution connection and maps it to the built-in Start interface. */
		void ConfigureEntry();

		/** Maps the first unnamed exit to Finish and names additional exits as Custom Outputs. */
		void ConfigureExits();

		/** Orders boundary pins top-to-bottom then left-to-right for deterministic interface naming. */
		static void SortBoundaryPinsByPosition(TArray<FBoundaryPin>& BoundaryPins);
	};

	/** Builds a read-only plan describing the boundary of the selection. Modifies nothing. */
	FCollapsePlan PlanCollapse(const TSet<UFlowGraphNode*>& SelectedNodes);

	/** Required prefix for a generated sub-graph asset. */
	const FString& GetRequiredAssetNamePrefix();

	/** Validates a proposed asset name, returning an empty text when it is usable. */
	FText ValidateNewAssetName(const UFlowAsset& SourceAsset, const FString& NewAssetName);
}
