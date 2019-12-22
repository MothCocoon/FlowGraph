#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"
#include "SGraphNode.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

#include "../Nodes/FlowGraphNode.h"
#include "../Nodes/FlowGraphNode_In.h"
#include "../Nodes/FlowGraphNode_Out.h"

#include "SFlowGraphNode.h"
#include "SFlowGraphNode_In.h"
#include "SFlowGraphNode_Out.h"

class FFlowGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* InNode) const override
	{
		if (UFlowGraphNode_In* NodeIn = Cast<UFlowGraphNode_In>(InNode))
		{
			return SNew(SFlowGraphNode_In, NodeIn);
		}

		if (UFlowGraphNode_Out* NodeOut = Cast<UFlowGraphNode_Out>(InNode))
		{
			return SNew(SFlowGraphNode_Out, NodeOut);
		}

		if (UFlowGraphNode* FlowNode = Cast<UFlowGraphNode>(InNode))
		{
			return SNew(SFlowGraphNode, FlowNode);
		}

		return nullptr;
	}
};