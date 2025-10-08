// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/NameTypes.h"
#include "FlowDataPinTypeName.h"

struct FFlowDataPinTypeNamesStandard
{
	// Other Standard Pin Types
	static const FFlowPinTypeName UnknownPinTypeName;
	static const FFlowPinTypeName ValueTypeNameExec;

	// "Standard" Data Pin Types
	static const FFlowPinTypeName ValueTypeNameBool;
	static const FFlowPinTypeName ValueTypeNameInt;
	static const FFlowPinTypeName ValueTypeNameInt64;
	static const FFlowPinTypeName ValueTypeNameFloat;
	static const FFlowPinTypeName ValueTypeNameDouble;
	static const FFlowPinTypeName ValueTypeNameEnum;
	static const FFlowPinTypeName ValueTypeNameName;
	static const FFlowPinTypeName ValueTypeNameString;
	static const FFlowPinTypeName ValueTypeNameText;
	static const FFlowPinTypeName ValueTypeNameVector;
	static const FFlowPinTypeName ValueTypeNameRotator;
	static const FFlowPinTypeName ValueTypeNameTransform;
	static const FFlowPinTypeName ValueTypeNameGameplayTag;
	static const FFlowPinTypeName ValueTypeNameGameplayTagContainer;
	static const FFlowPinTypeName ValueTypeNameInstancedStruct;
	static const FFlowPinTypeName ValueTypeNameObject;
	static const FFlowPinTypeName ValueTypeNameInstancedObject;
	static const FFlowPinTypeName ValueTypeNameClass;
};