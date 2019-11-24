#pragma once

#include "Engine/StreamableManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlowSubsystem.generated.h"

class UFlowAsset;
class UFlowNodeSubFlow;

/**
 * Flow Control System
 */
UCLASS(MinimalAPI)
class UFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UFlowSubsystem();

private:
	FStreamableManager Streamable;
	TArray<UFlowAsset*> ActiveFlows;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FLOW_API UFlowAsset* StartFlow(TSoftObjectPtr<UFlowAsset> FlowAsset);
	void StartSubFlow(UFlowNodeSubFlow* SubFlowNode, TSoftObjectPtr<UFlowAsset> ChildAsset, UFlowAsset* ParentFlow = nullptr);

private:
	UFlowAsset* CreateFlowInstance(TSoftObjectPtr<UFlowAsset> FlowAsset);
};
