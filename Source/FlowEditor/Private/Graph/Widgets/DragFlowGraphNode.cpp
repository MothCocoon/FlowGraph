// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "DragFlowGraphNode.h"

#include "Framework/Application/SlateApplication.h"
#include "Graph/Nodes/FlowGraphNode.h"

TSharedRef<FDragFlowGraphNode> FDragFlowGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphNode>& InDraggedNode)
{
	TSharedRef<FDragFlowGraphNode> Operation = MakeShareable(new FDragFlowGraphNode);

	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes.Add(InDraggedNode);
	// adjust the decorator away from the current mouse location a small amount based on cursor size
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();

	return Operation;
}

TSharedRef<FDragFlowGraphNode> FDragFlowGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphNode> >& InDraggedNodes)
{
	TSharedRef<FDragFlowGraphNode> Operation = MakeShareable(new FDragFlowGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes = InDraggedNodes;
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

UFlowGraphNode* FDragFlowGraphNode::GetDropTargetNode() const
{
	return Cast<UFlowGraphNode>(GetHoveredNode());
}
