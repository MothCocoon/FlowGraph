#pragma once

#include "Nodes/World/FlowNode_ComponentObserver.h"
#include "FlowNode_OnActorRegistered.generated.h"

/**
 * Triggers output when Flow Component with matching Identity Tag appears in the world
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Actor Registered"))
class FLOW_API UFlowNode_OnActorRegistered : public UFlowNode_ComponentObserver
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;
};
