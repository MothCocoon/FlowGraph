// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "FastForward/FlowFastForwardSubsystem.h"

#include "FlowAsset.h"
#include "FlowLogChannels.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Graph/FlowNode_SubGraph.h"

#include "Engine/GameInstance.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowFastForwardSubsystem)

void UFlowFastForwardSubsystem::Deinitialize()
{
	StopFastForward();

	Super::Deinitialize();
}

void UFlowFastForwardSubsystem::StartFastForward(const FFlowFastForwardRoute& InRoute)
{
	StopFastForward();

	const UFlowAsset* StartInstance = InRoute.StartInstance.Get();
	const FFlowFastForwardStep* AnchorStep = InRoute.IsEmpty() ? nullptr : &InRoute.Segments[0].Steps[0];
	UFlowNode* AnchorNode = StartInstance && AnchorStep ? StartInstance->GetNode(AnchorStep->NodeGuid) : nullptr;

	if (AnchorNode)
	{
		Route = InRoute;
		SegmentInstances.Init(nullptr, Route.Segments.Num());
		SegmentInstances[0] = Route.StartInstance;
		SegmentIndex = 0;
		StepIndex = 0;
		AdvanceStep();

		FFlowFastForwardGate::SetGate(this);

		AnchorNode->TriggerFastForwardInput(AnchorStep->InputPinName, AnchorStep->OutputPinName);
	}
	else
	{
		UE_LOG(LogFlow, Warning, TEXT("Flow Fast Forward can't start, the active node starting the route is missing"));
	}
}

void UFlowFastForwardSubsystem::StopFastForward()
{
	Route = FFlowFastForwardRoute();
	SegmentInstances.Reset();
	SegmentIndex = INDEX_NONE;
	StepIndex = INDEX_NONE;

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetTimerManager().ClearTimer(StallTimerHandle);
	}

	if (FFlowFastForwardGate::GetGate() == this)
	{
		FFlowFastForwardGate::SetGate(nullptr);
	}
}

bool UFlowFastForwardSubsystem::ShouldFastForwardInput(const UFlowNode& Node, const FName& PinName, FName& OutOutputPinName)
{
	bool bFastForward = false;

	if (IsExpectedStep(Node, PinName))
	{
		OutOutputPinName = Route.Segments[SegmentIndex].Steps[StepIndex].OutputPinName;
		SegmentInstances[SegmentIndex] = Node.GetFlowAsset();

		// Nodes passing the signal to another graph and the target node execute their logic without forcing any output
		bFastForward = !OutOutputPinName.IsNone();

		AdvanceStep();
	}
	else
	{
		ReportSignalOutsideRoute(Node, PinName);
	}

	return bFastForward;
}

void UFlowFastForwardSubsystem::OnOutputForced(const UFlowNode& Node, const FName& OutputPinName)
{
	int32& ForcedOutputCount = ForcedOutputCounts.FindOrAdd(Node.GetClass());
	++ForcedOutputCount;

	UE_LOG(LogFlow, Log, TEXT("Flow Fast Forward forced output %s of %s (%s), outputs of this class forced %d times in this session"),
		*OutputPinName.ToString(), *Node.GetName(), *Node.GetClass()->GetName(), ForcedOutputCount);
}

void UFlowFastForwardSubsystem::OnRouteBroken(const UFlowNode& Node, const FName& OutputPinName)
{
	UE_LOG(LogFlow, Warning, TEXT("Flow Fast Forward stopped before segment %d, step %d. Node %s (%s) finished without triggering output %s"),
		SegmentIndex, StepIndex, *Node.GetName(), *Node.GetClass()->GetName(), *OutputPinName.ToString());

	// Gate left registered would fast forward this route when the game reaches it later
	StopFastForward();
}

bool UFlowFastForwardSubsystem::IsExpectedStep(const UFlowNode& Node, const FName& PinName) const
{
	return IsRouteNode(Node, SegmentIndex, StepIndex) && PinName == Route.Segments[SegmentIndex].Steps[StepIndex].InputPinName;
}

bool UFlowFastForwardSubsystem::IsRouteNode(const UFlowNode& Node, const int32 InSegmentIndex, const int32 InStepIndex) const
{
	bool bRouteNode = false;

	if (Route.Segments.IsValidIndex(InSegmentIndex) && Route.Segments[InSegmentIndex].Steps.IsValidIndex(InStepIndex))
	{
		bRouteNode = Node.GetGuid() == Route.Segments[InSegmentIndex].Steps[InStepIndex].NodeGuid && IsSegmentInstance(Node.GetFlowAsset(), InSegmentIndex);
	}

	return bRouteNode;
}

bool UFlowFastForwardSubsystem::IsSegmentInstance(const UFlowAsset* Instance, const int32 InSegmentIndex) const
{
	bool bSegmentInstance = false;

	if (Instance && SegmentInstances.IsValidIndex(InSegmentIndex))
	{
		const UFlowAsset* SegmentInstance = SegmentInstances[InSegmentIndex].Get();
		if (SegmentInstance)
		{
			bSegmentInstance = Instance == SegmentInstance;
		}
		else if (InSegmentIndex > 0 && Route.Segments[InSegmentIndex].Steps[0].InputPinName.IsNone())
		{
			// Segment starting with an entry node runs in the graph created by the SubGraph node closing the previous segment
			const UFlowAsset* PreviousInstance = SegmentInstances[InSegmentIndex - 1].Get();
			const UFlowNode* SubGraphNode = PreviousInstance ? PreviousInstance->GetNode(Route.Segments[InSegmentIndex - 1].Steps.Last().NodeGuid) : nullptr;
			bSegmentInstance = SubGraphNode && Instance->GetNodeOwningThisAssetInstance() == SubGraphNode;
		}
	}

	return bSegmentInstance;
}

void UFlowFastForwardSubsystem::AdvanceStep()
{
	++StepIndex;

	// Segment of an entry node chosen as the target contains only this entry node
	while (Route.Segments.IsValidIndex(SegmentIndex) && StepIndex >= Route.Segments[SegmentIndex].Steps.Num())
	{
		++SegmentIndex;
		StepIndex = 0;

		// Entry node of the next graph is triggered by the graph itself, not by its input
		if (Route.Segments.IsValidIndex(SegmentIndex) && Route.Segments[SegmentIndex].Steps[0].InputPinName.IsNone())
		{
			StepIndex = 1;
		}
		else if (Route.Segments.IsValidIndex(SegmentIndex))
		{
			// Signal leaves the graph to its parent, while the graph still exists
			const UFlowAsset* PreviousInstance = SegmentInstances[SegmentIndex - 1].Get();
			SegmentInstances[SegmentIndex] = PreviousInstance ? PreviousInstance->GetParentInstance() : nullptr;
		}
	}

	if (SegmentIndex >= Route.Segments.Num())
	{
		StopFastForward();
	}
	else if (const UGameInstance* GameInstance = GetGameInstance())
	{
		// Every reached step restarts the countdown
		GameInstance->GetTimerManager().SetTimer(StallTimerHandle, this, &ThisClass::OnStalled, StallTimeout, false);
	}
}

void UFlowFastForwardSubsystem::ReportSignalOutsideRoute(const UFlowNode& Node, const FName& PinName) const
{
	bool bReported = false;

	for (int32 RouteSegmentIndex = FMath::Max(SegmentIndex, 0); RouteSegmentIndex < Route.Segments.Num() && !bReported; ++RouteSegmentIndex)
	{
		const int32 FirstStepIndex = RouteSegmentIndex == SegmentIndex ? StepIndex : 0;
		for (int32 RouteStepIndex = FirstStepIndex; RouteStepIndex < Route.Segments[RouteSegmentIndex].Steps.Num() && !bReported; ++RouteStepIndex)
		{
			if (IsRouteNode(Node, RouteSegmentIndex, RouteStepIndex))
			{
				bReported = true;

				UE_LOG(LogFlow, Error, TEXT("Flow Fast Forward: node %s (%s) of the route received input %s from outside the route, before the route reached it. ")
					TEXT("Fast Forward waits for segment %d, step %d and might never reach the target node. Implement FastForwardInput of the nodes involved or report the case on Flow Discord."),
					*Node.GetName(), *Node.GetClass()->GetName(), *PinName.ToString(), SegmentIndex, StepIndex);
			}
		}
	}
}

void UFlowFastForwardSubsystem::OnStalled()
{
	FString ExpectedNodeDescription = TEXT("unknown node");

	if (Route.Segments.IsValidIndex(SegmentIndex) && Route.Segments[SegmentIndex].Steps.IsValidIndex(StepIndex))
	{
		const FFlowFastForwardSegment& Segment = Route.Segments[SegmentIndex];
		const UFlowAsset* Template = Segment.TemplateAsset.Get();
		const UFlowNode* ExpectedNode = Template ? Template->GetNode(Segment.Steps[StepIndex].NodeGuid) : nullptr;

		if (ExpectedNode)
		{
			ExpectedNodeDescription = FString::Printf(TEXT("%s (%s) in %s"), *ExpectedNode->GetName(), *ExpectedNode->GetClass()->GetName(), *Template->GetName());
		}
	}

	UE_LOG(LogFlow, Error, TEXT("Flow Fast Forward hasn't reached the next node of the route for %.0f seconds, it waits for %s at segment %d, step %d. ")
		TEXT("The previous node of the route didn't pass the signal, its FastForwardInput might wait for something that doesn't happen. Implement FastForwardInput of this node or report the case on Flow Discord."),
		StallTimeout, *ExpectedNodeDescription, SegmentIndex, StepIndex);
}
