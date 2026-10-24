// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Layout/Visibility.h"
#include "UObject/Object.h"

class SWidget;
class UEdGraph;
class UFlowGraphNode;

/**
* Shared UI helpers for "Attach AddOn..." in Details panels.
*/
class FLOWEDITOR_API FFlowDetailsAddOnUI
{
public:
	/** Try to resolve the edited UObject (node or addon instance) to its UFlowGraphNode wrapper. */
	static UFlowGraphNode* FindGraphNodeForEditedObject(UObject* EditedObject);

	/** Return the owning UEdGraph for the graph node. */
	static UEdGraph* GetOwningEdGraph(UFlowGraphNode* GraphNode);

	/** Returns true if we can open/build the Attach AddOn menu for the edited object. */
	static bool CanAttachAddOn(UObject* EditedObject);

	/** Builds the menu widget content for attaching an addon (the same selector UI used by the context menu). */
	static TSharedRef<SWidget> BuildAttachAddOnMenuContent(UObject* EditedObject);

	/** Lower-level overload if caller already resolved graph + node. */
	static TSharedRef<SWidget> BuildAttachAddOnMenuContent(UEdGraph* Graph, UFlowGraphNode* GraphNode);

	/** Visible if any of the objects is a node or addon with attached AddOns. */
	static EVisibility GetAddOnsVisibility(const TArray<TWeakObjectPtr<UObject>>& Objects);
};