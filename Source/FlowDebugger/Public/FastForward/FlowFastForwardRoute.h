// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Containers/Array.h"
#include "Misc/Guid.h"
#include "UObject/NameTypes.h"
#include "UObject/WeakObjectPtrTemplates.h"

class UFlowAsset;

/**
 * Single node the fast forwarded signal passes through.
 */
struct FLOWDEBUGGER_API FFlowFastForwardStep
{
	FGuid NodeGuid;

	/* Exec input receiving the signal.
	 * None for the node starting the route and for entry nodes of the graph. */
	FName InputPinName = NAME_None;

	/* Exec output passing the signal further.
	 * None for the target node and for a node passing the signal to another graph. */
	FName OutputPinName = NAME_None;
};

/**
 * Steps executed within a single graph.
 */
struct FLOWDEBUGGER_API FFlowFastForwardSegment
{
	TWeakObjectPtr<const UFlowAsset> TemplateAsset;

	/* Ordered in execution order. */
	TArray<FFlowFastForwardStep> Steps;
};

/**
 * Route of the signal from the active node to the node chosen by user.
 * Steps address nodes by NodeGuid, since instances of graphs below StartInstance are created while fast forwarding.
 */
struct FLOWDEBUGGER_API FFlowFastForwardRoute
{
	/* Running instance containing the active node starting the route. */
	TWeakObjectPtr<UFlowAsset> StartInstance;

	/* Ordered from the graph of StartInstance to the graph of the target node.
	 * Each next segment runs in the graph entered or left by the last node of the previous segment. */
	TArray<FFlowFastForwardSegment> Segments;

	bool IsEmpty() const { return Segments.IsEmpty(); }
};

enum class EFlowFastForwardRouteResult : uint8
{
	Found,

	/* Target node is active already, there is nothing to fast forward. */
	TargetAlreadyActive,

	/* Every route backward ends at the entry of a running graph or at a node that doesn't pass the signal. */
	NoActiveNodeOnRoute,

	/* Target node doesn't belong to a running graph or to a graph that running graphs can instance. */
	InvalidTarget
};
