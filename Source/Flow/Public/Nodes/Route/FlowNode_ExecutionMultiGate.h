// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_ExecutionMultiGate.generated.h"

/**
 * Fires one output pin per activation, cycling through all output pins in order or, if bRandom is set, in random
 * order; a Reset input pin restarts the cycle from the beginning. Optionally loops when all outputs have been fired.
 * Use bRandom for randomized ordering, bLoop to allow re-cycling, and StartIndex to begin from a specific pin.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Multi Gate", Keywords = "series loop random"))
class FLOW_API UFlowNode_ExecutionMultiGate final : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowNode_ExecutionMultiGate();

protected:
	/* When true, outputs are selected randomly each activation instead of in order. */
	UPROPERTY(EditAnywhere, Category = "MultiGate")
	bool bRandom = false;
	/* Allow executing output pins again, without triggering Reset pin.
	 * If set to False, every output pin can be triggered only once/ */
	UPROPERTY(EditAnywhere, Category = "MultiGate")
	bool bLoop = false;
	/* Index of the first output pin to fire on the first activation (0-based). */
	UPROPERTY(EditAnywhere, Category = "MultiGate")
	int32 StartIndex = INDEX_NONE;

private:
	UPROPERTY(SaveGame)
	int32 NextOutput;

	UPROPERTY(SaveGame)
	TArray<bool> Completed;

public:
#if WITH_EDITOR
	virtual bool CanUserAddOutput() const override { return true; }
#endif

	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
	virtual const FFlowAgentDoc& GetAgentDoc() const override;
#endif
};
