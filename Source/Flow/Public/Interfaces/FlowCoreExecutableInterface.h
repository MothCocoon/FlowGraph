// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Interface.h"

#include "FlowCoreExecutableInterface.generated.h"

/**
 * Implemented by objects that can execute within a Flow Graph.
 * Example: UFlowNode and UFlowNodeAddOn subclasses implement this.
 */
UINTERFACE(MinimalAPI, Blueprintable, DisplayName = "Flow Core Executable Interface")
class UFlowCoreExecutableInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowCoreExecutableInterface
{
	GENERATED_BODY()

public:
	/* Method called just after creating the node instance, while initializing the Flow Asset instance.
	 * This happens before executing graph, only called during gameplay. */
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", DisplayName = "Initialize Instance")
	void K2_InitializeInstance();
	virtual void InitializeInstance() { Execute_K2_InitializeInstance(Cast<UObject>(this));  }

	/* Event called from UMKTFlowNode::DeinitializeInstance(). */
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", DisplayName = "Deinitialize Instance")
	void K2_DeinitializeInstance();
	virtual void DeinitializeInstance() { Execute_K2_DeinitializeInstance(Cast<UObject>(this)); }

	/* If preloading is enabled, will be called to preload content. */
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", DisplayName = "Preload Content")
	void K2_PreloadContent();
	virtual void PreloadContent() { Execute_K2_PreloadContent(Cast<UObject>(this)); }

	/* If preloading is enabled, will be called to flush content. */
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", DisplayName = "Flush Content")
	void K2_FlushContent();
	virtual void FlushContent() { Execute_K2_FlushContent(Cast<UObject>(this)); }

	/* Called immediately before the first input is triggered. */
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", DisplayName = "OnActivate")
	void K2_OnActivate();
	virtual void OnActivate() { Execute_K2_OnActivate(Cast<UObject>(this)); }

	/* Event called after node finished the work. */
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", DisplayName = "Cleanup")
	void K2_Cleanup();
	virtual void Cleanup() { Execute_K2_Cleanup(Cast<UObject>(this)); }

	/* Define what happens when node is terminated from the outside. */
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", DisplayName = "Force Finish Node")
	void K2_ForceFinishNode();
	virtual void ForceFinishNode() { Execute_K2_ForceFinishNode(Cast<UObject>(this)); }

	/* Event reacting on triggering Input pin. */
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", DisplayName = "Execute Input")
	void K2_ExecuteInput(const FName& PinName);
	virtual void ExecuteInput(const FName& PinName) { Execute_K2_ExecuteInput(Cast<UObject>(this), PinName); }
};
