// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "Types/FlowPinEnums.h"
#include "FlowDataPinResults.generated.h"

struct FInstancedStruct;
struct FFlowDataPinValue;


USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result")
struct FFlowDataPinResult
{
	GENERATED_BODY()

public:
	/* Result for the DataPin resolve attempt. */
	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	EFlowDataPinResolveResult Result = EFlowDataPinResolveResult::FailedUnimplemented;

public:
	FLOW_API explicit FFlowDataPinResult() = default;
	FLOW_API explicit FFlowDataPinResult(EFlowDataPinResolveResult InResult) : Result(InResult) { }

	template <typename TFlowDataPinValueSubclass>
	explicit FFlowDataPinResult(const TFlowDataPinValueSubclass& InValue) : Result(EFlowDataPinResolveResult::Success), ResultValue(TInstancedStruct<FFlowDataPinValue>::Make(InValue)) {}

public:
	UPROPERTY()
	TInstancedStruct<FFlowDataPinValue> ResultValue;
};
