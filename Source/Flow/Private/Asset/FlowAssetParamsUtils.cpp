// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowAssetParamsUtils.h"
#include "Asset/FlowAssetParamsTypes.h"
#include "Types/FlowDataPinProperties.h"
#include "FlowLogChannels.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowAssetParamsUtils)

#if WITH_EDITOR
FDateTime FFlowAssetParamsUtils::GetLastSavedTimestampForObject(const UObject* Object)
{
	if (!Object)
	{
		return FDateTime::MinValue();
	}

	const FString PackagePath = Object->GetPathName();
	return IFileManager::Get().GetTimeStamp(*PackagePath);
}

EFlowReconcilePropertiesResult FFlowAssetParamsUtils::CheckPropertiesMatch(
	const TArray<FFlowNamedDataPinProperty>& PropertiesA,
	const TArray<FFlowNamedDataPinProperty>& PropertiesB)
{
	if (PropertiesA.Num() != PropertiesB.Num())
	{
		return EFlowReconcilePropertiesResult::Error_PropertyCountMismatch;
	}

	for (int32 Index = 0; Index < PropertiesA.Num(); ++Index)
	{
		const FFlowNamedDataPinProperty& PropA = PropertiesA[Index];
		const FFlowNamedDataPinProperty& PropB = PropertiesB[Index];
		const UScriptStruct* ScriptStructA = PropA.DataPinProperty.GetScriptStruct();
		const UScriptStruct* ScriptStructB = PropB.DataPinProperty.GetScriptStruct();

		if (PropA.Name != PropB.Name ||
			ScriptStructA != ScriptStructB ||
			!IsValid(ScriptStructA))
		{
			return EFlowReconcilePropertiesResult::Error_PropertyTypeMismatch;
		}
	}

	return EFlowReconcilePropertiesResult::NoChanges;
}

const FFlowNamedDataPinProperty* FFlowAssetParamsUtils::FindPropertyByGuid(
	const TArray<FFlowNamedDataPinProperty>& Props,
	const FGuid& Guid)
{
	for (const FFlowNamedDataPinProperty& Prop : Props)
	{
		if (Prop.Guid == Guid)
		{
			return &Prop;
		}
	}

	return nullptr;
}

FFlowNamedDataPinProperty* FFlowAssetParamsUtils::FindPropertyByGuid(
	TArray<FFlowNamedDataPinProperty>& Props,
	const FGuid& Guid)
{
	for (FFlowNamedDataPinProperty& Prop : Props)
	{
		if (Prop.Guid == Guid)
		{
			return &Prop;
		}
	}

	return nullptr;
}

bool FFlowAssetParamsUtils::ArePropertyArraysEqual(
	const TArray<FFlowNamedDataPinProperty>& A,
	const TArray<FFlowNamedDataPinProperty>& B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < A.Num(); ++Index)
	{
		if (!ArePropertiesEqual(A[Index], B[Index]))
		{
			return false;
		}
	}

	return true;
}

bool FFlowAssetParamsUtils::ArePropertiesEqual(
	const FFlowNamedDataPinProperty& A,
	const FFlowNamedDataPinProperty& B)
{
	if (A.Name != B.Name || A.Guid != B.Guid)
	{
		return false;
	}

	const UScriptStruct* ScriptStructA = A.DataPinProperty.GetScriptStruct();
	const UScriptStruct* ScriptStructB = B.DataPinProperty.GetScriptStruct();
	if (ScriptStructA != ScriptStructB)
	{
		return false;
	}

	return A.DataPinProperty == B.DataPinProperty;
}

#endif
