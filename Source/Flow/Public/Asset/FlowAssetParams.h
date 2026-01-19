// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Engine/DataAsset.h"
#include "Types/FlowNamedDataPinProperty.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "Interfaces/FlowDataPinValueSupplierInterface.h"
#include "Interfaces/FlowAssetProviderInterface.h"
#include "Asset/FlowAssetParamsTypes.h"

#include "FlowAssetParams.generated.h"

class UFlowAsset;

/**
* Data asset for storing Flow Graph Start node parameters, supporting external configuration.
* This is considered experimental at the moment.
*/
UCLASS(BlueprintType)
class FLOW_API UFlowAssetParams
	: public UDataAsset
	, public IFlowAssetProviderInterface
	, public IFlowDataPinValueOwnerInterface
	, public IFlowDataPinValueSupplierInterface
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	// Reference to the associated Flow Asset.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = FlowAssetParams)
	TSoftObjectPtr<UFlowAsset> OwnerFlowAsset;

	// Reference to the "Parent" params object to inherit from (if any).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = FlowAssetParams)
	FFlowAssetParamsPtr ParentParams;

	// Array of properties synchronized with the Start node (local adds/overrides; effective flattened via ReconcilePropertiesWithParentParams).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FlowAssetParams, meta = (EditFixedSize))
	TArray<FFlowNamedDataPinProperty> Properties;
#endif

	UPROPERTY()
	TMap<FName, TInstancedStruct<FFlowDataPinValue>> PropertyMap;

public:
	// UObject interface
#if WITH_EDITOR
	virtual void PostLoad() override;
	virtual void PreSaveRoot(FObjectPreSaveRootContext ObjectSaveContext) override;
#endif
	virtual void Serialize(FArchive& Ar) override;
	// --

	// IFlowDataPinValueSupplierInterface
	virtual bool CanSupplyDataPinValues_Implementation() const override;
	virtual FFlowDataPinResult TrySupplyDataPin_Implementation(FName PinName) const override;
	// --

	// IFlowAssetProviderInterface
	virtual UFlowAsset* ProvideFlowAsset() const override;
	// --

#if WITH_EDITOR
	// UObject interface
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	// --

	// Generates properties from the associated Start node or updates Start node from params.
	EFlowReconcilePropertiesResult ReconcilePropertiesWithStartNode(
		const FDateTime& FlowAssetLastSaveTimeStamp,
		const TSoftObjectPtr<UFlowAsset>& InOwnerFlowAsset,
		TArray<FFlowNamedDataPinProperty>& MutablePropertiesFromStartNode);

	void ConfigureFlowAssetParams(TSoftObjectPtr<UFlowAsset> OwnerAsset, TSoftObjectPtr<UFlowAssetParams> InParentParams, const TArray<FFlowNamedDataPinProperty>& InProperties);

	// IFlowDataPinValueOwnerInterface
	virtual bool CanModifyFlowDataPinType() const override;
	virtual bool ShowFlowDataPinValueInputPinCheckbox() const override;
	virtual bool ShowFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const override;
	virtual bool CanEditFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const override;
	virtual void SetFlowDataPinValuesRebuildDelegate(FSimpleDelegate InDelegate) override
	{
		FlowDataPinValuesRebuildDelegate = InDelegate;
	}

	virtual void RequestFlowDataPinValuesDetailsRebuild() override
	{
		if (FlowDataPinValuesRebuildDelegate.IsBound())
		{
			FlowDataPinValuesRebuildDelegate.Execute();
		}
	}

private:
	FSimpleDelegate FlowDataPinValuesRebuildDelegate;
	// --

protected:

	// Updates properties from ParentParams, handling inheritance and name enforcement.
	EFlowReconcilePropertiesResult ReconcilePropertiesWithParentParams();

	EFlowReconcilePropertiesResult CheckForParentCycle() const;

	void ModifyAndRebuildPropertiesMap();

	void RebuildPropertiesMap();
#endif
};