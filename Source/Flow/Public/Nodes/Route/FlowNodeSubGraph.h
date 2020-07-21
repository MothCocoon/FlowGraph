#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNodeSubGraph.generated.h"

class UFlowAsset;

/**
 * Sub Graph
 */
UCLASS(meta = (DisplayName = "Sub Graph"))
class FLOW_API UFlowNodeSubGraph : public UFlowNode
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
	virtual void OnForceFinished() override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual UObject* GetAssetToOpen() override;
#endif
};
