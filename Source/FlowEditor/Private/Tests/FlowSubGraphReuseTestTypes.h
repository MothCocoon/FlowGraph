// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Components/ActorComponent.h"
#include "FlowAsset.h"
#include "Nodes/Graph/FlowNode_Finish.h"
#include "Nodes/Graph/FlowNode_Start.h"

#include "FlowSubGraphReuseTestTypes.generated.h"

class UFlowInjectComponentsManager;

UCLASS(Hidden, NotBlueprintable)
class UFlowSubGraphReuseTestComponent : public UActorComponent
{
	GENERATED_BODY()
};

UCLASS(Hidden, NotBlueprintable, meta = (ExcludeFromFlowCatalog))
class UFlowSubGraphReusePolicyOverrideAsset : public UFlowAsset
{
	GENERATED_BODY()

public:
	virtual ESubGraphFinishPolicy GetSubGraphFinishPolicy() const override
	{
		return ESubGraphFinishPolicy::Deinitialize;
	}
};

UCLASS(Hidden, NotBlueprintable, NotPlaceable, meta = (ExcludeFromFlowCatalog))
class UFlowSubGraphReuseTestStartNode : public UFlowNode_Start
{
	GENERATED_BODY()

public:
	virtual void TriggerFirstOutput(const bool bFinish) override;

	bool bAllNodesNeverActivatedBeforeStart = false;
	bool bNoPreviousRecordedNodesBeforeStart = false;
	FString OtherNodesWithPreviousActivation;
};

UCLASS(Hidden, NotBlueprintable, NotPlaceable, meta = (ExcludeFromFlowCatalog))
class UFlowSubGraphReuseTestFinishNode : public UFlowNode_Finish
{
	GENERATED_BODY()

public:
	virtual void ExecuteInput(const FName& PinName) override;
	void FinishGraph();
	virtual void DeinitializeInstance() override;

	UPROPERTY(Transient)
	int32 ActivationCount = 0;

	UPROPERTY()
	bool bInjectComponentOnActivation = false;

	UPROPERTY(Transient)
	TObjectPtr<UFlowSubGraphReuseTestComponent> InjectedComponent;

	UPROPERTY(Transient)
	TObjectPtr<UFlowInjectComponentsManager> InjectedComponentsManager;
};
