// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowEnumUtils.h"
#include "UObject/SoftObjectPtr.h"

#include "FlowAssetParamsTypes.generated.h"

class UFlowAssetParams;

// Result of reconciling Flow Asset Params with Start node or SuperParams.
UENUM(BlueprintType)
enum class EFlowReconcilePropertiesResult : uint8
{
	NoChanges,

	ParamsPropertiesUpdated,
	AssetPropertyValuesUpdated,

	Error_InvalidAsset,
	Error_PropertyCountMismatch,
	Error_PropertyTypeMismatch,
	Error_CyclicInheritance,
	Error_UnloadableParent,

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),

	SuccessFirst = NoChanges UMETA(Hidden),
	SuccessLast = AssetPropertyValuesUpdated UMETA(Hidden),

	ModifiedFirst = ParamsPropertiesUpdated UMETA(Hidden),
	ModifiedLast = AssetPropertyValuesUpdated UMETA(Hidden),

	ErrorFirst = Error_InvalidAsset UMETA(Hidden),
	ErrorLast = Error_UnloadableParent UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowReconcilePropertiesResult)

namespace EFlowReconcilePropertiesResult_Classifiers
{
	FORCEINLINE bool IsSuccessResult(EFlowReconcilePropertiesResult Result) { return FLOW_IS_ENUM_IN_SUBRANGE(Result, EFlowReconcilePropertiesResult::Success); }
	FORCEINLINE bool IsModifiedResult(EFlowReconcilePropertiesResult Result) { return FLOW_IS_ENUM_IN_SUBRANGE(Result, EFlowReconcilePropertiesResult::Modified); }
	FORCEINLINE bool IsErrorResult(EFlowReconcilePropertiesResult Result) { return FLOW_IS_ENUM_IN_SUBRANGE(Result, EFlowReconcilePropertiesResult::Error); }
}

// Wrapper for TSoftObjectPtr<UFlowAssetParams> to enable editor customization.
//
// Supported metadata tags:
// - ShowCreateNew - Should we show the "Create New" button?
// - HideChildParams - When showing a chooser, should we hide "Child" params or not? (Child params have a non-null ParentParams)
USTRUCT(BlueprintType)
struct FLOW_API FFlowAssetParamsPtr
{
	GENERATED_BODY()

	FFlowAssetParamsPtr() = default;
	FFlowAssetParamsPtr(TSoftObjectPtr<UFlowAssetParams> InAssetParamsPtr) : AssetPtr(InAssetParamsPtr) { }

	UFlowAssetParams* ResolveFlowAssetParams() const;

	// Reference to the Flow Asset Params.
	UPROPERTY(EditAnywhere, Category = FlowAssetParams, meta = (EditAssetInline))
	TSoftObjectPtr<UFlowAssetParams> AssetPtr;
};
