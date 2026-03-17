// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Containers/Set.h"
#include "UObject/NameTypes.h"

struct FFlowPinTypeNamesStandard
{
	/* Other Standard Pin Types. */
	FLOW_API static constexpr const TCHAR* PinTypeNameUnknown = TEXT("Unknown");
	FLOW_API static constexpr const TCHAR* PinTypeNameExec = TEXT("Exec");

	/* "Standard" Data Pin Types. */
	FLOW_API static constexpr const TCHAR* PinTypeNameBool = TEXT("Bool");
	FLOW_API static constexpr const TCHAR* PinTypeNameInt = TEXT("Int");
	FLOW_API static constexpr const TCHAR* PinTypeNameInt64 = TEXT("Int64");
	FLOW_API static constexpr const TCHAR* PinTypeNameFloat = TEXT("Float");
	FLOW_API static constexpr const TCHAR* PinTypeNameDouble = TEXT("Double");
	FLOW_API static constexpr const TCHAR* PinTypeNameEnum = TEXT("Enum");
	FLOW_API static constexpr const TCHAR* PinTypeNameName = TEXT("Name");
	FLOW_API static constexpr const TCHAR* PinTypeNameString = TEXT("String");
	FLOW_API static constexpr const TCHAR* PinTypeNameText = TEXT("Text");
	FLOW_API static constexpr const TCHAR* PinTypeNameVector = TEXT("Vector");
	FLOW_API static constexpr const TCHAR* PinTypeNameRotator = TEXT("Rotator");
	FLOW_API static constexpr const TCHAR* PinTypeNameTransform = TEXT("Transform");
	FLOW_API static constexpr const TCHAR* PinTypeNameGameplayTag = TEXT("GameplayTag");
	FLOW_API static constexpr const TCHAR* PinTypeNameGameplayTagContainer = TEXT("GameplayTagContainer");
	FLOW_API static constexpr const TCHAR* PinTypeNameInstancedStruct = TEXT("InstancedStruct");
	FLOW_API static constexpr const TCHAR* PinTypeNameObject = TEXT("Object");
	FLOW_API static constexpr const TCHAR* PinTypeNameClass = TEXT("Class");

	// Sets of PinTypeNames that will be used in the FFlowPinConnectionPolicy functions
	FLOW_API static const TSet<FName> AllStandardTypeNames;
	FLOW_API static const TSet<FName> AllStandardIntegerTypeNames;
	FLOW_API static const TSet<FName> AllStandardFloatTypeNames;
	FLOW_API static const TSet<FName> AllStandardStringLikeTypeNames;
	FLOW_API static const TSet<FName> AllStandardGameplayTagTypeNames;
	FLOW_API static const TSet<FName> AllStandardSubCategoryObjectTypeNames;
};