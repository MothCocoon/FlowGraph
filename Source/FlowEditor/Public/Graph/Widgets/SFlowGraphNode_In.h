#pragma once

#include "SFlowGraphNode.h"

class SFlowGraphNode_In : public SFlowGraphNode
{
protected:
	// SGraphNode
	virtual TSharedRef<SWidget> CreateNodeContentArea() override;
	// --
};