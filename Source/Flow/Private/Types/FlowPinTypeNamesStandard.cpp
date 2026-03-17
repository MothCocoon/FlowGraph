// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowPinTypeNamesStandard.h"

const TSet<FName> FFlowPinTypeNamesStandard::AllStandardTypeNames =
	{
		PinTypeNameBool,
		PinTypeNameInt,
		PinTypeNameInt64,
		PinTypeNameFloat,
		PinTypeNameDouble,
		PinTypeNameEnum,
		PinTypeNameName,
		PinTypeNameString,
		PinTypeNameText,
		PinTypeNameVector,
		PinTypeNameRotator,
		PinTypeNameTransform,
		PinTypeNameGameplayTag,
		PinTypeNameGameplayTagContainer,
		PinTypeNameInstancedStruct,
		PinTypeNameObject,
		PinTypeNameClass,
	};
const TSet<FName> FFlowPinTypeNamesStandard::AllStandardIntegerTypeNames =
	{
		PinTypeNameInt,
		PinTypeNameInt64,
	};
const TSet<FName> FFlowPinTypeNamesStandard::AllStandardFloatTypeNames =
	{
		PinTypeNameFloat,
		PinTypeNameDouble,
	};
const TSet<FName> FFlowPinTypeNamesStandard::AllStandardStringLikeTypeNames =
	{
		PinTypeNameName,
		PinTypeNameString,
		PinTypeNameText,
	};
const TSet<FName> FFlowPinTypeNamesStandard::AllStandardGameplayTagTypeNames =
	{
		PinTypeNameGameplayTag,
		PinTypeNameGameplayTagContainer,
	};
const TSet<FName> FFlowPinTypeNamesStandard::AllStandardSubCategoryObjectTypeNames =
	{
		PinTypeNameInstancedStruct,
		PinTypeNameObject,
		PinTypeNameClass,
	};