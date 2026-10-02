// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_ExecutionSequence.generated.h"

/**
 * Fires all output pins sequentially in order when its input is triggered.
 * All connected outputs execute in the same frame unless downstream nodes suspend (async).
 * Use bSavePinExecutionState to persist which outputs have already fired when save/load is used during gameplay.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Sequence"))
class FLOW_API UFlowNode_ExecutionSequence final : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_ExecutionSequence();

protected:
	/**
	 * If enabled and the graph is saved during gameplay, this node
	 * tracks and saves which pins it has executed.
	 *
	 * If you add new connections or replace old connections with
	 * different nodes, this node will detect the changes. If during gameplay
	 * you load an old save game which had different connections, this node
	 * will automatically execute the updated connections you created.
	 */
	UPROPERTY(EditAnywhere, Category = "Sequence")
	bool bSavePinExecutionState = true;

	UPROPERTY(SaveGame)
	TSet<FGuid> ExecutedConnections;

public:
#if WITH_EDITOR
	virtual bool CanUserAddOutput() const override { return true; }
#endif

	virtual void ExecuteInput(const FName& PinName) override;
	virtual void OnLoad_Implementation() override;
	virtual void Cleanup() override;

protected:
	void ExecuteNewConnections();

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
