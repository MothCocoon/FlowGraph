// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"
#include "UObject/ScriptInterface.h"

#include "FlowExternalExecutableInterface.generated.h"

class UFlowNodeBase;

/**
 * Implemented by external objects that can execute within a Flow Graph  via a FlowNode or FlowNodeAddOn proxy.
 */
UINTERFACE(MinimalAPI, Blueprintable, DisplayName = "Flow External Executable Interface")
class UFlowExternalExecutableInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowExternalExecutableInterface
{
	GENERATED_BODY()

public:
	/* Called immediately prior to OnActivate() to set the native proxy that is executing the
	 * external executable object in the flow graph.  This is primarily done so that the external element has a
	 * handle to call TriggerOutput() and Finish() when it has completed its work. */
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", DisplayName = "PreActivateExternalFlowExecutable")
	void K2_PreActivateExternalFlowExecutable(const UFlowNodeBase* FlowNodeBase);
	virtual void PreActivateExternalFlowExecutable(UFlowNodeBase& FlowNodeBase);
};
