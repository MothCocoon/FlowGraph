// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowPin.h"
#include "FastForward/FlowFastForwardRoute.h"
#include "Templates/Tuple.h"

class UFlowAsset;
class UFlowNode;
class UFlowNode_SubGraph;
class UFlowSubsystem;

/**
 * Finds the shortest route of the exec signal from the nearest active node to the target node, also through SubGraphs.
 * Graphs not instanced yet are traversed by their templates.
 */
class FLOWDEBUGGER_API FFlowFastForwardRouteFinder
{
public:
	explicit FFlowFastForwardRouteFinder(const UFlowSubsystem& InFlowSubsystem);

	/* Target node belongs to a Flow Asset instance or to a template, if the graph isn't instanced yet. */
	EFlowFastForwardRouteResult FindRoute(const UFlowNode& TargetNode, FFlowFastForwardRoute& OutRoute);

private:
	struct FIncomingConnection
	{
		FName InputPinName = NAME_None;
		FConnectedPin SourcePin;
	};

	struct FGraphContext
	{
		/* Instance of a running graph, template otherwise. */
		const UFlowAsset* Graph = nullptr;

		const UFlowAsset* TemplateAsset = nullptr;

		/* Instance represented by this context, also if its graph is traversed by the template. */
		UFlowAsset* Instance = nullptr;

		int32 ParentIndex = INDEX_NONE;
		FGuid OwningSubGraphGuid;

		/* Nodes of a running graph can be active. */
		bool bRunning = false;

		/* SubGraph node in the parent graph can start this graph. */
		bool bCanStartFromParent = false;

		bool bIncomingConnectionsBuilt = false;

		/* Exec connections keyed by the node receiving the signal. */
		TMultiMap<FGuid, FIncomingConnection> IncomingConnections;
	};

	struct FSearchState
	{
		int32 ContextIndex = INDEX_NONE;
		FGuid NodeGuid;

		/* None accepts the signal on any exec input. */
		FName RequiredInputPinName = NAME_None;

		/* State one step closer to the target node. */
		int32 NextStateIndex = INDEX_NONE;

		/* None if the signal passes to the graph of the next state. */
		FName OutputPinName = NAME_None;

		/* Input of the next node receiving the signal. */
		FName NextInputPinName = NAME_None;
	};

	using FSearchStateKey = TTuple<int32, FGuid, FName, FName>;

	const UFlowSubsystem& FlowSubsystem;
	const UFlowAsset* TargetTemplate = nullptr;

	/* Package of the target template and packages referencing it directly or indirectly. */
	TSet<FName> PackagesReachingTarget;

	TArray<FGraphContext> Contexts;

	/* Ordered by breadth-first search. */
	TArray<FSearchState> States;
	TSet<FSearchStateKey> VisitedStates;

	void CollectPackagesReachingTarget();

	void BuildContexts();
	void AddInstanceContext(UFlowAsset& Instance, const int32 ParentIndex);
	void AddTemplateContext(const UFlowNode_SubGraph& SubGraphNode, const int32 ParentIndex);
	int32 AddVirtualContext(const UFlowNode_SubGraph& SubGraphNode, const UFlowAsset& ChildTemplate, const int32 ParentIndex);
	int32 FindOrAddChildContext(const UFlowNode_SubGraph& SubGraphNode, const int32 ParentIndex);
	void ExpandSubGraphs(const int32 ContextIndex);
	bool IsTemplateTraversedByAncestor(const UFlowAsset* Template, const int32 ContextIndex) const;

	void AddStartStates(const UFlowNode& TargetNode);
	bool IsAnyStartStateActive() const;

	int32 Search();
	void AddIncomingStates(const int32 StateIndex);

	/* Returns false if the output of the SubGraph node doesn't come from its graph. */
	bool AddSubGraphExitStates(const int32 StateIndex, const UFlowNode_SubGraph& SubGraphNode, const FIncomingConnection& Connection);

	void AddOwningSubGraphState(const int32 StateIndex);
	void AddState(const FSearchState& State);
	bool IsNodeActive(const FSearchState& State) const;
	void BuildIncomingConnections(FGraphContext& Context) const;

	void BuildRoute(const int32 AnchorStateIndex, FFlowFastForwardRoute& OutRoute) const;
};
