// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Graph/Widgets/SFlowGraphNode.h"

class SFlowGraphNode_SubGraph : public SFlowGraphNode
{
protected:
	// SGraphNode
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	// --
};
