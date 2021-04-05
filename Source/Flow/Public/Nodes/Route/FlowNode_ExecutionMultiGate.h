#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_ExecutionMultiGate.generated.h"

/**
 * Executes a series of pins in order
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "MultiGate"))
class FLOW_API UFlowNode_ExecutionMultiGate final : public UFlowNode
{
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "MultiGate")
	bool bRandom;

	UPROPERTY(EditDefaultsOnly, Category = "MultiGate")
	bool bLoop;

	UPROPERTY(EditDefaultsOnly, Category = "MultiGate")
	int32 StartIndex = INDEX_NONE;
	
#if WITH_EDITOR
	virtual bool CanUserAddOutput() const override { return true; }
#endif
	
protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
#endif

	int32 NextOutput;
	TArray<bool> Completed;

	static const FName ResetInputName;
};
