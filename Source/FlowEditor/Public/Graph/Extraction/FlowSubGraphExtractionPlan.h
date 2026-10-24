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
 * Extracts a selection of Flow graph nodes into a newly created Flow Asset, replacing the selection
 * with a SubGraph node that runs that asset.
 *
 * The SubGraph interface is derived from the connections that cross the selection boundary. A pin is
 * on the boundary when at least one of its links terminates on a node outside the selection, which is
 * the same test FBlueprintEditor::CollapseNodesIntoGraph uses.
 *
 * Flow's SubGraph interface is not a Blueprint tunnel, and three of its runtime rules shape what can
 * be extracted:
 *
 * 1. Only the Start pin instantiates the child. UFlowNode_SubGraph::ExecuteInput calls CreateSubFlow
 *    for Start alone; any other input pin routes to UFlowAsset::TriggerCustomInput_FromSubGraph, which
 *    requires an already-running instance. An extractable selection therefore has exactly one incoming
 *    execution boundary, and that boundary maps to Start.
 *
 * 2. Every ordinary exit becomes a Custom Output named after its pin, or its node's title for an unnamed
 *    default pin. Names are unique and cannot use reserved interface names. An ordinary exit must not
 *    end the child instance because that would cancel other active branches. A Finish node is placed
 *    but left unconnected, so the SubGraph stays active
 *    until the parent tears it down in UFlowNode_SubGraph::Cleanup. That is normal for a long-running
 *    SubGraph and matches shipped content.
 *
 * 3. Data pins cannot cross the boundary at all. The interface mirrors only the exec-only CustomInputs
 *    and CustomOutputs arrays, so a crossing data link is a hard error.
 */
namespace FlowSubGraphExtraction
{
	/** A pin inside the selection that has at least one link leaving it. */
	struct FBoundaryPin
	{
		/** Pin on a selected node. Valid only while planning; the node is destroyed by Apply. */
		UEdGraphPin* InnerPin = nullptr;

		/** Pins on non-selected nodes that InnerPin links to. These survive the extraction. */
		TArray<UEdGraphPin*> OuterPins;

		/** Guid of the selected node owning InnerPin, used to find its clone in the new asset. */
		FGuid InnerNodeGuid;

		/** Name of InnerPin, used to find the matching pin on the clone. */
		FName InnerPinName;

		/** Pin name exposed on the SubGraph node: Start or a unique custom output name. */
		FName InterfaceName;

		/** True when this boundary maps to the built-in Start pin rather than a custom output. */
		bool bUsesBuiltInPin = false;
	};

	/**
	 * A description of the extraction, produced before anything is modified so the caller can
	 * refuse or warn without having touched the graph.
	 */
	struct FLOWEDITOR_API FExtractionPlan
	{
		/** Builds and validates an extraction plan from the selected graph nodes. */
		explicit FExtractionPlan(const TSet<UFlowGraphNode*>& InSelectedNodes);

		/** Nodes that will move into the new asset. */
		TSet<UFlowGraphNode*> SelectedNodes;

		/** Boundary pins that execution enters the selection through. */
		TArray<FBoundaryPin> Entries;

		/** Boundary pins that execution leaves the selection through. */
		TArray<FBoundaryPin> Exits;

		/** Conditions that make the extraction impossible. Non-empty means the action must not run. */
		TArray<FText> Errors;

		/** Conditions the user should confirm before proceeding. */
		TArray<FText> Warnings;

		bool CanApply() const { return Errors.IsEmpty() && !SelectedNodes.IsEmpty(); }

	private:
		/** Collects valid top-level nodes. */
		void GatherSelectedNodes(const TSet<UFlowGraphNode*>& InSelectedNodes);

		/** Records errors for nodes that do not share a graph or cannot move into a SubGraph. */
		void ValidateSelectedNodes();

		/** Finds connections crossing the selection boundary and classifies them as entries or exits. */
		void GatherBoundaryPins();

		/** Requires one incoming execution connection and maps it to the built-in Start interface. */
		void ConfigureEntry();

		/** Names all ordinary exits as Custom Outputs, preserving concurrent child execution. */
		void ConfigureExits();

		/** Orders boundary pins top-to-bottom then left-to-right for deterministic interface naming. */
		static void SortBoundaryPinsByPosition(TArray<FBoundaryPin>& BoundaryPins);
	};

	/** Builds a read-only plan describing the boundary of the selection. Modifies nothing. */
	FLOWEDITOR_API FExtractionPlan PlanExtraction(const TSet<UFlowGraphNode*>& SelectedNodes);

	/** Name proposed for a generated SubGraph asset, starting with the project's required prefix. */
	FLOWEDITOR_API FString GetDefaultNewAssetName();

	/** Validates a proposed asset name, returning an empty text when it is usable. */
	FLOWEDITOR_API FText ValidateNewAssetName(const UFlowAsset& SourceAsset, const FString& NewAssetName);
}
