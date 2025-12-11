// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowEnumUtils.h"

#include "FlowPinEnums.generated.h"

UENUM(BlueprintType, meta = (ScriptName = "LegacyFlowPinNameEnum"))
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

// Result enum for TryResolveDataPin()
UENUM(BlueprintType)
enum class EFlowDataPinResolveResult : uint8
{
	// Pin resolved successfully
	Success,

	// The pin name is unknown
	FailedUnknownPin,

	// The pin was requested as an unsupported type
	FailedMismatchedType,

	// The Flow Node or AddOn did not implement the necessary function to provide this value
	FailedUnimplemented,

	// Failed due to insufficient values (eg. resolving a single value with an empty array)
	FailedInsufficientValues,

	// Could not resolve an enum value
	FailedUnknownEnumValue,

	// Tried to extract with a null FlowNodeBase
	FailedNullFlowNodeBase,

	// Failed with an error message (see the error log)
	FailedWithError,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowDataPinResolveResult)

UENUM(BlueprintType)
enum class EFlowDataPinResolveSimpleResult : uint8
{
	Succeeded = 1,
	Failed = 0,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowDataPinResolveSimpleResult)

namespace EFlowDataPinResolveResult_Classifiers
{
	FORCEINLINE bool IsSuccess(EFlowDataPinResolveResult Result) { return Result == EFlowDataPinResolveResult::Success; }
	FORCEINLINE EFlowDataPinResolveSimpleResult ConvertToSimpleResult(EFlowDataPinResolveResult ResultEnum)
		{ return IsSuccess(ResultEnum) ? EFlowDataPinResolveSimpleResult::Succeeded : EFlowDataPinResolveSimpleResult::Failed; }
};

UENUM(BlueprintType)
enum class EFlowDataMultiType : uint8
{
	Single,
	Array,

	// TODO (gtaylor) Consider future types like Set, Map

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowDataMultiType)

UENUM(BlueprintType)
enum class EFlowSingleFromArray : uint8
{
	// For the Single value, use the [0]th value (First)
	FirstValue,

	// For the Single value, use the [N-1]th value (Last)
	LastValue,

	// Expect a single value only, log an error if not (and return [0]th)
	ExpectSingleValueOnly,

	// Used in the FlowPinType templates for entire array extraction
	EntireArray UMETA(Hidden),

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowSingleFromArray)

namespace EFlowSingleFromArray_Classifiers
{
	FORCEINLINE int32 ConvertToIndex(EFlowSingleFromArray SingleFromArray, int32 ArrayMax)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowSingleFromArray, 4);
		switch (SingleFromArray)
		{
		case EFlowSingleFromArray::FirstValue:
			{
				if (ArrayMax > 0)
				{
					return 0;
				}
				else
				{
					return INDEX_NONE;
				}
			}

		case EFlowSingleFromArray::LastValue:
			{
				if (ArrayMax > 0)
				{
					return ArrayMax - 1;
				}
				else
				{
					return INDEX_NONE;
				}
			}

		case EFlowSingleFromArray::EntireArray:
			check(SingleFromArray != EFlowSingleFromArray::EntireArray);
			return INDEX_NONE;

		default:
		case EFlowSingleFromArray::ExpectSingleValueOnly:
			{
				if (ArrayMax == 1)
				{
					return 0;
				}
				else
				{
					return INDEX_NONE;
				}
			}
		}
	}
};