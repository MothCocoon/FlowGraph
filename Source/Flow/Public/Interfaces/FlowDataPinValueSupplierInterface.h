// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowDataPinResults.h"

#include "UObject/Interface.h"

#include "FlowDataPinValueSupplierInterface.generated.h"

// Interface to define a Flow Data Pin value supplier.  This is generally a UFlowNode subclass, 
// but we may support external suppliers that are not flow nodes in the future 
// (eg, for supplying configuration values for the root graph)
UINTERFACE(MinimalAPI, Blueprintable, DisplayName = "Flow Data Pin Value Supplier Interface")
class UFlowDataPinValueSupplierInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowDataPinValueSupplierInterface
{
	GENERATED_BODY()

public:
	// Can this node actually supply Data Pin values?
	// Implementers of this interface will need to use their own logic to answer this question.
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Can Supply DataPin Values")
	bool CanSupplyDataPinValues() const;
	virtual bool CanSupplyDataPinValues_Implementation() const { return true; }

	// Must implement TrySupplyDataAs... for every EFlowPinType
	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	// Try to supply the value for a data Bool pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Bool")
	FFlowDataPinResult_Bool TrySupplyDataPinAsBool(const FName& PinName) const;
	virtual FFlowDataPinResult_Bool TrySupplyDataPinAsBool_Implementation(const FName& PinName) const { return FFlowDataPinResult_Bool(); }

	// Try to supply the value for a data Int pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Int")
	FFlowDataPinResult_Int TrySupplyDataPinAsInt(const FName& PinName) const;
	virtual FFlowDataPinResult_Int TrySupplyDataPinAsInt_Implementation(const FName& PinName) const { return FFlowDataPinResult_Int(); }

	// Try to supply the value for a data Float pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Float")
	FFlowDataPinResult_Float TrySupplyDataPinAsFloat(const FName& PinName) const;
	virtual FFlowDataPinResult_Float TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const { return FFlowDataPinResult_Float(); }

	// Try to supply the value for a data Name pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Name")
	FFlowDataPinResult_Name TrySupplyDataPinAsName(const FName& PinName) const;
	virtual FFlowDataPinResult_Name TrySupplyDataPinAsName_Implementation(const FName& PinName) const { return FFlowDataPinResult_Name(); }

	// Try to supply the value for a data String pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As String")
	FFlowDataPinResult_String TrySupplyDataPinAsString(const FName& PinName) const;
	virtual FFlowDataPinResult_String TrySupplyDataPinAsString_Implementation(const FName& PinName) const { return FFlowDataPinResult_String(); }

	// Try to supply the value for a data Text pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Text")
	FFlowDataPinResult_Text TrySupplyDataPinAsText(const FName& PinName) const;
	virtual FFlowDataPinResult_Text TrySupplyDataPinAsText_Implementation(const FName& PinName) const { return FFlowDataPinResult_Text(); }

	// Try to supply the value for a data Enum pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Enum")
	FFlowDataPinResult_Enum TrySupplyDataPinAsEnum(const FName& PinName) const;
	virtual FFlowDataPinResult_Enum TrySupplyDataPinAsEnum_Implementation(const FName& PinName) const { return FFlowDataPinResult_Enum(); }

	// Try to supply the value for a data Vector pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Vector")
	FFlowDataPinResult_Vector TrySupplyDataPinAsVector(const FName& PinName) const;
	virtual FFlowDataPinResult_Vector TrySupplyDataPinAsVector_Implementation(const FName& PinName) const { return FFlowDataPinResult_Vector(); }

	// Try to supply the value for a data Rotator pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Rotator")
	FFlowDataPinResult_Rotator TrySupplyDataPinAsRotator(const FName& PinName) const;
	virtual FFlowDataPinResult_Rotator TrySupplyDataPinAsRotator_Implementation(const FName& PinName) const { return FFlowDataPinResult_Rotator(); }

	// Try to supply the value for a data Transform pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Transform")
	FFlowDataPinResult_Transform TrySupplyDataPinAsTransform(const FName& PinName) const;
	virtual FFlowDataPinResult_Transform TrySupplyDataPinAsTransform_Implementation(const FName& PinName) const { return FFlowDataPinResult_Transform(); }

	// Try to supply the value for a data GameplayTag pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As GameplayTag")
	FFlowDataPinResult_GameplayTag TrySupplyDataPinAsGameplayTag(const FName& PinName) const;
	virtual FFlowDataPinResult_GameplayTag TrySupplyDataPinAsGameplayTag_Implementation(const FName& PinName) const { return FFlowDataPinResult_GameplayTag(); }

	// Try to supply the value for a data GameplayTagContainer pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As GameplayTagContainer")
	FFlowDataPinResult_GameplayTagContainer TrySupplyDataPinAsGameplayTagContainer(const FName& PinName) const;
	virtual FFlowDataPinResult_GameplayTagContainer TrySupplyDataPinAsGameplayTagContainer_Implementation(const FName& PinName) const { return FFlowDataPinResult_GameplayTagContainer(); }

	// Try to supply the value for a data InstancedStruct pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As InstancedStruct")
	FFlowDataPinResult_InstancedStruct TrySupplyDataPinAsInstancedStruct(const FName& PinName) const;
	virtual FFlowDataPinResult_InstancedStruct TrySupplyDataPinAsInstancedStruct_Implementation(const FName& PinName) const { return FFlowDataPinResult_InstancedStruct(); }

	// Try to supply the value for a data Object pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Object")
	FFlowDataPinResult_Object TrySupplyDataPinAsObject(const FName& PinName) const;
	virtual FFlowDataPinResult_Object TrySupplyDataPinAsObject_Implementation(const FName& PinName) const { return FFlowDataPinResult_Object(); }

	// Try to supply the value for a data Class pin
	UFUNCTION(BlueprintNativeEvent, Category = DataPins, DisplayName = "Try Supply DataPin As Class")
	FFlowDataPinResult_Class TrySupplyDataPinAsClass(const FName& PinName) const;
	virtual FFlowDataPinResult_Class TrySupplyDataPinAsClass_Implementation(const FName& PinName) const { return FFlowDataPinResult_Class(); }
};
