// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Engine/DataAsset.h"
#include "Types/FlowDataPinProperties.h"
#include "Interfaces/FlowDataPinValueSupplierInterface.h"
#include "Interfaces/FlowAssetProviderInterface.h"
#include "Asset/FlowAssetParamsTypes.h"

#include "FlowAssetParams.generated.h"

class UFlowAsset;

/**
* Data asset for storing Flow Graph Start node parameters, supporting external configuration.
*/
UCLASS(BlueprintType)
class FLOW_API UFlowAssetParams : public UDataAsset, public IFlowDataPinValueSupplierInterface, public IFlowAssetProviderInterface
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
	TMap<FName, TInstancedStruct<FFlowDataPinProperty>> PropertyMap;

public:
	// UObject interface
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;
	// --

	// IFlowDataPinValueSupplierInterface
	virtual bool CanSupplyDataPinValues_Implementation() const override;
	virtual FFlowDataPinResult_Bool TrySupplyDataPinAsBool_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Int TrySupplyDataPinAsInt_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Float TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Name TrySupplyDataPinAsName_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_String TrySupplyDataPinAsString_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Text TrySupplyDataPinAsText_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Enum TrySupplyDataPinAsEnum_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Vector TrySupplyDataPinAsVector_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Rotator TrySupplyDataPinAsRotator_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Transform TrySupplyDataPinAsTransform_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_GameplayTag TrySupplyDataPinAsGameplayTag_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_GameplayTagContainer TrySupplyDataPinAsGameplayTagContainer_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_InstancedStruct TrySupplyDataPinAsInstancedStruct_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Object TrySupplyDataPinAsObject_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Class TrySupplyDataPinAsClass_Implementation(const FName& PinName) const override;
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

protected:

	// Updates properties from ParentParams, handling inheritance and name enforcement.
	EFlowReconcilePropertiesResult ReconcilePropertiesWithParentParams();

	bool TryCheckOutFromSourceControl() const;

	EFlowReconcilePropertiesResult CheckForParentCycle() const;

	void ModifyAndRebuildPropertiesMap();

	void RebuildPropertiesMap();
#endif
};