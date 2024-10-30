// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowNodeBase.h"
#include "Nodes/FlowPin.h"

#include "FlowNodeAddOn.generated.h"

class UFlowNode;

/**
 * A Flow Node AddOn allows user to extend given node instance in the graph with additional logic.
 */
UCLASS(Abstract, MinimalApi, EditInlineNew, Blueprintable)
class UFlowNodeAddOn : public UFlowNodeBase
{
	GENERATED_BODY()

protected:
	// The FlowNode that contains this AddOn
	// (accessible only when initialized, runtime only)
	UPROPERTY(Transient)
	TObjectPtr<UFlowNode> FlowNode;

	// Input pins to add to the owning flow node
	// If defined, ExecuteInput will only be executed for these inputs
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FlowNodeAddOn")
	TArray<FFlowPin> InputPins;

	// Output pins to add to the owning flow node
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FlowNodeAddOn")
	TArray<FFlowPin> OutputPins;
	
public:
	// UFlowNodeBase

	// AddOns may opt in to be eligible for a given parent
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "FlowNodeAddOn")
	FLOW_API EFlowAddOnAcceptResult AcceptFlowNodeAddOnParent(const UFlowNodeBase* ParentTemplate) const;

	FLOW_API virtual UFlowNode* GetFlowNodeSelfOrOwner() override { return FlowNode; }
	FLOW_API virtual bool IsSupportedInputPinName(const FName& PinName) const override;

	FLOW_API virtual void TriggerFirstOutput(const bool bFinish) override;
	FLOW_API virtual void TriggerOutput(const FName PinName, const bool bFinish = false, const EFlowPinActivationType ActivationType = EFlowPinActivationType::Default) override;
	FLOW_API virtual void Finish() override;
	// --

	// IFlowCoreExecutableInterface
	FLOW_API virtual void InitializeInstance() override;
	FLOW_API virtual void DeinitializeInstance() override;
	// --

	// UFlowNodeAddOn
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FlowNodeAddon", DisplayName = "Get Flow Node")
	FLOW_API UFlowNode* GetFlowNode() const;
	// --

#if WITH_EDITOR
	// IFlowContextPinSupplierInterface
	FLOW_API virtual bool SupportsContextPins() const override { return Super::SupportsContextPins() || (!InputPins.IsEmpty() || !OutputPins.IsEmpty()); }
	FLOW_API virtual TArray<FFlowPin> GetContextInputs() const override;
	FLOW_API virtual TArray<FFlowPin> GetContextOutputs() const override;
	// --
#endif // WITH_EDITOR

protected:
	void CacheFlowNode();
};
