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

	// all instanced assets
	TSet<TWeakObjectPtr<UFlowAsset>> InstancedAssets;

	// instanced assets "owned" by Sub Flow nodes
	TMap<UFlowNodeSubFlow*, UFlowAsset*> InstancedSubFlows;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FLOW_API void StartFlow(UFlowAsset* FlowAsset);
	FLOW_API void EndFlow(UFlowAsset* FlowAsset);

	void PreloadSubFlow(UFlowNodeSubFlow* SubFlow);
	void FlushPreload(UFlowNodeSubFlow* SubFlow);

	void StartSubFlow(UFlowNodeSubFlow* SubFlow);

private:
	UFlowAsset* CreateFlowInstance(TSoftObjectPtr<UFlowAsset> FlowAsset);

public:
	FORCEINLINE UWorld* GetWorld() const;
};