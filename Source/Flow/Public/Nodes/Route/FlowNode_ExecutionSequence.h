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

protected:
	/**
	 * If enabled and the graph is saved during gameplay, this node
	 * tracks and saves which pins it has executed.
	 *
	 * If you add new connections or replace old connections with with
	 * different nodes, this node will detect the changes. If during gameplay
	 * you load an old save game which had different connections, this node
	 * will automatically execute the updated connections you created.
	 */
	UPROPERTY(EditAnywhere, Category = "Sequence")
	bool bSavePinExecutionState;

	UPROPERTY(SaveGame)
	TSet<FGuid> ExecutedConnections;

public:
#if WITH_EDITOR
	virtual bool CanUserAddOutput() const override { return true; }
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void OnLoad_Implementation() override;
	virtual void Cleanup() override;

	void ExecuteNewConnections();

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
#endif
};
