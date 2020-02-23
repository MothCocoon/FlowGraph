#pragma once

#include "SFlowGraphNode.h"

class SFlowGraphNode_Out : public SFlowGraphNode
{
protected:
	// SGraphNode
	virtual TSharedRef<SWidget> CreateNodeContentArea() override;
	// --
};