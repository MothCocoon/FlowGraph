// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowDataPinProperties.h"
#include "FlowDataPinResults.h"

#include "FlowDataPinBlueprintLibrary.generated.h"

// Auto-cast operators for blueprint to their inner types
UCLASS()
class UFlowDataPinBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = FlowPin, Meta = (BlueprintThreadSafe, DisplayName = "Make Flow Pin"))
	static
		UPARAM(DisplayName = "Flow Pin") FFlowPin
		MakeStruct(
			FName PinName,
			FText PinFriendlyName,
			FString PinToolTip)
	{
		return FFlowPin(PinName, PinFriendlyName, PinToolTip);
	}

	UFUNCTION(BlueprintPure, Category = FlowPin, Meta = (BlueprintThreadSafe, DisplayName = "Break Flow Pin"))
	static void
		BreakStruct(
			UPARAM(DisplayName = "Flow Pin") FFlowPin Ref,
			FName& OutPinName,
			FText& OutPinFriendlyName,
			FString& OutPinToolTip)
	{
		OutPinName = Ref.PinName;
		OutPinFriendlyName = Ref.PinFriendlyName;
		OutPinToolTip = Ref.PinToolTip;
	}

	// Recommend implementing AutoConvert_FlowDataPinProperty... for every EFlowPinType
	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	// FFlowDataPinProperty auto-cast functions

	// Convert bool property values to their inner blue values
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Bool", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static bool AutoConvert_FlowDataPinPropertyBoolToBool(const FFlowDataPinOutputProperty_Bool& BoolProperty) { return BoolProperty.Value; }

	// to Int variants for all int types
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Int", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static int32 AutoConvert_FlowDataPinPropertyInt32ToInt32(const FFlowDataPinOutputProperty_Int32& IntProperty) { return IntProperty.Value; }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Int64", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static int64 AutoConvert_FlowDataPinPropertyInt32ToInt64(const FFlowDataPinOutputProperty_Int32& IntProperty) { return static_cast<int64>(IntProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Int", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static int32 AutoConvert_FlowDataPinPropertyInt64ToInt32(const FFlowDataPinOutputProperty_Int64& IntProperty) { /* possible loss of precision */ return static_cast<int32>(IntProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Int64", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static int64 AutoConvert_FlowDataPinPropertyInt64ToInt64(const FFlowDataPinOutputProperty_Int64& IntProperty) { return IntProperty.Value; }

	// to Float variants for all float types
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Float", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static float AutoConvert_FlowDataPinPropertyFloat32ToFloat32(const FFlowDataPinOutputProperty_Float& FloatProperty) { return FloatProperty.Value; }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Double", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static double AutoConvert_FlowDataPinPropertyFloat32ToFloat64(const FFlowDataPinOutputProperty_Float& FloatProperty) { return static_cast<double>(FloatProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Float", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static float AutoConvert_FlowDataPinPropertyFloat64ToFloat32(const FFlowDataPinOutputProperty_Double& FloatProperty) { /* possible loss of precision */ return static_cast<float>(FloatProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Double", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static double AutoConvert_FlowDataPinPropertyFloat64ToFloat64(const FFlowDataPinOutputProperty_Double& FloatProperty) { return FloatProperty.Value; }

	// to Name variants for all text-based types
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Name", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FName AutoConvert_FlowDataPinPropertyNameToName(const FFlowDataPinOutputProperty_Name& NameProperty) { return NameProperty.Value; }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Name", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FName AutoConvert_FlowDataPinPropertyStringToName(const FFlowDataPinOutputProperty_String& StringProperty) { return FName(StringProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Name", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FName AutoConvert_FlowDataPinPropertyTextToName(const FFlowDataPinOutputProperty_Text& TextProperty) { return FName(TextProperty.Value.ToString()); }

	// to String variants for all text-based types
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to String", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FString AutoConvert_FlowDataPinPropertyNameToString(const FFlowDataPinOutputProperty_Name& NameProperty) { return NameProperty.Value.ToString(); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to String", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FString AutoConvert_FlowDataPinPropertyStringToString(const FFlowDataPinOutputProperty_String& StringProperty) { return StringProperty.Value; }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to String", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FString AutoConvert_FlowDataPinPropertyTextToString(const FFlowDataPinOutputProperty_Text& TextProperty) { return TextProperty.Value.ToString(); }

	// to Text variants for all text-based types
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Text", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FText AutoConvert_FlowDataPinPropertyNameToText(const FFlowDataPinOutputProperty_Name& NameProperty) { return FText::FromName(NameProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Text", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FText AutoConvert_FlowDataPinPropertyStringToText(const FFlowDataPinOutputProperty_String& StringProperty) { return FText::FromString(StringProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Text", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FText AutoConvert_FlowDataPinPropertyTextToText(const FFlowDataPinOutputProperty_Text& TextProperty) { return TextProperty.Value; }

	// Convert enum property values to their inner enum values
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Enum", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static uint8 AutoConvert_FlowDataPinPropertyEnumToEnum(const FFlowDataPinOutputProperty_Enum& EnumProperty);

	// Convert vector property values to their inner Vector
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Vector", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FVector AutoConvert_FlowDataPinPropertyVectorToVector(const FFlowDataPinOutputProperty_Vector& VectorProperty) { return VectorProperty.Value; }

	// Convert Rotator property values to their inner Rotator
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Rotator", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FRotator AutoConvert_FlowDataPinPropertyRotatorToRotator(const FFlowDataPinOutputProperty_Rotator& RotatorProperty) { return RotatorProperty.Value; }

	// Convert Transform property values to their inner Transform
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Transform", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FTransform AutoConvert_FlowDataPinPropertyTransformToTransform(const FFlowDataPinOutputProperty_Transform& TransformProperty) { return TransformProperty.Value; }

	// Convert GameplayTag property values to their inner GameplayTag
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to GameplayTag", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FGameplayTag AutoConvert_FlowDataPinPropertyGameplayTagToGameplayTag(const FFlowDataPinOutputProperty_GameplayTag& GameplayTagProperty) { return GameplayTagProperty.Value; }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to GameplayTagContainer", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FGameplayTagContainer AutoConvert_FlowDataPinPropertyGameplayTagToGameplayTagContainer(const FFlowDataPinOutputProperty_GameplayTag& GameplayTagProperty) { return FGameplayTagContainer(GameplayTagProperty.Value); }

	// Convert GameplayTagContainer property values to their inner GameplayTagContainer
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to GameplayTagContainer", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FGameplayTagContainer AutoConvert_FlowDataPinPropertyGameplayTagContainerToGameplayTagContainer(const FFlowDataPinOutputProperty_GameplayTagContainer& GameplayTagContainerProperty) { return GameplayTagContainerProperty.Value; }

	// Convert InstancedStruct property values to their inner InstancedStruct
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to InstancedStruct", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FInstancedStruct AutoConvert_FlowDataPinPropertyInstancedStructToInstancedStruct(const FFlowDataPinOutputProperty_InstancedStruct& InstancedStructProperty) { return InstancedStructProperty.Value; }

	// Convert Object property values to their inner Object
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Object", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static UObject* AutoConvert_FlowDataPinPropertyObjectToObject(const FFlowDataPinOutputProperty_Object& ObjectProperty) { return ObjectProperty.GetObjectValue(); }

	// Convert Class property values to their inner Class
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Class", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static UClass* AutoConvert_FlowDataPinPropertyClassToClass(const FFlowDataPinOutputProperty_Class& ClassProperty) { return ClassProperty.GetResolvedClass(); }

	// Convert Class property values to their FSoftClassPath
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to SoftClass", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FSoftClassPath AutoConvert_FlowDataPinPropertyClassToSoftClass(const FFlowDataPinOutputProperty_Class& ClassProperty) { return ClassProperty.GetAsSoftClass(); }

	// Recommend implementing AutoConvert_FlowDataPinResult... for every EFlowPinType
	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	// FFlowDataPinResults auto-cast functions

	// Convert bool property values to their inner blueprint values
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Bool", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static bool AutoConvert_FlowDataPinResultBoolToBool(const FFlowDataPinResult_Bool& BoolProperty) { return BoolProperty.Value; }

	// to Int variants for all int types (that blueprint supports
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Int", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static int32 AutoConvert_FlowDataPinResultInt64ToInt32(const FFlowDataPinResult_Int& IntProperty) { /* possible loss of precision */ return static_cast<int32>(IntProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Int64", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static int64 AutoConvert_FlowDataPinResultInt64ToInt64(const FFlowDataPinResult_Int& IntProperty) { return IntProperty.Value; }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Int", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static uint8 AutoConvert_FlowDataPinResultInt64ToUint8(const FFlowDataPinResult_Int& IntProperty) { /* possible loss of precision */ return static_cast<uint8>(IntProperty.Value); }

	// to Float variants for all float types
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Float", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static float AutoConvert_FlowDataPinResultFloat64ToFloat32(const FFlowDataPinResult_Float& FloatProperty) { /* possible loss of precision */ return static_cast<float>(FloatProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Double", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static double AutoConvert_FlowDataPinResultFloat64ToFloat64(const FFlowDataPinResult_Float& FloatProperty) { return FloatProperty.Value; }

	// to Name variants for all text-based types
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Name", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FName AutoConvert_FlowDataPinResultNameToName(const FFlowDataPinResult_Name& NameProperty) { return NameProperty.Value; }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Name", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FName AutoConvert_FlowDataPinResultStringToName(const FFlowDataPinResult_String& StringProperty) { return FName(StringProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Name", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FName AutoConvert_FlowDataPinResultTextToName(const FFlowDataPinResult_Text& TextProperty) { return FName(TextProperty.Value.ToString()); }

	// to String variants for all text-based types
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to String", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FString AutoConvert_FlowDataPinResultNameToString(const FFlowDataPinResult_Name& NameProperty) { return NameProperty.Value.ToString(); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to String", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FString AutoConvert_FlowDataPinResultStringToString(const FFlowDataPinResult_String& StringProperty) { return StringProperty.Value; }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to String", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FString AutoConvert_FlowDataPinResultTextToString(const FFlowDataPinResult_Text& TextProperty) { return TextProperty.Value.ToString(); }

	// to Text variants for all text-based types
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Text", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FText AutoConvert_FlowDataPinResultNameToText(const FFlowDataPinResult_Name& NameProperty) { return FText::FromName(NameProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Text", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FText AutoConvert_FlowDataPinResultStringToText(const FFlowDataPinResult_String& StringProperty) { return FText::FromString(StringProperty.Value); }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Text", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FText AutoConvert_FlowDataPinResultTextToText(const FFlowDataPinResult_Text& TextProperty) { return TextProperty.Value; }

	// Convert enum property values to their inner enum values
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Enum", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static uint8 AutoConvert_FlowDataPinResultEnumToEnum(const FFlowDataPinResult_Enum& EnumProperty);

	// Convert vector property values to their inner Vector
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Vector", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FVector AutoConvert_FlowDataPinResultVectorToVector(const FFlowDataPinResult_Vector& VectorProperty) { return VectorProperty.Value; }

	// Convert Rotator property values to their inner Rotator
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Rotator", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FRotator AutoConvert_FlowDataPinResultRotatorToRotator(const FFlowDataPinResult_Rotator& RotatorProperty) { return RotatorProperty.Value; }

	// Convert Transform property values to their inner Transform
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Transform", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FTransform AutoConvert_FlowDataPinResultTransformToTransform(const FFlowDataPinResult_Transform& TransformProperty) { return TransformProperty.Value; }

	// Convert GameplayTag property values to their inner GameplayTag
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to GameplayTag", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FGameplayTag AutoConvert_FlowDataPinResultGameplayTagToGameplayTag(const FFlowDataPinResult_GameplayTag& GameplayTagProperty) { return GameplayTagProperty.Value; }

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to GameplayTagContainer", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FGameplayTagContainer AutoConvert_FlowDataPinResultGameplayTagToGameplayTagContainer(const FFlowDataPinResult_GameplayTag& GameplayTagProperty) { return FGameplayTagContainer(GameplayTagProperty.Value); }

	// Convert GameplayTagContainer property values to their inner GameplayTagContainer
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to GameplayTagContainer", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FGameplayTagContainer AutoConvert_FlowDataPinResultGameplayTagContainerToGameplayTagContainer(const FFlowDataPinResult_GameplayTagContainer& GameplayTagContainerProperty) { return GameplayTagContainerProperty.Value; }

	// Convert InstancedStruct property values to their inner InstancedStruct
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to InstancedStruct", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FInstancedStruct AutoConvert_FlowDataPinResultInstancedStructToInstancedStruct(const FFlowDataPinResult_InstancedStruct& InstancedStructProperty) { return InstancedStructProperty.Value; }

	// Convert Object property values to their inner Object
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Object", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static UObject* AutoConvert_FlowDataPinResultObjectToObject(const FFlowDataPinResult_Object& ObjectProperty) { return ObjectProperty.Value; }

	// Convert Class property values to their inner Class
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Class", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static UClass* AutoConvert_FlowDataPinResultClassToClass(const FFlowDataPinResult_Class& ClassProperty) { return ClassProperty.GetOrResolveClass(); }

	// Convert Class property values to their the FSoftClassPath
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to SoftClass", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FSoftClassPath AutoConvert_FlowDataPinResultClassToSoftClass(const FFlowDataPinResult_Class& ClassProperty) { return ClassProperty.GetAsSoftClass(); }
};