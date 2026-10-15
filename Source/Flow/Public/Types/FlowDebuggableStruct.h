#pragma once

#include "Containers/UnrealString.h"
#include "UObject/ObjectMacros.h"

#include "FlowDebuggableStruct.generated.h"

// Base struct you can inherit from to give structs wrapped
// by FFlowDataPinValue_InstancedStruct a custom debug string
// for Flow Editor data pin debugging.
USTRUCT()
struct FLOW_API FFlowDebuggableStruct
{
	GENERATED_BODY()

	virtual ~FFlowDebuggableStruct() = default;

	virtual bool GetDebugString(FString& OutString) const { return false; }
};
