// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Editor/GraphEditor/Private/DragNode.h"
#include "Templates/SharedPointer.h"

class SGraphPanel;
class UFlowGraphNode;

// Adapted from FDragAIGraphNode
class FDragFlowGraphNode : public FDragNode
{
public:
	DRAG_DROP_OPERATOR_TYPE(FDragFlowGraphNode, FDragNode)

	static TSharedRef<FDragFlowGraphNode> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphNode>& InDraggedNode);
	static TSharedRef<FDragFlowGraphNode> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray<TSharedRef<SGraphNode>>& InDraggedNodes);

	UFlowGraphNode* GetDropTargetNode() const;

	double StartTime;

protected:
	typedef FDragNode Super;
};