// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_ExecutionSequence.generated.h"

/**
 * Executes all outputs sequentially
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Sequence"))
class FLOW_API UFlowNode_ExecutionSequence final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	virtual bool CanUserAddOutput() const override { return true; }

	virtual FString GetNodeDescription() const override;
#endif

	virtual void OnLoad_Implementation() override;
	
protected:
	virtual void ExecuteInput(const FName& PinName) override;

	/**
	 * If enabled and the flowgraph is saved during gameplay, this node
	 * tracks and saves which pins it has executed.
	 *
	 * If you add new connections or replace old connections with with
	 * different nodes, this node will detect the changes. If during gameplay
	 * you load an old save game which had different connections, this node
	 * will automatically execute the updated connections you created.
	 *
	 * This is useful if you want the ability to add new parts to your
	 * graph after release.
	 */
	UPROPERTY(EditAnywhere)
	bool bSavePinExecutionState = false;
	
	UPROPERTY(SaveGame)
	TSet<FGuid> ExecutedConnections;
	
	void ExecuteNewConnections();
};
