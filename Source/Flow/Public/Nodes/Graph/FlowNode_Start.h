// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/Graph/FlowNode_DefineProperties.h"
#include "Interfaces/FlowNodeWithExternalDataPinSupplierInterface.h"
#include "FlowNode_Start.generated.h"

/**
 * Execution of the graph always starts from this node
 */
UCLASS(NotBlueprintable, NotPlaceable, meta = (DisplayName = "Start"))
class FLOW_API UFlowNode_Start
	: public UFlowNode_DefineProperties
	, public IFlowNodeWithExternalDataPinSupplierInterface
{
	GENERATED_UCLASS_BODY()

	friend class UFlowAsset;

protected:

	// External DataPin Value Supplier
	// (eg, the UFlowNode_SubGraph that instanced this Start node's flow asset)
	UPROPERTY(Transient)
	TScriptInterface<IFlowDataPinValueSupplierInterface> FlowDataPinValueSupplierInterface;

public:

	// IFlowCoreExecutableInterface
	virtual void ExecuteInput(const FName& PinName) override;
	// --

	// IFlowNodeWithExternalDataPinSupplierInterface
	virtual void SetDataPinValueSupplier(IFlowDataPinValueSupplierInterface* DataPinValueSupplier) override;
	virtual IFlowDataPinValueSupplierInterface* GetExternalDataPinSupplier() const override { return FlowDataPinValueSupplierInterface.GetInterface(); }
#if WITH_EDITOR
	virtual bool TryAppendExternalInputPins(TArray<FFlowPin>& InOutPins) const override;
#endif
	// --

	// Must implement TrySupplyDataAs... for every EFlowPinType
	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	// IFlowDataPinValueSupplierInterface
	virtual FFlowDataPinResult_Bool TrySupplyDataPinAsBool_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Int TrySupplyDataPinAsInt_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Float TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Name TrySupplyDataPinAsName_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_String TrySupplyDataPinAsString_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Text TrySupplyDataPinAsText_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Enum TrySupplyDataPinAsEnum_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Vector TrySupplyDataPinAsVector_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Rotator TrySupplyDataPinAsRotator_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Transform TrySupplyDataPinAsTransform_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_GameplayTag TrySupplyDataPinAsGameplayTag_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_GameplayTagContainer TrySupplyDataPinAsGameplayTagContainer_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_InstancedStruct TrySupplyDataPinAsInstancedStruct_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Object TrySupplyDataPinAsObject_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Class TrySupplyDataPinAsClass_Implementation(const FName& PinName) const override;
	// --
};
