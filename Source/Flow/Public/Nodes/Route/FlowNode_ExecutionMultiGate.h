// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_ExecutionMultiGate.generated.h"

/**
 * Executes a series of pins in order
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Multi Gate"))
class FLOW_API UFlowNode_ExecutionMultiGate final : public UFlowNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "MultiGate")
	bool bRandom;

	// Allow executing output pins again, without triggering Reset pin
	// If set to False, every output pin can be triggered only once
	UPROPERTY(EditAnywhere, Category = "MultiGate")
	bool bLoop;

	UPROPERTY(EditAnywhere, Category = "MultiGate")
	int32 StartIndex;

private:
	UPROPERTY(SaveGame)
	int32 NextOutput;

	UPROPERTY(SaveGame)
	TArray<bool> Completed;

public:
#if WITH_EDITOR
	virtual bool CanUserAddOutput() const override { return true; }
#endif

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
#endif
};
