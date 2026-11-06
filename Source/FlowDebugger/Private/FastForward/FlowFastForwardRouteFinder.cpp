// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "FastForward/FlowFastForwardRouteFinder.h"

#include "FlowAsset.h"
#include "FlowSubsystem.h"
#include "Nodes/Graph/FlowNode_CustomInput.h"
#include "Nodes/Graph/FlowNode_CustomOutput.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

#include "AssetRegistry/IAssetRegistry.h"

FFlowFastForwardRouteFinder::FFlowFastForwardRouteFinder(const UFlowSubsystem& InFlowSubsystem)
	: FlowSubsystem(InFlowSubsystem)
{
}

EFlowFastForwardRouteResult FFlowFastForwardRouteFinder::FindRoute(const UFlowNode& TargetNode, FFlowFastForwardRoute& OutRoute)
{
	EFlowFastForwardRouteResult Result = EFlowFastForwardRouteResult::InvalidTarget;

	OutRoute = FFlowFastForwardRoute();
	PackagesReachingTarget.Reset();
	Contexts.Reset();
	States.Reset();
	VisitedStates.Reset();

	if (const UFlowAsset* TargetAsset = TargetNode.GetFlowAsset())
	{
		TargetTemplate = TargetAsset->IsInstanceInitialized() ? TargetAsset->GetTemplateAsset() : TargetAsset;

		CollectPackagesReachingTarget();
		BuildContexts();
		AddStartStates(TargetNode);

		if (States.Num() > 0)
		{
			if (IsAnyStartStateActive())
			{
				Result = EFlowFastForwardRouteResult::TargetAlreadyActive;
			}
			else
			{
				const int32 AnchorStateIndex = Search();
				if (AnchorStateIndex != INDEX_NONE)
				{
					BuildRoute(AnchorStateIndex, OutRoute);
					Result = EFlowFastForwardRouteResult::Found;
				}
				else
				{
					Result = EFlowFastForwardRouteResult::NoActiveNodeOnRoute;
				}
			}
		}
	}

	return Result;
}

void FFlowFastForwardRouteFinder::CollectPackagesReachingTarget()
{
	// Asset Registry knows the references without loading assets
	const IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
	TArray<FName> PendingPackages = {TargetTemplate->GetPackage()->GetFName()};

	while (PendingPackages.Num() > 0)
	{
		const FName PackageName = PendingPackages.Pop();
		if (!PackagesReachingTarget.Contains(PackageName))
		{
			PackagesReachingTarget.Add(PackageName);

			TArray<FName> Referencers;
			AssetRegistry.GetReferencers(PackageName, Referencers);
			PendingPackages.Append(Referencers);
		}
	}
}

void FFlowFastForwardRouteFinder::BuildContexts()
{
	for (UFlowAsset* RootInstance : FlowSubsystem.GetAllRootInstances())
	{
		if (IsValid(RootInstance))
		{
			AddInstanceContext(*RootInstance, INDEX_NONE);
		}
	}
}

void FFlowFastForwardRouteFinder::AddInstanceContext(UFlowAsset& Instance, const int32 ParentIndex)
{
	FGraphContext Context;
	Context.TemplateAsset = Instance.GetTemplateAsset();
	Context.Instance = &Instance;
	Context.ParentIndex = ParentIndex;

	if (const UFlowNode_SubGraph* OwningSubGraphNode = Instance.GetNodeOwningThisAssetInstance())
	{
		// Finished or preloaded instance gets started again by the SubGraph node, so it's traversed by the template
		Context.OwningSubGraphGuid = OwningSubGraphNode->GetGuid();
		Context.bRunning = OwningSubGraphNode->GetActivationState() == EFlowNodeState::Active;
		Context.bCanStartFromParent = !Context.bRunning;
	}
	else
	{
		Context.bRunning = true;
	}

	Context.Graph = Context.bRunning ? &Instance : Context.TemplateAsset;

	ExpandSubGraphs(Contexts.Add(MoveTemp(Context)));
}

void FFlowFastForwardRouteFinder::AddTemplateContext(const UFlowNode_SubGraph& SubGraphNode, const int32 ParentIndex)
{
	const UFlowAsset* ChildTemplate = PackagesReachingTarget.Contains(SubGraphNode.GetAsset().ToSoftObjectPath().GetLongPackageFName())
		? SubGraphNode.GetAsset().LoadSynchronous()
		: nullptr;

	if (ChildTemplate)
	{
		const int32 ContextIndex = AddVirtualContext(SubGraphNode, *ChildTemplate, ParentIndex);
		if (ContextIndex != INDEX_NONE)
		{
			ExpandSubGraphs(ContextIndex);
		}
	}
}

int32 FFlowFastForwardRouteFinder::AddVirtualContext(const UFlowNode_SubGraph& SubGraphNode, const UFlowAsset& ChildTemplate, const int32 ParentIndex)
{
	int32 ContextIndex = INDEX_NONE;

	if (!IsTemplateTraversedByAncestor(&ChildTemplate, ParentIndex))
	{
		FGraphContext Context;
		Context.Graph = &ChildTemplate;
		Context.TemplateAsset = &ChildTemplate;
		Context.ParentIndex = ParentIndex;
		Context.OwningSubGraphGuid = SubGraphNode.GetGuid();
		Context.bCanStartFromParent = true;

		ContextIndex = Contexts.Add(MoveTemp(Context));
	}

	return ContextIndex;
}

int32 FFlowFastForwardRouteFinder::FindOrAddChildContext(const UFlowNode_SubGraph& SubGraphNode, const int32 ParentIndex)
{
	int32 ChildIndex = Contexts.IndexOfByPredicate([&SubGraphNode, ParentIndex](const FGraphContext& Context)
	{
		return Context.ParentIndex == ParentIndex && Context.OwningSubGraphGuid == SubGraphNode.GetGuid();
	});

	// Graphs not leading to the target template weren't added while building contexts
	if (ChildIndex == INDEX_NONE)
	{
		if (const UFlowAsset* ChildTemplate = SubGraphNode.GetAsset().LoadSynchronous())
		{
			ChildIndex = AddVirtualContext(SubGraphNode, *ChildTemplate, ParentIndex);
		}
	}

	return ChildIndex;
}

void FFlowFastForwardRouteFinder::ExpandSubGraphs(const int32 ContextIndex)
{
	// SubGraph nodes of an instance are keys of instanced SubFlows, template nodes aren't
	UFlowAsset* Instance = Contexts[ContextIndex].Instance;
	const UFlowAsset* NodesOwner = Instance ? Instance : Contexts[ContextIndex].Graph;

	for (const TPair<FGuid, UFlowNode*>& Node : NodesOwner->GetNodes())
	{
		if (UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(Node.Value))
		{
			UFlowAsset* SubFlow = Instance ? FlowSubsystem.GetInstancedSubFlows().FindRef(SubGraphNode) : nullptr;
			if (IsValid(SubFlow))
			{
				AddInstanceContext(*SubFlow, ContextIndex);
			}
			else
			{
				AddTemplateContext(*SubGraphNode, ContextIndex);
			}
		}
	}
}

bool FFlowFastForwardRouteFinder::IsTemplateTraversedByAncestor(const UFlowAsset* Template, const int32 ContextIndex) const
{
	// Running graphs are finite, only graphs traversed by templates can nest themselves endlessly
	bool bTraversed = false;

	for (int32 Index = ContextIndex; Index != INDEX_NONE && !bTraversed && !Contexts[Index].bRunning; Index = Contexts[Index].ParentIndex)
	{
		bTraversed = Contexts[Index].TemplateAsset == Template;
	}

	return bTraversed;
}

void FFlowFastForwardRouteFinder::AddStartStates(const UFlowNode& TargetNode)
{
	const UFlowAsset* TargetAsset = TargetNode.GetFlowAsset();
	const bool bTargetInInstance = TargetAsset->IsInstanceInitialized();

	for (int32 ContextIndex = 0; ContextIndex < Contexts.Num(); ++ContextIndex)
	{
		const FGraphContext& Context = Contexts[ContextIndex];
		if (bTargetInInstance ? Context.Instance == TargetAsset : Context.TemplateAsset == TargetAsset)
		{
			FSearchState TargetState;
			TargetState.ContextIndex = ContextIndex;
			TargetState.NodeGuid = TargetNode.GetGuid();
			AddState(TargetState);
		}
	}
}

bool FFlowFastForwardRouteFinder::IsAnyStartStateActive() const
{
	bool bActive = false;

	for (const FSearchState& State : States)
	{
		bActive |= IsNodeActive(State);
	}

	return bActive;
}

int32 FFlowFastForwardRouteFinder::Search()
{
	int32 AnchorStateIndex = INDEX_NONE;

	// States array is the queue of breadth-first search, so the first active node is the nearest one
	// Active node starts the route only by an output it can trigger without receiving another input
	for (int32 StateIndex = 0; StateIndex < States.Num() && AnchorStateIndex == INDEX_NONE; ++StateIndex)
	{
		const FSearchState& State = States[StateIndex];
		if (!State.OutputPinName.IsNone() && IsNodeActive(State)
			&& Contexts[State.ContextIndex].Graph->GetNode(State.NodeGuid)->CanFastForward(NAME_None, State.OutputPinName))
		{
			AnchorStateIndex = StateIndex;
		}
		else
		{
			AddIncomingStates(StateIndex);
			AddOwningSubGraphState(StateIndex);
		}
	}

	return AnchorStateIndex;
}

void FFlowFastForwardRouteFinder::AddIncomingStates(const int32 StateIndex)
{
	const FSearchState State = States[StateIndex];

	if (!Contexts[State.ContextIndex].bIncomingConnectionsBuilt)
	{
		BuildIncomingConnections(Contexts[State.ContextIndex]);
	}

	// Entering the graph of a SubGraph node adds contexts, so the context isn't referenced below
	const UFlowAsset* Graph = Contexts[State.ContextIndex].Graph;
	const UFlowNode* Node = Graph->GetNode(State.NodeGuid);
	TArray<FIncomingConnection> IncomingConnections;
	Contexts[State.ContextIndex].IncomingConnections.MultiFind(State.NodeGuid, IncomingConnections, true);

	for (const FIncomingConnection& Connection : IncomingConnections)
	{
		// Node receiving the signal without passing it further accepts it on any input
		const bool bInputLeadsToOutput = State.OutputPinName.IsNone() || (Node && Node->CanFastForward(Connection.InputPinName, State.OutputPinName));

		if (bInputLeadsToOutput && (State.RequiredInputPinName.IsNone() || Connection.InputPinName == State.RequiredInputPinName))
		{
			// Disabled node ignores the signal, so it can't be on the route
			const UFlowNode* SourceNode = Graph->GetNode(Connection.SourcePin.NodeGuid);
			if (SourceNode && SourceNode->GetSignalMode() != EFlowSignalMode::Disabled)
			{
				// Signal leaving the graph of a SubGraph node comes from the node finishing that graph or sending its custom output
				// SubGraph node passing the signal through doesn't run its graph
				const UFlowNode_SubGraph* SubGraphNode = SourceNode->GetSignalMode() == EFlowSignalMode::Enabled ? Cast<UFlowNode_SubGraph>(SourceNode) : nullptr;
				if (SubGraphNode == nullptr || !AddSubGraphExitStates(StateIndex, *SubGraphNode, Connection))
				{
					FSearchState SourceState;
					SourceState.ContextIndex = State.ContextIndex;
					SourceState.NodeGuid = Connection.SourcePin.NodeGuid;
					SourceState.NextStateIndex = StateIndex;
					SourceState.OutputPinName = Connection.SourcePin.PinName;
					SourceState.NextInputPinName = Connection.InputPinName;
					AddState(SourceState);
				}
			}
		}
	}
}

bool FFlowFastForwardRouteFinder::AddSubGraphExitStates(const int32 StateIndex, const UFlowNode_SubGraph& SubGraphNode, const FIncomingConnection& Connection)
{
	bool bExitFound = false;

	const int32 ChildIndex = FindOrAddChildContext(SubGraphNode, States[StateIndex].ContextIndex);
	if (ChildIndex != INDEX_NONE)
	{
		const bool bFinishPin = Connection.SourcePin.PinName == UFlowNode_SubGraph::FinishPin.PinName;

		for (const TPair<FGuid, UFlowNode*>& Node : Contexts[ChildIndex].Graph->GetNodes())
		{
			const UFlowNode_CustomOutput* CustomOutputNode = Cast<UFlowNode_CustomOutput>(Node.Value);
			const bool bExitNode = bFinishPin
				? Node.Value && Node.Value->CanFinishGraph()
				: CustomOutputNode && CustomOutputNode->GetEventName() == Connection.SourcePin.PinName;

			if (bExitNode)
			{
				bExitFound = true;

				if (Node.Value->GetSignalMode() != EFlowSignalMode::Disabled)
				{
					FSearchState ExitState;
					ExitState.ContextIndex = ChildIndex;
					ExitState.NodeGuid = Node.Key;
					ExitState.NextStateIndex = StateIndex;
					ExitState.NextInputPinName = Connection.InputPinName;
					AddState(ExitState);
				}
			}
		}
	}

	return bExitFound;
}

void FFlowFastForwardRouteFinder::AddOwningSubGraphState(const int32 StateIndex)
{
	const FSearchState State = States[StateIndex];
	const FGraphContext& Context = Contexts[State.ContextIndex];

	if (Context.ParentIndex != INDEX_NONE)
	{
		// Pin of the SubGraph node triggering this entry node
		FName SubGraphInputPinName = NAME_None;

		// SubGraph node starts only a graph that isn't running, and passes custom inputs only to a running graph
		const UFlowNode* Node = Context.Graph->GetNode(State.NodeGuid);
		if (Node && Node == Context.Graph->GetDefaultEntryNode() && Context.bCanStartFromParent)
		{
			SubGraphInputPinName = UFlowNode_SubGraph::StartPin.PinName;
		}
		else if (const UFlowNode_CustomInput* CustomInputNode = Cast<UFlowNode_CustomInput>(Node))
		{
			SubGraphInputPinName = Context.bRunning ? CustomInputNode->GetEventName() : NAME_None;
		}

		const UFlowNode* SubGraphNode = Contexts[Context.ParentIndex].Graph->GetNode(Context.OwningSubGraphGuid);
		if (!SubGraphInputPinName.IsNone() && SubGraphNode && SubGraphNode->GetSignalMode() == EFlowSignalMode::Enabled)
		{
			FSearchState SubGraphState;
			SubGraphState.ContextIndex = Context.ParentIndex;
			SubGraphState.NodeGuid = Context.OwningSubGraphGuid;
			SubGraphState.RequiredInputPinName = SubGraphInputPinName;
			SubGraphState.NextStateIndex = StateIndex;
			AddState(SubGraphState);
		}
	}
}

void FFlowFastForwardRouteFinder::AddState(const FSearchState& State)
{
	bool bAlreadyVisited = false;
	VisitedStates.Add(FSearchStateKey(State.ContextIndex, State.NodeGuid, State.RequiredInputPinName, State.OutputPinName), &bAlreadyVisited);

	if (!bAlreadyVisited)
	{
		States.Add(State);
	}
}

bool FFlowFastForwardRouteFinder::IsNodeActive(const FSearchState& State) const
{
	const FGraphContext& Context = Contexts[State.ContextIndex];
	const UFlowNode* Node = Context.bRunning ? Context.Graph->GetNode(State.NodeGuid) : nullptr;

	return Node && Node->GetActivationState() == EFlowNodeState::Active;
}

void FFlowFastForwardRouteFinder::BuildIncomingConnections(FGraphContext& Context) const
{
	// Connections also store data input pins, so only exec outputs are taken from them
	for (const TPair<FGuid, UFlowNode*>& Node : Context.Graph->GetNodes())
	{
		if (Node.Value)
		{
			for (const FFlowPin& OutputPin : Node.Value->GetOutputPins())
			{
				const FConnectedPin ConnectedPin = Node.Value->GetConnection(OutputPin.PinName);
				if (OutputPin.IsExecPin() && ConnectedPin.NodeGuid.IsValid())
				{
					Context.IncomingConnections.Add(ConnectedPin.NodeGuid, FIncomingConnection{ConnectedPin.PinName, FConnectedPin(Node.Key, OutputPin.PinName)});
				}
			}
		}
	}

	Context.bIncomingConnectionsBuilt = true;
}

void FFlowFastForwardRouteFinder::BuildRoute(const int32 AnchorStateIndex, FFlowFastForwardRoute& OutRoute) const
{
	OutRoute.StartInstance = Contexts[States[AnchorStateIndex].ContextIndex].Instance;

	// Active node starting the route received its input before
	FName InputPinName = NAME_None;
	int32 SegmentContextIndex = INDEX_NONE;

	for (int32 StateIndex = AnchorStateIndex; StateIndex != INDEX_NONE; StateIndex = States[StateIndex].NextStateIndex)
	{
		const FSearchState& State = States[StateIndex];

		if (State.ContextIndex != SegmentContextIndex)
		{
			SegmentContextIndex = State.ContextIndex;
			OutRoute.Segments.AddDefaulted_GetRef().TemplateAsset = Contexts[SegmentContextIndex].TemplateAsset;
		}

		FFlowFastForwardStep& Step = OutRoute.Segments.Last().Steps.AddDefaulted_GetRef();
		Step.NodeGuid = State.NodeGuid;
		Step.InputPinName = InputPinName;
		Step.OutputPinName = State.OutputPinName;

		InputPinName = State.NextInputPinName;
	}
}
