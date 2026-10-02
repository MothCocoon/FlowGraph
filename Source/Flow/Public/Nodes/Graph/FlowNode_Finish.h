// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/Graph/FlowNode_SetGraphOutput.h"
#include "FlowNode_Finish.generated.h"

/**
 * Terminates execution of this Flow Asset and deactivates all currently active nodes and
 * sub-graphs. Place at every logical end point of the graph (success, failure, timeout, etc.).
 * Multiple Finish nodes are allowed; the first to be triggered wins.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Finish", Keywords = "output datapin"))
class FLOW_API UFlowNode_Finish : public UFlowNode_SetGraphOutput
{
	GENERATED_BODY()

public:
	UFlowNode_Finish();

	virtual bool CanFinishGraph() const override { return true; }
	virtual void ExecuteInput(const FName& PinName) override;

#if WITH_EDITOR
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
