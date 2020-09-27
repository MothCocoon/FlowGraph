#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_SubGraph.generated.h"

/**
 * Sub Graph
 */
UCLASS(meta = (DisplayName = "Sub Graph"))
class FLOW_API UFlowNode_SubGraph : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
	friend class UFlowSubsystem;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Graph")
	TSoftObjectPtr<UFlowAsset> Asset;

protected:
	virtual void PreloadContent() override;
	virtual void FlushContent() override;

	virtual void ExecuteInput(const FName& PinName) override;

public:
	virtual void ForceFinishNode() override;

#if WITH_EDITOR
	virtual FString GetNodeDescription() const override;
	virtual UObject* GetAssetToOpen() override;

	virtual bool SupportsContextPins() const override { return true; }

	virtual TArray<FName> GetContextInputs() override;
	virtual TArray<FName> GetContextOutputs() override;
#endif
};
