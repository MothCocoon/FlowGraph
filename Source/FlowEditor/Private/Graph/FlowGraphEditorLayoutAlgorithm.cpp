// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphEditorLayoutAlgorithm.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphNode_Comment.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/FlowPin.h"
#include "UObject/Package.h"

namespace UE::FlowGraph::Private
{
namespace
{
constexpr int32 HorizontalSpacing = 160;
constexpr int32 VerticalSpacing = 80;
constexpr int32 ComponentSpacing = 240;
constexpr int32 CommentHorizontalPadding = 64;
constexpr int32 CommentTopPadding = 96;
constexpr int32 CommentBottomPadding = 64;
constexpr int32 MinimumNodeWidth = 260;
constexpr int32 MinimumNodeHeight = 100;
constexpr int32 EstimatedNodeWidth = 380;
constexpr int32 EstimatedNodeBaseHeight = 120;
constexpr int32 EstimatedPinHeight = 28;
constexpr int32 EstimatedAddOnHeight = 48;
constexpr int32 CollisionNudge = 16;
constexpr int32 MaximumCollisionNudges = 4096;
constexpr int32 OrderingSweepCount = 8;
constexpr double PinIndexWeight = 1.0 / 100.0;

struct FLayoutNode
{
	FGuid Guid;
	FIntPoint OriginalPosition = FIntPoint::ZeroValue;
	FIntPoint Size = FIntPoint(MinimumNodeWidth, MinimumNodeHeight);
	int32 Rank = 0;
	int32 Order = 0;
};

struct FLayoutEdge
{
	FGuid Source;
	FGuid Target;
	int32 SourcePinIndex = 0;
};

struct FLayoutPartition
{
	FGuid CommentGuid;
	TArray<FGuid> Nodes;
	FIntPoint OriginalOrigin = FIntPoint::ZeroValue;
};

struct FCommentSnapshot
{
	TWeakObjectPtr<UEdGraphNode_Comment> Comment;
	TArray<FGuid> Members;
	int64 Area = 0;
};

bool IsGuidLess(const FGuid& A, const FGuid& B)
{
	return A.ToString() < B.ToString();
}

FIntRect MakeRect(const FIntPoint& Position, const FIntPoint& Size)
{
	return FIntRect(Position, Position + Size);
}

bool RectsOverlap(const FIntRect& A, const FIntRect& B)
{
	return A.Min.X < B.Max.X && A.Max.X > B.Min.X && A.Min.Y < B.Max.Y && A.Max.Y > B.Min.Y;
}

template <typename NodeBaseType>
int32 CountAddOns(const NodeBaseType* NodeBase)
{
	if (!NodeBase)
	{
		return 0;
	}

	int32 Count = 0;
	for (const UFlowNodeAddOn* AddOn : NodeBase->GetFlowNodeAddOnChildren())
	{
		++Count;
		Count += CountAddOns(AddOn);
	}
	return Count;
}

FIntPoint EstimateNodeSize(const UFlowNode* Node, const TMap<FGuid, FIntPoint>& MeasuredNodeSizes)
{
	if (!Node)
	{
		return FIntPoint(MinimumNodeWidth, MinimumNodeHeight);
	}

	if (const FIntPoint* MeasuredSize = MeasuredNodeSizes.Find(Node->GetGuid()))
	{
		return FIntPoint(
			FMath::Max(MeasuredSize->X, MinimumNodeWidth),
			FMath::Max(MeasuredSize->Y, MinimumNodeHeight));
	}

	const UEdGraphNode* GraphNode = Node->GetGraphNode();
	if (GraphNode && GraphNode->NodeWidth > 0 && GraphNode->NodeHeight > 0)
	{
		return FIntPoint(
			FMath::Max(GraphNode->NodeWidth, MinimumNodeWidth),
			FMath::Max(GraphNode->NodeHeight, MinimumNodeHeight));
	}

	int32 InputPinCount = 0;
	int32 OutputPinCount = 0;
	if (GraphNode)
	{
		for (const UEdGraphPin* Pin : GraphNode->Pins)
		{
			if (!Pin)
			{
				continue;
			}
			if (Pin->Direction == EGPD_Input)
			{
				++InputPinCount;
			}
			else
			{
				++OutputPinCount;
			}
		}
	}

	const int32 PinRows = FMath::Max(InputPinCount, OutputPinCount);
	const int32 Height = EstimatedNodeBaseHeight + PinRows * EstimatedPinHeight
		+ CountAddOns(Node) * EstimatedAddOnHeight;
	return FIntPoint(EstimatedNodeWidth, FMath::Max(Height, MinimumNodeHeight));
}

TArray<FCommentSnapshot> GatherComments(
	const UFlowAsset* FlowAsset,
	const TMap<FGuid, FLayoutNode>& LayoutNodes)
{
	TArray<FCommentSnapshot> Comments;
	const UEdGraph* Graph = FlowAsset ? FlowAsset->GetGraph() : nullptr;
	if (!Graph)
	{
		return Comments;
	}

	for (UEdGraphNode* GraphNode : Graph->Nodes)
	{
		UEdGraphNode_Comment* Comment = Cast<UEdGraphNode_Comment>(GraphNode);
		if (!Comment || Comment->NodeWidth <= 0 || Comment->NodeHeight <= 0)
		{
			continue;
		}

		FCommentSnapshot& Snapshot = Comments.AddDefaulted_GetRef();
		Snapshot.Comment = Comment;
		Snapshot.Area = static_cast<int64>(Comment->NodeWidth) * Comment->NodeHeight;
		const FIntRect CommentRect(
			Comment->NodePosX,
			Comment->NodePosY,
			Comment->NodePosX + Comment->NodeWidth,
			Comment->NodePosY + Comment->NodeHeight);

		for (const UObject* Object : Comment->GetNodesUnderComment())
		{
			const UEdGraphNode* NodeUnderComment = Cast<UEdGraphNode>(Object);
			if (NodeUnderComment && LayoutNodes.Contains(NodeUnderComment->NodeGuid))
			{
				Snapshot.Members.AddUnique(NodeUnderComment->NodeGuid);
			}
		}

		if (Snapshot.Members.IsEmpty())
		{
			for (const TPair<FGuid, FLayoutNode>& Pair : LayoutNodes)
			{
				const FIntRect NodeRect = MakeRect(Pair.Value.OriginalPosition, Pair.Value.Size);
				if (RectsOverlap(CommentRect, NodeRect))
				{
					Snapshot.Members.Add(Pair.Key);
				}
			}
		}
		Snapshot.Members.Sort(IsGuidLess);
	}

	Comments.Sort([](const FCommentSnapshot& A, const FCommentSnapshot& B)
	{
		if (A.Area != B.Area)
		{
			return A.Area < B.Area;
		}
		const UEdGraphNode_Comment* CommentA = A.Comment.Get();
		const UEdGraphNode_Comment* CommentB = B.Comment.Get();
		return CommentA && CommentB && IsGuidLess(CommentA->NodeGuid, CommentB->NodeGuid);
	});
	return Comments;
}

void ResolveNodes(
	const UFlowAsset* FlowAsset,
	const TSet<FGuid>& TargetGuids,
	const TMap<FGuid, FIntPoint>& MeasuredNodeSizes,
	TMap<FGuid, FLayoutNode>& OutNodes)
{
	OutNodes.Reset();
	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (!Pair.Value || (!TargetGuids.IsEmpty() && !TargetGuids.Contains(Pair.Key)))
		{
			continue;
		}

		const UEdGraphNode* GraphNode = Pair.Value->GetGraphNode();
		FLayoutNode& LayoutNode = OutNodes.Add(Pair.Key);
		LayoutNode.Guid = Pair.Key;
		LayoutNode.OriginalPosition = GraphNode
			? FIntPoint(GraphNode->NodePosX, GraphNode->NodePosY)
			: FIntPoint::ZeroValue;
		LayoutNode.Size = EstimateNodeSize(Pair.Value, MeasuredNodeSizes);
	}
}

void AddEdge(
	const FGuid& Source,
	const FGuid& Target,
	int32 SourcePinIndex,
	const TMap<FGuid, FLayoutNode>& Nodes,
	TArray<FLayoutEdge>& OutEdges)
{
	if (!Source.IsValid() || !Target.IsValid() || Source == Target
		|| !Nodes.Contains(Source) || !Nodes.Contains(Target))
	{
		return;
	}

	for (const FLayoutEdge& Edge : OutEdges)
	{
		if (Edge.Source == Source && Edge.Target == Target)
		{
			return;
		}
	}
	OutEdges.Add({ Source, Target, SourcePinIndex });
}

void BuildEdges(const UFlowAsset* FlowAsset, const TMap<FGuid, FLayoutNode>& Nodes, TArray<FLayoutEdge>& OutEdges)
{
	OutEdges.Reset();
	TArray<FGuid> SortedGuids;
	Nodes.GetKeys(SortedGuids);
	SortedGuids.Sort(IsGuidLess);

	for (const FGuid& Guid : SortedGuids)
	{
		const UFlowNode* Node = FlowAsset->GetNode(Guid);
		if (!Node)
		{
			continue;
		}

		int32 ExecOutputPinIndex = 0;
		for (const FFlowPin& OutputPin : Node->GetOutputPins())
		{
			if (!OutputPin.IsExecPin())
			{
				continue;
			}
			const FConnectedPin Connection = Node->GetConnection(OutputPin.PinName);
			AddEdge(Guid, Connection.NodeGuid, ExecOutputPinIndex++, Nodes, OutEdges);
		}

		for (const FFlowPin& InputPin : Node->GetInputPins())
		{
			if (InputPin.IsExecPin())
			{
				continue;
			}
			const FConnectedPin Connection = Node->GetConnection(InputPin.PinName);
			if (!Connection.NodeGuid.IsValid())
			{
				continue;
			}

			int32 DataOutputPinIndex = 0;
			if (const UFlowNode* SourceNode = FlowAsset->GetNode(Connection.NodeGuid))
			{
				for (const FFlowPin& SourceOutputPin : SourceNode->GetOutputPins())
				{
					if (SourceOutputPin.IsExecPin())
					{
						continue;
					}
					if (SourceOutputPin.PinName == Connection.PinName)
					{
						break;
					}
					++DataOutputPinIndex;
				}
			}
			AddEdge(Connection.NodeGuid, Guid, DataOutputPinIndex, Nodes, OutEdges);
		}
	}
}

void BuildPartitions(
	const TMap<FGuid, FLayoutNode>& Nodes,
	const TArray<FLayoutEdge>& Edges,
	const TArray<FCommentSnapshot>& Comments,
	TArray<FLayoutPartition>& OutPartitions)
{
	OutPartitions.Reset();
	TMap<FGuid, FGuid> NodeToComment;
	for (const TPair<FGuid, FLayoutNode>& Pair : Nodes)
	{
		const FCommentSnapshot* OutermostComment = nullptr;
		for (const FCommentSnapshot& Comment : Comments)
		{
			if (Comment.Members.Contains(Pair.Key)
				&& (!OutermostComment || Comment.Area > OutermostComment->Area))
			{
				OutermostComment = &Comment;
			}
		}
		if (OutermostComment && OutermostComment->Comment.IsValid())
		{
			NodeToComment.Add(Pair.Key, OutermostComment->Comment->NodeGuid);
		}
	}

	TMap<FGuid, TArray<FGuid>> UndirectedNeighbors;
	for (const FLayoutEdge& Edge : Edges)
	{
		UndirectedNeighbors.FindOrAdd(Edge.Source).AddUnique(Edge.Target);
		UndirectedNeighbors.FindOrAdd(Edge.Target).AddUnique(Edge.Source);
	}

	TSet<FGuid> Visited;
	TArray<FGuid> SortedGuids;
	Nodes.GetKeys(SortedGuids);
	SortedGuids.Sort([&Nodes](const FGuid& A, const FGuid& B)
	{
		const FLayoutNode& NodeA = Nodes.FindChecked(A);
		const FLayoutNode& NodeB = Nodes.FindChecked(B);
		if (NodeA.OriginalPosition.Y != NodeB.OriginalPosition.Y)
		{
			return NodeA.OriginalPosition.Y < NodeB.OriginalPosition.Y;
		}
		if (NodeA.OriginalPosition.X != NodeB.OriginalPosition.X)
		{
			return NodeA.OriginalPosition.X < NodeB.OriginalPosition.X;
		}
		return IsGuidLess(A, B);
	});

	for (const FGuid& StartGuid : SortedGuids)
	{
		if (Visited.Contains(StartGuid))
		{
			continue;
		}

		FLayoutPartition& Partition = OutPartitions.AddDefaulted_GetRef();
		Partition.CommentGuid = NodeToComment.FindRef(StartGuid);
		TArray<FGuid> Pending({ StartGuid });
		while (!Pending.IsEmpty())
		{
			const FGuid Guid = Pending.Pop(EAllowShrinking::No);
			if (Visited.Contains(Guid) || NodeToComment.FindRef(Guid) != Partition.CommentGuid)
			{
				continue;
			}
			Visited.Add(Guid);
			Partition.Nodes.Add(Guid);

			if (const TArray<FGuid>* Neighbors = UndirectedNeighbors.Find(Guid))
			{
				for (const FGuid& Neighbor : *Neighbors)
				{
					Pending.Add(Neighbor);
				}
			}
		}

		Partition.Nodes.Sort(IsGuidLess);
		Partition.OriginalOrigin = Nodes.FindChecked(Partition.Nodes[0]).OriginalPosition;
		for (const FGuid& Guid : Partition.Nodes)
		{
			const FIntPoint Position = Nodes.FindChecked(Guid).OriginalPosition;
			Partition.OriginalOrigin.X = FMath::Min(Partition.OriginalOrigin.X, Position.X);
			Partition.OriginalOrigin.Y = FMath::Min(Partition.OriginalOrigin.Y, Position.Y);
		}
	}

	OutPartitions.Sort([](const FLayoutPartition& A, const FLayoutPartition& B)
	{
		if (A.OriginalOrigin.Y != B.OriginalOrigin.Y)
		{
			return A.OriginalOrigin.Y < B.OriginalOrigin.Y;
		}
		if (A.OriginalOrigin.X != B.OriginalOrigin.X)
		{
			return A.OriginalOrigin.X < B.OriginalOrigin.X;
		}
		return IsGuidLess(A.Nodes[0], B.Nodes[0]);
	});
}

void FindStronglyConnectedComponents(
	const TArray<FGuid>& PartitionNodes,
	const TArray<FLayoutEdge>& Edges,
	TMap<FGuid, int32>& OutComponentByNode,
	TArray<TArray<FGuid>>& OutComponents)
{
	TSet<FGuid> PartitionSet(PartitionNodes);
	TMap<FGuid, TArray<FGuid>> Successors;
	for (const FLayoutEdge& Edge : Edges)
	{
		if (PartitionSet.Contains(Edge.Source) && PartitionSet.Contains(Edge.Target))
		{
			Successors.FindOrAdd(Edge.Source).AddUnique(Edge.Target);
		}
	}

	TMap<FGuid, int32> IndexByNode;
	TMap<FGuid, int32> LowLinkByNode;
	TArray<FGuid> Stack;
	TSet<FGuid> OnStack;
	int32 NextIndex = 0;

	TFunction<void(const FGuid&)> Visit = [&](const FGuid& Guid)
	{
		IndexByNode.Add(Guid, NextIndex);
		LowLinkByNode.Add(Guid, NextIndex++);
		Stack.Add(Guid);
		OnStack.Add(Guid);

		TArray<FGuid> SortedSuccessors = Successors.FindRef(Guid);
		SortedSuccessors.Sort(IsGuidLess);
		for (const FGuid& Successor : SortedSuccessors)
		{
			if (!IndexByNode.Contains(Successor))
			{
				Visit(Successor);
				LowLinkByNode.FindChecked(Guid) = FMath::Min(
					LowLinkByNode.FindChecked(Guid), LowLinkByNode.FindChecked(Successor));
			}
			else if (OnStack.Contains(Successor))
			{
				LowLinkByNode.FindChecked(Guid) = FMath::Min(
					LowLinkByNode.FindChecked(Guid), IndexByNode.FindChecked(Successor));
			}
		}

		if (LowLinkByNode.FindChecked(Guid) != IndexByNode.FindChecked(Guid))
		{
			return;
		}

		TArray<FGuid>& Component = OutComponents.AddDefaulted_GetRef();
		while (!Stack.IsEmpty())
		{
			const FGuid Member = Stack.Pop(EAllowShrinking::No);
			OnStack.Remove(Member);
			Component.Add(Member);
			if (Member == Guid)
			{
				break;
			}
		}
		Component.Sort(IsGuidLess);
	};

	for (const FGuid& Guid : PartitionNodes)
	{
		if (!IndexByNode.Contains(Guid))
		{
			Visit(Guid);
		}
	}

	for (int32 ComponentIndex = 0; ComponentIndex < OutComponents.Num(); ++ComponentIndex)
	{
		for (const FGuid& Guid : OutComponents[ComponentIndex])
		{
			OutComponentByNode.Add(Guid, ComponentIndex);
		}
	}
}

void AssignRanks(
	const FLayoutPartition& Partition,
	const TArray<FLayoutEdge>& Edges,
	TMap<FGuid, FLayoutNode>& Nodes)
{
	TMap<FGuid, int32> ComponentByNode;
	TArray<TArray<FGuid>> Components;
	FindStronglyConnectedComponents(Partition.Nodes, Edges, ComponentByNode, Components);

	TMap<FGuid, int32> CycleOrder;
	for (const TArray<FGuid>& Component : Components)
	{
		TArray<FGuid> OrderedMembers = Component;
		OrderedMembers.Sort([&Nodes](const FGuid& A, const FGuid& B)
		{
			const FLayoutNode& NodeA = Nodes.FindChecked(A);
			const FLayoutNode& NodeB = Nodes.FindChecked(B);
			if (NodeA.OriginalPosition.X != NodeB.OriginalPosition.X)
			{
				return NodeA.OriginalPosition.X < NodeB.OriginalPosition.X;
			}
			if (NodeA.OriginalPosition.Y != NodeB.OriginalPosition.Y)
			{
				return NodeA.OriginalPosition.Y < NodeB.OriginalPosition.Y;
			}
			return IsGuidLess(A, B);
		});
		for (int32 MemberIndex = 0; MemberIndex < OrderedMembers.Num(); ++MemberIndex)
		{
			CycleOrder.Add(OrderedMembers[MemberIndex], MemberIndex);
		}
	}

	TSet<FGuid> PartitionSet(Partition.Nodes);
	TMap<FGuid, TArray<FGuid>> ForwardSuccessors;
	TMap<FGuid, int32> InDegree;
	for (const FGuid& Guid : Partition.Nodes)
	{
		InDegree.Add(Guid, 0);
		Nodes.FindChecked(Guid).Rank = 0;
	}

	for (const FLayoutEdge& Edge : Edges)
	{
		if (!PartitionSet.Contains(Edge.Source) || !PartitionSet.Contains(Edge.Target))
		{
			continue;
		}

		const bool bSameComponent = ComponentByNode.FindChecked(Edge.Source)
			== ComponentByNode.FindChecked(Edge.Target);
		if (bSameComponent && CycleOrder.FindChecked(Edge.Source) >= CycleOrder.FindChecked(Edge.Target))
		{
			continue;
		}

		ForwardSuccessors.FindOrAdd(Edge.Source).AddUnique(Edge.Target);
		++InDegree.FindChecked(Edge.Target);
	}

	TArray<FGuid> Ready;
	for (const FGuid& Guid : Partition.Nodes)
	{
		if (InDegree.FindChecked(Guid) == 0)
		{
			Ready.Add(Guid);
		}
	}
	Ready.Sort(IsGuidLess);

	while (!Ready.IsEmpty())
	{
		const FGuid Guid = Ready[0];
		Ready.RemoveAt(0, EAllowShrinking::No);
		TArray<FGuid> Successors = ForwardSuccessors.FindRef(Guid);
		Successors.Sort(IsGuidLess);
		for (const FGuid& Successor : Successors)
		{
			Nodes.FindChecked(Successor).Rank = FMath::Max(
				Nodes.FindChecked(Successor).Rank, Nodes.FindChecked(Guid).Rank + 1);
			if (--InDegree.FindChecked(Successor) == 0)
			{
				Ready.Add(Successor);
				Ready.Sort(IsGuidLess);
			}
		}
	}
}

void OrderRanks(
	const FLayoutPartition& Partition,
	const TArray<FLayoutEdge>& Edges,
	TMap<FGuid, FLayoutNode>& Nodes,
	TArray<TArray<FGuid>>& OutRankGroups)
{
	int32 MaxRank = 0;
	for (const FGuid& Guid : Partition.Nodes)
	{
		MaxRank = FMath::Max(MaxRank, Nodes.FindChecked(Guid).Rank);
	}
	OutRankGroups.SetNum(MaxRank + 1);

	for (const FGuid& Guid : Partition.Nodes)
	{
		OutRankGroups[Nodes.FindChecked(Guid).Rank].Add(Guid);
	}
	for (TArray<FGuid>& Group : OutRankGroups)
	{
		Group.Sort([&Nodes](const FGuid& A, const FGuid& B)
		{
			const FLayoutNode& NodeA = Nodes.FindChecked(A);
			const FLayoutNode& NodeB = Nodes.FindChecked(B);
			if (NodeA.OriginalPosition.Y != NodeB.OriginalPosition.Y)
			{
				return NodeA.OriginalPosition.Y < NodeB.OriginalPosition.Y;
			}
			return IsGuidLess(A, B);
		});
		for (int32 OrderIndex = 0; OrderIndex < Group.Num(); ++OrderIndex)
		{
			Nodes.FindChecked(Group[OrderIndex]).Order = OrderIndex;
		}
	}

	TSet<FGuid> PartitionSet(Partition.Nodes);
	TMap<FGuid, TArray<FLayoutEdge>> IncomingEdges;
	TMap<FGuid, TArray<FLayoutEdge>> OutgoingEdges;
	for (const FLayoutEdge& Edge : Edges)
	{
		if (PartitionSet.Contains(Edge.Source) && PartitionSet.Contains(Edge.Target))
		{
			IncomingEdges.FindOrAdd(Edge.Target).Add(Edge);
			OutgoingEdges.FindOrAdd(Edge.Source).Add(Edge);
		}
	}

	for (int32 Sweep = 0; Sweep < OrderingSweepCount; ++Sweep)
	{
		const bool bDownSweep = Sweep % 2 == 0;
		const int32 StartRank = bDownSweep ? 0 : OutRankGroups.Num() - 1;
		const int32 EndRank = bDownSweep ? OutRankGroups.Num() : -1;
		const int32 Step = bDownSweep ? 1 : -1;
		for (int32 RankIndex = StartRank; RankIndex != EndRank; RankIndex += Step)
		{
			TArray<FGuid>& Group = OutRankGroups[RankIndex];
			if (Group.Num() <= 1)
			{
				continue;
			}

			TMap<FGuid, double> Signals;
			for (const FGuid& Guid : Group)
			{
				const TArray<FLayoutEdge>& NeighborEdges = bDownSweep
					? IncomingEdges.FindRef(Guid)
					: OutgoingEdges.FindRef(Guid);
				TArray<double> NeighborSignals;
				for (const FLayoutEdge& Edge : NeighborEdges)
				{
					const FGuid& Neighbor = bDownSweep ? Edge.Source : Edge.Target;
					NeighborSignals.Add(Nodes.FindChecked(Neighbor).Order
						+ Edge.SourcePinIndex * PinIndexWeight);
				}
				if (NeighborSignals.IsEmpty())
				{
					Signals.Add(Guid, Nodes.FindChecked(Guid).Order);
					continue;
				}
				NeighborSignals.Sort();
				const int32 Count = NeighborSignals.Num();
				Signals.Add(Guid, Count % 2 == 1
					? NeighborSignals[Count / 2]
					: (NeighborSignals[Count / 2 - 1] + NeighborSignals[Count / 2]) / 2.0);
			}

			Group.Sort([&Signals](const FGuid& A, const FGuid& B)
			{
				const double SignalA = Signals.FindChecked(A);
				const double SignalB = Signals.FindChecked(B);
				return !FMath::IsNearlyEqual(SignalA, SignalB) ? SignalA < SignalB : IsGuidLess(A, B);
			});
			for (int32 OrderIndex = 0; OrderIndex < Group.Num(); ++OrderIndex)
			{
				Nodes.FindChecked(Group[OrderIndex]).Order = OrderIndex;
			}
		}
	}
}

FIntPoint PlacePartition(
	const FLayoutPartition& Partition,
	const TArray<FLayoutEdge>& Edges,
	TMap<FGuid, FLayoutNode>& Nodes,
	const FIntPoint& Origin,
	TMap<FGuid, FIntPoint>& OutPositions)
{
	AssignRanks(Partition, Edges, Nodes);
	TArray<TArray<FGuid>> RankGroups;
	OrderRanks(Partition, Edges, Nodes, RankGroups);

	TArray<int32> ColumnX;
	ColumnX.SetNum(RankGroups.Num());
	int32 CurrentX = Origin.X;
	for (int32 RankIndex = 0; RankIndex < RankGroups.Num(); ++RankIndex)
	{
		ColumnX[RankIndex] = CurrentX;
		int32 ColumnWidth = MinimumNodeWidth;
		for (const FGuid& Guid : RankGroups[RankIndex])
		{
			ColumnWidth = FMath::Max(ColumnWidth, Nodes.FindChecked(Guid).Size.X);
		}
		CurrentX += ColumnWidth + HorizontalSpacing;
	}

	int32 Bottom = Origin.Y;
	for (int32 RankIndex = 0; RankIndex < RankGroups.Num(); ++RankIndex)
	{
		int32 CurrentY = Origin.Y;
		for (const FGuid& Guid : RankGroups[RankIndex])
		{
			OutPositions.Add(Guid, FIntPoint(ColumnX[RankIndex], CurrentY));
			CurrentY += Nodes.FindChecked(Guid).Size.Y + VerticalSpacing;
		}
		Bottom = FMath::Max(Bottom, CurrentY - VerticalSpacing);
	}
	return FIntPoint(CurrentX - HorizontalSpacing, Bottom);
}

FIntRect GetPartitionBounds(
	const FLayoutPartition& Partition,
	const TMap<FGuid, FLayoutNode>& Nodes,
	const TMap<FGuid, FIntPoint>& Positions)
{
	FIntRect Bounds = MakeRect(
		Positions.FindChecked(Partition.Nodes[0]),
		Nodes.FindChecked(Partition.Nodes[0]).Size);
	for (const FGuid& Guid : Partition.Nodes)
	{
		const FIntRect NodeRect = MakeRect(Positions.FindChecked(Guid), Nodes.FindChecked(Guid).Size);
		Bounds.Min.X = FMath::Min(Bounds.Min.X, NodeRect.Min.X);
		Bounds.Min.Y = FMath::Min(Bounds.Min.Y, NodeRect.Min.Y);
		Bounds.Max.X = FMath::Max(Bounds.Max.X, NodeRect.Max.X);
		Bounds.Max.Y = FMath::Max(Bounds.Max.Y, NodeRect.Max.Y);
	}

	if (Partition.CommentGuid.IsValid())
	{
		Bounds.Min.X -= CommentHorizontalPadding;
		Bounds.Min.Y -= CommentTopPadding;
		Bounds.Max.X += CommentHorizontalPadding;
		Bounds.Max.Y += CommentBottomPadding;
	}
	return Bounds;
}

void TranslatePartition(
	const FLayoutPartition& Partition,
	const FIntPoint& Translation,
	TMap<FGuid, FIntPoint>& InOutPositions)
{
	for (const FGuid& Guid : Partition.Nodes)
	{
		InOutPositions.FindChecked(Guid) += Translation;
	}
}

void ResolvePartitionCollisions(
	const TArray<FLayoutPartition>& Partitions,
	const TMap<FGuid, FLayoutNode>& Nodes,
	TMap<FGuid, FIntPoint>& InOutPositions)
{
	TArray<FIntRect> PlacedBounds;
	TArray<FIntPoint> OriginalAnchors;
	for (const FLayoutPartition& Partition : Partitions)
	{
		FIntRect Bounds = GetPartitionBounds(Partition, Nodes, InOutPositions);
		bool bOverlaps = false;
		for (const FIntRect& OtherBounds : PlacedBounds)
		{
			if (RectsOverlap(Bounds, OtherBounds))
			{
				bOverlaps = true;
				break;
			}
		}

		if (bOverlaps)
		{
			int32 MoveLeft = 0;
			int32 MoveRight = 0;
			int32 MoveUp = 0;
			int32 MoveDown = 0;
			for (const FIntRect& OtherBounds : PlacedBounds)
			{
				if (Bounds.Min.Y < OtherBounds.Max.Y && Bounds.Max.Y > OtherBounds.Min.Y)
				{
					MoveLeft = FMath::Min(MoveLeft, OtherBounds.Min.X - ComponentSpacing - Bounds.Max.X);
					MoveRight = FMath::Max(MoveRight, OtherBounds.Max.X + ComponentSpacing - Bounds.Min.X);
				}
				if (Bounds.Min.X < OtherBounds.Max.X && Bounds.Max.X > OtherBounds.Min.X)
				{
					MoveUp = FMath::Min(MoveUp, OtherBounds.Min.Y - ComponentSpacing - Bounds.Max.Y);
					MoveDown = FMath::Max(MoveDown, OtherBounds.Max.Y + ComponentSpacing - Bounds.Min.Y);
				}
			}

			const FIntPoint OriginalAnchor = Partition.OriginalOrigin;
			FIntPoint ReferenceAnchor = OriginalAnchors[0];
			int64 ClosestDistance = MAX_int64;
			for (const FIntPoint& OtherAnchor : OriginalAnchors)
			{
				const FIntPoint Delta = OriginalAnchor - OtherAnchor;
				const int64 Distance = static_cast<int64>(Delta.X) * Delta.X
					+ static_cast<int64>(Delta.Y) * Delta.Y;
				if (Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					ReferenceAnchor = OtherAnchor;
				}
			}

			const FIntPoint OriginalDelta = OriginalAnchor - ReferenceAnchor;
			const TArray<FIntPoint> Candidates({
				FIntPoint(MoveLeft, 0),
				FIntPoint(MoveRight, 0),
				FIntPoint(0, MoveUp),
				FIntPoint(0, MoveDown)
			});
			FIntPoint BestTranslation = Candidates[0];
			int64 BestScore = MAX_int64;
			for (const FIntPoint& Candidate : Candidates)
			{
				const int64 Distance = FMath::Abs(static_cast<int64>(Candidate.X))
					+ FMath::Abs(static_cast<int64>(Candidate.Y));
				const bool bSameDirection = static_cast<int64>(Candidate.X) * OriginalDelta.X > 0
					|| static_cast<int64>(Candidate.Y) * OriginalDelta.Y > 0;
				const int64 DirectionPenalty = bSameDirection ? 0 : ComponentSpacing;
				const int64 Score = Distance + DirectionPenalty;
				if (Score < BestScore)
				{
					BestScore = Score;
					BestTranslation = Candidate;
				}
			}

			TranslatePartition(Partition, BestTranslation, InOutPositions);
			Bounds.Min += BestTranslation;
			Bounds.Max += BestTranslation;
		}

		PlacedBounds.Add(Bounds);
		OriginalAnchors.Add(Partition.OriginalOrigin);
	}
}

void AvoidUnselectedNodes(
	const UFlowAsset* FlowAsset,
	const TSet<FGuid>& TargetGuids,
	const TMap<FGuid, FLayoutNode>& LayoutNodes,
	const TMap<FGuid, FIntPoint>& MeasuredNodeSizes,
	TMap<FGuid, FIntPoint>& InOutPositions)
{
	if (TargetGuids.IsEmpty())
	{
		return;
	}

	TArray<FIntRect> Obstacles;
	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (!Pair.Value || LayoutNodes.Contains(Pair.Key))
		{
			continue;
		}
		const UEdGraphNode* GraphNode = Pair.Value->GetGraphNode();
		if (GraphNode)
		{
			Obstacles.Add(MakeRect(
				FIntPoint(GraphNode->NodePosX, GraphNode->NodePosY),
				EstimateNodeSize(Pair.Value, MeasuredNodeSizes)));
		}
	}

	for (int32 NudgeCount = 0; NudgeCount < MaximumCollisionNudges; ++NudgeCount)
	{
		bool bOverlaps = false;
		for (const TPair<FGuid, FIntPoint>& Pair : InOutPositions)
		{
			const FIntRect NodeRect = MakeRect(Pair.Value, LayoutNodes.FindChecked(Pair.Key).Size);
			for (const FIntRect& Obstacle : Obstacles)
			{
				if (RectsOverlap(NodeRect, Obstacle))
				{
					bOverlaps = true;
					break;
				}
			}
			if (bOverlaps)
			{
				break;
			}
		}

		if (!bOverlaps)
		{
			return;
		}
		for (TPair<FGuid, FIntPoint>& Pair : InOutPositions)
		{
			Pair.Value.Y += CollisionNudge;
		}
	}
}

bool ResizeFullyTargetedComments(
	const TArray<FCommentSnapshot>& Comments,
	const TMap<FGuid, FIntPoint>& Positions,
	const TMap<FGuid, FLayoutNode>& LayoutNodes)
{
	bool bAnyChanged = false;
	for (const FCommentSnapshot& Snapshot : Comments)
	{
		UEdGraphNode_Comment* Comment = Snapshot.Comment.Get();
		if (!Comment || Snapshot.Members.IsEmpty())
		{
			continue;
		}

		bool bAllMembersTargeted = true;
		for (const FGuid& Guid : Snapshot.Members)
		{
			if (!Positions.Contains(Guid))
			{
				bAllMembersTargeted = false;
				break;
			}
		}
		if (!bAllMembersTargeted)
		{
			continue;
		}

		FIntRect Bounds = MakeRect(
			Positions.FindChecked(Snapshot.Members[0]),
			LayoutNodes.FindChecked(Snapshot.Members[0]).Size);
		for (const FGuid& Guid : Snapshot.Members)
		{
			const FIntRect NodeRect = MakeRect(Positions.FindChecked(Guid), LayoutNodes.FindChecked(Guid).Size);
			Bounds.Min.X = FMath::Min(Bounds.Min.X, NodeRect.Min.X);
			Bounds.Min.Y = FMath::Min(Bounds.Min.Y, NodeRect.Min.Y);
			Bounds.Max.X = FMath::Max(Bounds.Max.X, NodeRect.Max.X);
			Bounds.Max.Y = FMath::Max(Bounds.Max.Y, NodeRect.Max.Y);
		}

		const int32 NewX = Bounds.Min.X - CommentHorizontalPadding;
		const int32 NewY = Bounds.Min.Y - CommentTopPadding;
		const int32 NewWidth = Bounds.Width() + CommentHorizontalPadding * 2;
		const int32 NewHeight = Bounds.Height() + CommentTopPadding + CommentBottomPadding;
		if (Comment->NodePosX != NewX || Comment->NodePosY != NewY
			|| Comment->NodeWidth != NewWidth || Comment->NodeHeight != NewHeight)
		{
			Comment->Modify();
			Comment->NodePosX = NewX;
			Comment->NodePosY = NewY;
			Comment->NodeWidth = NewWidth;
			Comment->NodeHeight = NewHeight;
			bAnyChanged = true;
		}
	}
	return bAnyChanged;
}
} // namespace

void FFlowGraphEditorLayoutAlgorithm::Compute(
	const UFlowAsset* FlowAsset,
	const TSet<FGuid>& TargetGuids,
	const TMap<FGuid, FIntPoint>& MeasuredNodeSizes,
	TMap<FGuid, FIntPoint>& OutPositions)
{
	OutPositions.Reset();
	if (!FlowAsset)
	{
		return;
	}

	TMap<FGuid, FLayoutNode> LayoutNodes;
	ResolveNodes(FlowAsset, TargetGuids, MeasuredNodeSizes, LayoutNodes);
	if (LayoutNodes.IsEmpty())
	{
		return;
	}

	TArray<FLayoutEdge> Edges;
	BuildEdges(FlowAsset, LayoutNodes, Edges);
	const TArray<FCommentSnapshot> Comments = GatherComments(FlowAsset, LayoutNodes);
	TArray<FLayoutPartition> Partitions;
	BuildPartitions(LayoutNodes, Edges, Comments, Partitions);

	for (const FLayoutPartition& Partition : Partitions)
	{
		PlacePartition(Partition, Edges, LayoutNodes, Partition.OriginalOrigin, OutPositions);
	}
	ResolvePartitionCollisions(Partitions, LayoutNodes, OutPositions);

	AvoidUnselectedNodes(FlowAsset, TargetGuids, LayoutNodes, MeasuredNodeSizes, OutPositions);
}

void FFlowGraphEditorLayoutAlgorithm::Apply(
	UFlowAsset* FlowAsset,
	const TMap<FGuid, FIntPoint>& Positions,
	const TMap<FGuid, FIntPoint>& MeasuredNodeSizes)
{
	if (!FlowAsset || Positions.IsEmpty())
	{
		return;
	}

	TMap<FGuid, FLayoutNode> AllLayoutNodes;
	ResolveNodes(FlowAsset, TSet<FGuid>(), MeasuredNodeSizes, AllLayoutNodes);
	const TArray<FCommentSnapshot> Comments = GatherComments(FlowAsset, AllLayoutNodes);

	bool bAnyChanged = false;
	for (const TPair<FGuid, FIntPoint>& Pair : Positions)
	{
		UFlowNode* Node = FlowAsset->GetNode(Pair.Key);
		UEdGraphNode* GraphNode = Node ? Node->GetGraphNode() : nullptr;
		if (!GraphNode || (GraphNode->NodePosX == Pair.Value.X && GraphNode->NodePosY == Pair.Value.Y))
		{
			continue;
		}

		GraphNode->Modify();
		GraphNode->NodePosX = Pair.Value.X;
		GraphNode->NodePosY = Pair.Value.Y;
		bAnyChanged = true;
	}

	bAnyChanged |= ResizeFullyTargetedComments(Comments, Positions, AllLayoutNodes);
	if (bAnyChanged)
	{
		FlowAsset->GetOutermost()->MarkPackageDirty();
	}
}
} // namespace UE::FlowGraph::Private
