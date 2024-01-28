// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Interface.h"

#include "FlowOwnerInterface.generated.h"

// (optional) interface to enable a Flow owner object to execute CallOwnerFunction nodes
UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class UFlowOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowOwnerInterface
{
	GENERATED_BODY()
};
