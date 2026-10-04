// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/NameTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Containers/Map.h"

#include "FlowOutputDataPinValues.generated.h"

struct FFlowDataPinValue;

/**
 * Container for output data pin values from a Flow Asset execution.
 * Holds a map of pin names to their resolved values.
 */
USTRUCT()
struct FFlowOutputDataPinValues
{
	GENERATED_BODY()

public:
	/* Map of output pin names to their resolved values. */
	UPROPERTY()
	TMap<FName, TInstancedStruct<FFlowDataPinValue>> Values;
};
