// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Interface.h"

#include "FlowNamedPropertiesSupplierInterface.generated.h"

struct FFlowNamedDataPinProperty;

UINTERFACE(Blueprintable)
class FLOW_API UFlowNamedPropertiesSupplierInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for Flow nodes that supply named properties, such as Start or DefineProperties nodes.
 */
class FLOW_API IFlowNamedPropertiesSupplierInterface
{
	GENERATED_BODY()

public:

#if WITH_EDITOR
	// Returns the array of named properties defined by this node.
	virtual TArray<FFlowNamedDataPinProperty>& GetMutableNamedProperties() = 0;
#endif
};
