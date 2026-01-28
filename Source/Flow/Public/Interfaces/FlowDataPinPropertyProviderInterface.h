// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "StructUtils/InstancedStruct.h"
#include "UObject/Interface.h"
#include "FlowDataPinPropertyProviderInterface.generated.h"

struct FFlowDataPinValue;

// Interface to define a FFlowDataPinValue provider.
// This is used in plumbing data in the AI Flow extension plugin into the Flow Data Pins framework.
UINTERFACE(MinimalAPI, NotBlueprintable)
class UFlowDataPinPropertyProviderInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowDataPinPropertyProviderInterface
{
	GENERATED_BODY()

public:

	// Provide a FFlowDataPinValue (instancedStruct) for the creation of data pins and supplying their values.
	virtual bool TryProvideFlowDataPinProperty(TInstancedStruct<FFlowDataPinValue>& OutFlowDataPinProperty) const = 0;
};
