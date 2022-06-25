// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNode.h"
#include "FlowNode_Actor.generated.h"

class UFlowComponent;

/** */
UENUM()
enum class EFlowNodePropertyResolveMode : uint8
{
	/** No auto resolve. Do resolve manually */
	None,
	/** Resolves properties on node initialization */
	OnInitialize,
	/** Resolves properties when input triggered for the first time */
	OnInputTrigger
};


/**
 * Base class for nodes operating on actors using SoftActorPtr
 * Resolves actor references using owner of FlowAsset
 * Owner of FlowAsset must be Actor or ActorComponent
 */
UCLASS(Abstract, Blueprintable)
class FLOW_API UFlowNode_Actor : public UFlowNode
{
	GENERATED_UCLASS_BODY()	

protected:

	/** Determines when to perform property resolve */
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	EFlowNodePropertyResolveMode PropertyResolveMode;

	uint8 bHasResolvedSoftObjectPointers : 1;


protected:
	virtual void InitializeInstance() override;
	virtual void ExecuteInput(const FName& PinName) override;


	/**
	 * Find current level context and fix soft pointers to point at actual actors in level
	 */
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void ResolveSoftActorPtrs();
};
