// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowEnumUtils.h"

#include "FlowPinEnums.generated.h"

UENUM(BlueprintType)
enum class EFlowPinType : uint8
{
	// Execution pin
	Exec,

	// FBoolProperty
	Bool,

	// FByteProperty FInt16Property FIntProperty FInt64Property FUInt16Property FUInt32Property FUInt64Property
	Int,

	// FFloatProperty, FDoubleProperty
	Float,

	// FNameProperty
	Name,

	// FStringProperty
	String,

	// FTextProperty
	Text,

	// FEnumProperty, FByteProperty
	Enum,

	// FVector (FStructProperty)
	Vector,

	// FRotator (FStructProperty)
	Rotator,

	// FTransform (FStructProperty)
	Transform,

	// FGameplayTag (FStructProperty)
	GameplayTag,

	// FGameplayTagContainer (FStructProperty)
	GameplayTagContainer,

	// FInstancedStruct (FStructProperty)
	InstancedStruct,

	// FObjectProperty, FObjectPtrProperty, FWeakObjectProperty, FLazyObjectProperty, FSoftObjectProperty
	Object,

	// FClassProperty, FClassPtrProperty, FSoftClassProperty
	Class,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowPinType)

// Result enum for TryResolveDataPinAs...() functions
UENUM(BlueprintType)
enum class EFlowDataPinResolveResult : uint8
{
	// Pin resolved successfully
	Success,

	// The pin is not connected to another pin
	FailedUnconnected,

	// The pin name is unknown
	FailedUnknownPin,

	// The pin was requested as an unsupported type
	FailedMismatchedType,

	// The Flow Node or AddOn did not implement the necessary function to provide this value
	FailedUnimplemented,

	// Failed due to missing pin (may just need re-save for the asset)
	FailedMissingPin,

	// Failed with an error message (see the error log)
	FailedWithError,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowDataPinResolveResult)
