// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "Nodes/Graph/FlowNode_BlueprintDataPinSupplierBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_BlueprintDataPinSupplierBase)

UFlowNode_BlueprintDataPinSupplierBase::UFlowNode_BlueprintDataPinSupplierBase()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::Default;
	Category = TEXT("Graph");
#endif

	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

FFlowDataPinResult UFlowNode_BlueprintDataPinSupplierBase::TrySupplyDataPin(const FName PinName) const
{
	return BP_TrySupplyDataPin(PinName);
}

FFlowDataPinResult UFlowNode_BlueprintDataPinSupplierBase::BP_TrySupplyDataPin_Implementation(const FName PinName) const
{
	return Super::TrySupplyDataPin(PinName);
}
