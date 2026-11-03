// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "GameplayTagContainer.h"
#include "Nodes/FlowNode.h"

#include "FlowDataPinValuesTestTypes.generated.h"

/** Native property sources for the data-pin resolution regression tests. */
UCLASS(Hidden, NotBlueprintable, NotPlaceable, meta = (ExcludeFromFlowCatalog))
class UFlowDataPinValuesTestNode : public UFlowNode
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (DefaultForInputFlowPin, FlowPinType = "Int"))
	int32 Integer = 42;

	UPROPERTY(meta = (DefaultForInputFlowPin, FlowPinType = "Int"))
	TArray<int32> Integers = { 3, 7, 11 };

	UPROPERTY(meta = (DefaultForInputFlowPin, FlowPinType = "Vector"))
	FVector Vector = FVector(1, 2, 3);

	UPROPERTY(meta = (DefaultForInputFlowPin, FlowPinType = "GameplayTagContainer"))
	FGameplayTagContainer Tags;
};
