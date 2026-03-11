// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Misc/DateTime.h"
#include "Asset/FlowAssetParamsTypes.h"

#include "FlowAssetParamsUtils.generated.h"

class UObject;
class UFlowAssetParams;
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

	/**
	* Create Flow Asset Params asset from a parent params asset.
	* - Creates the new asset in the same folder as the parent
	* - Uses parent's asset name as the base for unique name generation (ParentName, ParentName_1, ...)
	* - Copies OwnerFlowAsset + Properties and sets ParentParams to the provided parent
	* - Runs ReconcilePropertiesWithParentParams (cycle detection, flattened inheritance, etc.)
	* - Attempts source control checkout/add
	* - Saves the new package
	* - Registers and syncs to Content Browser
	*
	* @param ParentParams The parent params asset to inherit from. Must be valid.
	* @param bShowDialogs If true, errors are surfaced via modal dialogs as well as logs.
	* @param OutOptionalFailureReason If provided, filled with a human-readable error message on failure.
	* @return The created child params asset or nullptr on failure.
	*/
	static UFlowAssetParams* CreateChildParamsAsset(UFlowAssetParams& ParentParams, const bool bShowDialogs = true, FText* OutOptionalFailureReason = nullptr);

protected:
	static void FailCreateChild(const FText& Reason, const bool bShowDialogs, FText* OutOptionalFailureReason);

#endif
};
