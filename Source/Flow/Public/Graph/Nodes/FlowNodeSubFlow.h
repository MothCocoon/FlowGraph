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
	
	friend class UFlowSubsystem;
	
private:
	UPROPERTY(EditAnywhere, Category = "Flow")
	TSoftObjectPtr<UFlowAsset> FlowAsset;

protected:
	virtual void PreloadContent() override;
	virtual void FlushContent() override;

	virtual void ExecuteInput(const FName& PinName) override;
	virtual void OnForceFinished() override;

#if WITH_EDITOR
public:
	virtual FString GetNodeDescription() const override;
	virtual UObject* GetAssetToOpen();
#endif
};