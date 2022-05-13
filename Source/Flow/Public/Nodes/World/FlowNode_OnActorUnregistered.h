// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/World/FlowNode_ComponentObserver.h"
#include "FlowNode_OnActorUnregistered.generated.h"

/**
 * Triggers output when Flow Component with matching Identity Tag disappears from the world
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Actor Unregistered"))
class FLOW_API UFlowNode_OnActorUnregistered : public UFlowNode_ComponentObserver
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;
	virtual void ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component) override;
};
