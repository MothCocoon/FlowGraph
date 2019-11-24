#pragma once

#include "FlowNode.h"
#include "FlowNodeSubFlow.generated.h"

class UFlowAsset;

/**
 * Sub Flow
 */
UCLASS(meta = (DisplayName = "Sub Flow"))
class FLOW_API UFlowNodeSubFlow : public UFlowNode
{
	GENERATED_UCLASS_BODY()
	
private:
	UPROPERTY(EditAnywhere, Category = "Flow")
	TSoftObjectPtr<UFlowAsset> FlowAsset;

protected:
	virtual void ExecuteInput(const uint8 Pin) override;

public:
	virtual FString GetDesc() override;

#if WITH_EDITOR
	virtual UObject* GetAssetToOpen();
#endif
};
