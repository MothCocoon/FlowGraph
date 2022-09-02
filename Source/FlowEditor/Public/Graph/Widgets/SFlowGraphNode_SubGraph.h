// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Graph/Widgets/SFlowGraphNode.h"

class FLOWEDITOR_API SFlowGraphNode_SubGraph : public SFlowGraphNode
{
protected:
	// SGraphNode
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	// --
};
