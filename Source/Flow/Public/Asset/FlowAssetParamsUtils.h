// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Misc/DateTime.h"
#include "Asset/FlowAssetParamsTypes.h"

#include "FlowAssetParamsUtils.generated.h"

class UObject;
struct FFlowNamedDataPinProperty;

/**
* Utility functions for Flow Asset Params reconciliation and validation.
*/
USTRUCT()
struct FLOW_API FFlowAssetParamsUtils
{
	GENERATED_BODY()

#if WITH_EDITOR
	static FDateTime GetLastSavedTimestampForObject(const UObject* Object);

	static EFlowReconcilePropertiesResult CheckPropertiesMatch(
		const TArray<FFlowNamedDataPinProperty>& PropertiesA,
		const TArray<FFlowNamedDataPinProperty>& PropertiesB);

	static const FFlowNamedDataPinProperty* FindPropertyByGuid(
		const TArray<FFlowNamedDataPinProperty>& Props,
		const FGuid& Guid);

	static FFlowNamedDataPinProperty* FindPropertyByGuid(
		TArray<FFlowNamedDataPinProperty>& Props,
		const FGuid& Guid);

	static bool ArePropertyArraysEqual(
		const TArray<FFlowNamedDataPinProperty>& A,
		const TArray<FFlowNamedDataPinProperty>& B);

	static bool ArePropertiesEqual(
		const FFlowNamedDataPinProperty& A,
		const FFlowNamedDataPinProperty& B);
#endif
};