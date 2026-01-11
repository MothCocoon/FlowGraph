// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "FlowDataPinValuesStandard.h"
#include "FlowDataPinResults.h"
#include "FlowPinTypeTemplates.h"
#include "FlowDataPinBlueprintLibrary.generated.h"

struct FFlowDataPinValue;

// Auto‑cast operators for blueprint to their inner types
UCLASS()
class UFlowDataPinBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	static void ResolveAndExtract_Impl(
		UFlowNodeBase* Target,
		FName PinName,
		EFlowDataPinResolveSimpleResult& SimpleResult,
		EFlowDataPinResolveResult& ResultEnum,
		auto&& ExtractLambda);

public:

	// ----------  Pin construction helpers  ----------

	UFUNCTION(BlueprintPure, Category = FlowPin, Meta = (BlueprintThreadSafe, DisplayName = "Make Flow Pin"))
	static UPARAM(DisplayName = "Flow Pin") FFlowPin MakeStruct(FName PinName, FText PinFriendlyName, FString PinToolTip)
	{
		return FFlowPin(PinName, PinFriendlyName, PinToolTip);
	}

	UFUNCTION(BlueprintPure, Category = FlowPin, Meta = (BlueprintThreadSafe, DisplayName = "Break Flow Pin"))
	static void BreakStruct(UPARAM(DisplayName = "Flow Pin") FFlowPin Ref, FName& OutPinName, FText& OutPinFriendlyName, FString& OutPinToolTip)
	{
		OutPinName = Ref.PinName;
		OutPinFriendlyName = Ref.PinFriendlyName;
		OutPinToolTip = Ref.PinToolTip;
	}

	// ----------  Resolve As ... functions ----------
	// Full-featured resolve nodes with execution pins for detailed error handling.
	// Use these when you need to branch on Success/Failure/Coercion or inspect the exact resolve result.

	// Resolve a Bool DataPin Value to a single bool.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Bool", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsBool(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Bool& BoolValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, bool& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a Bool DataPin Value to a bool array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Bool Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsBoolArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Bool& BoolValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<bool>& Values);

	// Resolve an Int DataPin Value to a single int32.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Int", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsInt(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Int& IntValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, int32& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve an Int DataPin Value to an int32 array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Int Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsIntArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Int& IntValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<int32>& Values);

	// Resolve an Int64 DataPin Value to a single int64.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Int64", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsInt64(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Int64& Int64Value, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, int64& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve an Int64 DataPin Value to an int64 array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Int64 Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsInt64Array(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Int64& Int64Value, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<int64>& Values);

	// Resolve a Float DataPin Value to a single float.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Float", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsFloat(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Float& FloatValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, float& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a Float DataPin Value to a float array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Float Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsFloatArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Float& FloatValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<float>& Values);

	// Resolve a Double DataPin Value to a single double.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Double", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsDouble(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Double& DoubleValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, double& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a Double DataPin Value to a double array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Double Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsDoubleArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Double& DoubleValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<double>& Values);

	// Resolve a Name DataPin Value to a single FName.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Name", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsName(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Name& NameValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FName& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a Name DataPin Value to an FName array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Name Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsNameArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Name& NameValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FName>& Values);

	// Resolve a String DataPin Value to a single FString.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As String", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsString(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_String& StringValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FString& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a String DataPin Value to an FString array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As String Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsStringArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_String& StringValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FString>& Values);

	// Resolve a Text DataPin Value to a single FText.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Text", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsText(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Text& TextValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FText& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a Text DataPin Value to an FText array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Text Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsTextArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Text& TextValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FText>& Values);

	// Resolve an Enum DataPin Value to a single uint8.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Enum", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsEnum(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Enum& EnumValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, uint8& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve an Enum DataPin Value to a uint8 array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Enum Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsEnumArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Enum& EnumValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<uint8>& Values);

	// Resolve a Vector DataPin Value to a single FVector.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Vector", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsVector(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Vector& VectorValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FVector& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a Vector DataPin Value to an FVector array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Vector Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsVectorArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Vector& VectorValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FVector>& Values);

	// Resolve a Rotator DataPin Value to a single FRotator.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Rotator", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsRotator(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Rotator& RotatorValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FRotator& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a Rotator DataPin Value to an FRotator array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Rotator Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsRotatorArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Rotator& RotatorValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FRotator>& Values);

	// Resolve a Transform DataPin Value to a single FTransform.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Transform", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsTransform(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Transform& TransformValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FTransform& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a Transform DataPin Value to an FTransform array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Transform Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsTransformArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Transform& TransformValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FTransform>& Values);

	// Resolve a GameplayTag DataPin Value to a single FGameplayTag.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As GameplayTag", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsGameplayTag(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_GameplayTag& GameplayTagValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FGameplayTag& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a GameplayTag DataPin Value to an FGameplayTag array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As GameplayTag Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsGameplayTagArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_GameplayTag& GameplayTagValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FGameplayTag>& Values);

	// Resolve a GameplayTagContainer DataPin Value (scalar only).
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As GameplayTagContainer", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsGameplayTagContainer(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_GameplayTagContainer& GameplayTagContainerValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FGameplayTagContainer& Value);

	// Resolve an InstancedStruct DataPin Value to a single FInstancedStruct.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As InstancedStruct", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsInstancedStruct(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_InstancedStruct& InstancedStructValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FInstancedStruct& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve an InstancedStruct DataPin Value to an FInstancedStruct array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As InstancedStruct Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsInstancedStructArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_InstancedStruct& InstancedStructValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FInstancedStruct>& Values);

	// Resolve an Object DataPin Value to a single UObject*.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Object", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsObject(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Object& ObjectValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, UObject*& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve an Object DataPin Value to a UObject* array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Object Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsObjectArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Object& ObjectValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<UObject*>& Values);

	// Resolve a Class DataPin Value to a single UClass*.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Class", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsClass(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Class& ClassValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, UClass*& Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Resolve a Class DataPin Value to a UClass* array.
	UFUNCTION(BlueprintCallable, Category = DataPins, meta = (DisplayName = "Resolve As Class Array", DefaultToSelf = "Target", ExpandEnumAsExecs = "Result"))
	static void ResolveAsClassArray(UFlowNodeBase* Target, UPARAM(Ref) const FFlowDataPinValue_Class& ClassValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<UClass*>& Values);

	// ----------  Auto-Resolve As ... functions ----------
	// Easy-resolve convenience nodes. On failure, logs an error and returns a safe default (false/empty/null).
	// Use these for fast, fire-and-forget resolving when you don't expect failures.

	// Easy Resolve a Bool DataPin Value to a single bool (last value if array). Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Bool", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static bool AutoConvert_TryResolveAsBool(UPARAM(Ref) const FFlowDataPinValue_Bool& BoolValue, const UFlowNodeBase* Target);

	// Easy Resolve a Bool DataPin Value to a bool array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Bool Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<bool> AutoConvert_TryResolveAsBoolArray(UPARAM(Ref) const FFlowDataPinValue_Bool& BoolValue, const UFlowNodeBase* Target);

	// Easy Resolve an Int DataPin Value to a single int32. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Int", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static int32 AutoConvert_TryResolveAsInt(UPARAM(Ref) const FFlowDataPinValue_Int& IntValue, const UFlowNodeBase* Target);

	// Easy Resolve an Int DataPin Value to an int32 array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Int Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<int32> AutoConvert_TryResolveAsIntArray(UPARAM(Ref) const FFlowDataPinValue_Int& IntValue, const UFlowNodeBase* Target);

	// Easy Resolve an Int64 DataPin Value to a single int64. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Int64", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static int64 AutoConvert_TryResolveAsInt64(UPARAM(Ref) const FFlowDataPinValue_Int64& Int64Value, const UFlowNodeBase* Target);

	// Easy Resolve an Int64 DataPin Value to an int64 array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Int64 Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<int64> AutoConvert_TryResolveAsInt64Array(UPARAM(Ref) const FFlowDataPinValue_Int64& Int64Value, const UFlowNodeBase* Target);

	// Easy Resolve a Float DataPin Value to a single float. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Float", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static float AutoConvert_TryResolveAsFloat(UPARAM(Ref) const FFlowDataPinValue_Float& FloatValue, const UFlowNodeBase* Target);

	// Easy Resolve a Float DataPin Value to a float array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Float Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<float> AutoConvert_TryResolveAsFloatArray(UPARAM(Ref) const FFlowDataPinValue_Float& FloatValue, const UFlowNodeBase* Target);

	// Easy Resolve a Double DataPin Value to a single double. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Double", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static double AutoConvert_TryResolveAsDouble(UPARAM(Ref) const FFlowDataPinValue_Double& DoubleValue, const UFlowNodeBase* Target);

	// Easy Resolve a Double DataPin Value to a double array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Double Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<double> AutoConvert_TryResolveAsDoubleArray(UPARAM(Ref) const FFlowDataPinValue_Double& DoubleValue, const UFlowNodeBase* Target);

	// Easy Resolve a Name DataPin Value to a single FName. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Name", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static FName AutoConvert_TryResolveAsName(UPARAM(Ref) const FFlowDataPinValue_Name& NameValue, const UFlowNodeBase* Target);

	// Easy Resolve a Name DataPin Value to an FName array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Name Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<FName> AutoConvert_TryResolveAsNameArray(UPARAM(Ref) const FFlowDataPinValue_Name& NameValue, const UFlowNodeBase* Target);

	// Easy Resolve a String DataPin Value to a single FString. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to String", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static FString AutoConvert_TryResolveAsString(UPARAM(Ref) const FFlowDataPinValue_String& StringValue, const UFlowNodeBase* Target);

	// Easy Resolve a String DataPin Value to an FString array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to String Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<FString> AutoConvert_TryResolveAsStringArray(UPARAM(Ref) const FFlowDataPinValue_String& StringValue, const UFlowNodeBase* Target);

	// Easy Resolve a Text DataPin Value to a single FText. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Text", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static FText AutoConvert_TryResolveAsText(UPARAM(Ref) const FFlowDataPinValue_Text& TextValue, const UFlowNodeBase* Target);

	// Easy Resolve a Text DataPin Value to an FText array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Text Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<FText> AutoConvert_TryResolveAsTextArray(UPARAM(Ref) const FFlowDataPinValue_Text& TextValue, const UFlowNodeBase* Target);

	// Easy Resolve an Enum DataPin Value to a single uint8. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Enum", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static uint8 AutoConvert_TryResolveAsEnum(UPARAM(Ref) const FFlowDataPinValue_Enum& EnumValue, const UFlowNodeBase* Target);

	// Easy Resolve an Enum DataPin Value to a uint8 array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Enum Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<uint8> AutoConvert_TryResolveAsEnumArray(UPARAM(Ref) const FFlowDataPinValue_Enum& EnumValue, const UFlowNodeBase* Target);

	// Easy Resolve a Vector DataPin Value to a single FVector. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Vector", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static FVector AutoConvert_TryResolveAsVector(UPARAM(Ref) const FFlowDataPinValue_Vector& VectorValue, const UFlowNodeBase* Target);

	// Easy Resolve a Vector DataPin Value to an FVector array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Vector Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<FVector> AutoConvert_TryResolveAsVectorArray(UPARAM(Ref) const FFlowDataPinValue_Vector& VectorValue, const UFlowNodeBase* Target);

	// Easy Resolve a Rotator DataPin Value to a single FRotator. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Rotator", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static FRotator AutoConvert_TryResolveAsRotator(UPARAM(Ref) const FFlowDataPinValue_Rotator& RotatorValue, const UFlowNodeBase* Target);

	// Easy Resolve a Rotator DataPin Value to an FRotator array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Rotator Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<FRotator> AutoConvert_TryResolveAsRotatorArray(UPARAM(Ref) const FFlowDataPinValue_Rotator& RotatorValue, const UFlowNodeBase* Target);

	// Easy Resolve a Transform DataPin Value to a single FTransform. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Transform", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static FTransform AutoConvert_TryResolveAsTransform(UPARAM(Ref) const FFlowDataPinValue_Transform& TransformValue, const UFlowNodeBase* Target);

	// Easy Resolve a Transform DataPin Value to an FTransform array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Transform Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<FTransform> AutoConvert_TryResolveAsTransformArray(UPARAM(Ref) const FFlowDataPinValue_Transform& TransformValue, const UFlowNodeBase* Target);

	// Easy Resolve a GameplayTag DataPin Value to a single FGameplayTag. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to GameplayTag", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static FGameplayTag AutoConvert_TryResolveAsGameplayTag(UPARAM(Ref) const FFlowDataPinValue_GameplayTag& GameplayTagValue, const UFlowNodeBase* Target);

	// Easy Resolve a GameplayTag DataPin Value to an FGameplayTag array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to GameplayTag Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<FGameplayTag> AutoConvert_TryResolveAsGameplayTagArray(UPARAM(Ref) const FFlowDataPinValue_GameplayTag& GameplayTagValue, const UFlowNodeBase* Target);

	// Easy Resolve a GameplayTagContainer DataPin Value (scalar only). Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to GameplayTagContainer", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static FGameplayTagContainer AutoConvert_TryResolveAsGameplayTagContainer(UPARAM(Ref) const FFlowDataPinValue_GameplayTagContainer& GameplayTagContainerValue, const UFlowNodeBase* Target);

	// Easy Resolve an InstancedStruct DataPin Value to a single FInstancedStruct. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to InstancedStruct", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static FInstancedStruct AutoConvert_TryResolveAsInstancedStruct(UPARAM(Ref) const FFlowDataPinValue_InstancedStruct& InstancedStructValue, const UFlowNodeBase* Target);

	// Easy Resolve an InstancedStruct DataPin Value to an FInstancedStruct array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to InstancedStruct Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<FInstancedStruct> AutoConvert_TryResolveAsInstancedStructArray(UPARAM(Ref) const FFlowDataPinValue_InstancedStruct& InstancedStructValue, const UFlowNodeBase* Target);

	// Easy Resolve an Object DataPin Value to a single UObject*. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Object", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static UObject* AutoConvert_TryResolveAsObject(UPARAM(Ref) const FFlowDataPinValue_Object& ObjectValue, const UFlowNodeBase* Target);

	// Easy Resolve an Object DataPin Value to a UObject* array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Object Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<UObject*> AutoConvert_TryResolveAsObjectArray(UPARAM(Ref) const FFlowDataPinValue_Object& ObjectValue, const UFlowNodeBase* Target);

	// Easy Resolve a Class DataPin Value to a single UClass*. Logs error on failure.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Class", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static UClass* AutoConvert_TryResolveAsClass(UPARAM(Ref) const FFlowDataPinValue_Class& ClassValue, const UFlowNodeBase* Target);

	// Easy Resolve a Class DataPin Value to a UClass* array. Logs error on failure and returns empty array.
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Auto-Resolve to Class Array", CompactNodeTitle = "->", BlueprintAutocast, DefaultToSelf = "Target"), Category = DataPins)
	static TArray<UClass*> AutoConvert_TryResolveAsClassArray(UPARAM(Ref) const FFlowDataPinValue_Class& ClassValue, const UFlowNodeBase* Target);

	//  ----------  Result → result enum converter ----------

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Convert to Result", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static EFlowDataPinResolveResult AutoConvert_TryExtractResultEnum(const FFlowDataPinResult& DataPinResult)
	{
		return DataPinResult.Result;
	}

	// ----------  Result → native value extractors  ----------

	// Bool
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Bool", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static bool AutoConvert_TryExtractBool(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Bool Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<bool> AutoConvert_TryExtractBoolArray(const FFlowDataPinResult& DataPinResult);

	// Int (int32)
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Int", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static int32 AutoConvert_TryExtractInt(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Int Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<int32> AutoConvert_TryExtractIntArray(const FFlowDataPinResult& DataPinResult);

	// Int64
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Int64", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static int64 AutoConvert_TryExtractInt64(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Int64 Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<int64> AutoConvert_TryExtractInt64Array(const FFlowDataPinResult& DataPinResult);

	// Float
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Float", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static float AutoConvert_TryExtractFloat(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Float Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<float> AutoConvert_TryExtractFloatArray(const FFlowDataPinResult& DataPinResult);

	// Double
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Double", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static double AutoConvert_TryExtractDouble(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Double Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<double> AutoConvert_TryExtractDoubleArray(const FFlowDataPinResult& DataPinResult);

	// Name
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Name", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FName AutoConvert_TryExtractName(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Name Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<FName> AutoConvert_TryExtractNameArray(const FFlowDataPinResult& DataPinResult);

	// String
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to String", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FString AutoConvert_TryExtractString(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to String Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<FString> AutoConvert_TryExtractStringArray(const FFlowDataPinResult& DataPinResult);

	// Text
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Text", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FText AutoConvert_TryExtractText(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Text Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<FText> AutoConvert_TryExtractTextArray(const FFlowDataPinResult& DataPinResult);

	// Enum
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Enum", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static uint8 AutoConvert_TryExtractEnum(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Enum Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<uint8> AutoConvert_TryExtractEnumArray(const FFlowDataPinResult& DataPinResult);

	// Vector
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Vector", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FVector AutoConvert_TryExtractVector(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Vector Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<FVector> AutoConvert_TryExtractVectorArray(const FFlowDataPinResult& DataPinResult);

	// Rotator
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Rotator", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FRotator AutoConvert_TryExtractRotator(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Rotator Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<FRotator> AutoConvert_TryExtractRotatorArray(const FFlowDataPinResult& DataPinResult);

	// Transform
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Transform", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FTransform AutoConvert_TryExtractTransform(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Transform Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<FTransform> AutoConvert_TryExtractTransformArray(const FFlowDataPinResult& DataPinResult);

	// GameplayTag
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to GameplayTag", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FGameplayTag AutoConvert_TryExtractGameplayTag(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to GameplayTag Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<FGameplayTag> AutoConvert_TryExtractGameplayTagArray(const FFlowDataPinResult& DataPinResult);

	// GameplayTagContainer
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to GameplayTagContainer", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FGameplayTagContainer AutoConvert_TryExtractGameplayTagContainer(const FFlowDataPinResult& DataPinResult);

	// InstancedStruct
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to InstancedStruct", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static FInstancedStruct AutoConvert_TryExtractInstancedStruct(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to InstancedStruct Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<FInstancedStruct> AutoConvert_TryExtractInstancedStructArray(const FFlowDataPinResult& DataPinResult);

	// Object
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Object", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static UObject* AutoConvert_TryExtractObject(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Object Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<UObject*> AutoConvert_TryExtractObjectArray(const FFlowDataPinResult& DataPinResult);

	// Class
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Class", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static UClass* AutoConvert_TryExtractClass(const FFlowDataPinResult& DataPinResult);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Extract to Class Array", CompactNodeTitle = "->", BlueprintAutocast), Category = DataPins)
	static TArray<UClass*> AutoConvert_TryExtractClassArray(const FFlowDataPinResult& DataPinResult);

	// ----------  Get & Set Value functions ----------
	// Direct access to the stored payload on a DataPin Value struct.
	// Set functions: Safe for both input and output pins.
	// Get functions: ONLY safe on output pins. Using on an input pin triggers a runtime error in editor builds.

	// Set a single bool on a Bool DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Bool Value"))
	static void SetBoolValue(bool bInValue, UPARAM(Ref) FFlowDataPinValue_Bool& BoolValue) { BoolValue.Values = { bInValue }; }

	// Set a bool array on a Bool DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Bool Values"))
	static void SetBoolValues(const TArray<bool>& InValues, UPARAM(Ref) FFlowDataPinValue_Bool& BoolValue) { BoolValue.Values = InValues; }

	// Get a single bool from an output Bool DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Bool Value"))
	static bool GetBoolValue(UPARAM(Ref) const FFlowDataPinValue_Bool& BoolValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full bool array from an output Bool DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Bool Values"))
	static TArray<bool> GetBoolValues(UPARAM(Ref) FFlowDataPinValue_Bool& BoolValue);

	// Set a single int32 on an Int DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Int Value"))
	static void SetIntValue(int32 InValue, UPARAM(Ref) FFlowDataPinValue_Int& IntValue) { IntValue.Values = { InValue }; }

	// Set an int32 array on an Int DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Int Values"))
	static void SetIntValues(const TArray<int32>& InValues, UPARAM(Ref) FFlowDataPinValue_Int& IntValue) { IntValue.Values = InValues; }

	// Get a single int32 from an output Int DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Int Value"))
	static int32 GetIntValue(UPARAM(Ref) const FFlowDataPinValue_Int& IntValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full int32 array from an output Int DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Int Values"))
	static TArray<int32> GetIntValues(UPARAM(Ref) FFlowDataPinValue_Int& IntValue);

	// Set a single int64 on an Int64 DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Int64 Value"))
	static void SetInt64Value(int64 InValue, UPARAM(Ref) FFlowDataPinValue_Int64& Int64Value) { Int64Value.Values = { InValue }; }

	// Set an int64 array on an Int64 DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Int64 Values"))
	static void SetInt64Values(const TArray<int64>& InValues, UPARAM(Ref) FFlowDataPinValue_Int64& Int64Value) { Int64Value.Values = InValues; }

	// Get a single int64 from an output Int64 DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Int64 Value"))
	static int64 GetInt64Value(UPARAM(Ref) const FFlowDataPinValue_Int64& Int64Value, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full int64 array from an output Int64 DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Int64 Values"))
	static TArray<int64> GetInt64Values(UPARAM(Ref) FFlowDataPinValue_Int64& Int64Value);

	// Set a single float on a Float DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Float Value"))
	static void SetFloatValue(float InValue, UPARAM(Ref) FFlowDataPinValue_Float& FloatValue) { FloatValue.Values = { InValue }; }

	// Set a float array on a Float DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Float Values"))
	static void SetFloatValues(const TArray<float>& InValues, UPARAM(Ref) FFlowDataPinValue_Float& FloatValue) { FloatValue.Values = InValues; }

	// Get a single float from an output Float DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Float Value"))
	static float GetFloatValue(UPARAM(Ref) const FFlowDataPinValue_Float& FloatValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full float array from an output Float DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Float Values"))
	static TArray<float> GetFloatValues(UPARAM(Ref) FFlowDataPinValue_Float& FloatValue);

	// Set a single double on a Double DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Double Value"))
	static void SetDoubleValue(double InValue, UPARAM(Ref) FFlowDataPinValue_Double& DoubleValue) { DoubleValue.Values = { InValue }; }

	// Set a double array on a Double DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Double Values"))
	static void SetDoubleValues(const TArray<double>& InValues, UPARAM(Ref) FFlowDataPinValue_Double& DoubleValue) { DoubleValue.Values = InValues; }

	// Get a single double from an output Double DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Double Value"))
	static double GetDoubleValue(UPARAM(Ref) const FFlowDataPinValue_Double& DoubleValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full double array from an output Double DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Double Values"))
	static TArray<double> GetDoubleValues(UPARAM(Ref) FFlowDataPinValue_Double& DoubleValue);

	// Set a single FName on a Name DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Name Value"))
	static void SetNameValue(FName InValue, UPARAM(Ref) FFlowDataPinValue_Name& NameValue) { NameValue.Values = { InValue }; }

	// Set an FName array on a Name DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Name Values"))
	static void SetNameValues(const TArray<FName>& InValues, UPARAM(Ref) FFlowDataPinValue_Name& NameValue) { NameValue.Values = InValues; }

	// Get a single FName from an output Name DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Name Value"))
	static FName GetNameValue(UPARAM(Ref) const FFlowDataPinValue_Name& NameValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full FName array from an output Name DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Name Values"))
	static TArray<FName> GetNameValues(UPARAM(Ref) FFlowDataPinValue_Name& NameValue);

	// Set a single FString on a String DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set String Value"))
	static void SetStringValue(const FString& InValue, UPARAM(Ref) FFlowDataPinValue_String& StringValue) { StringValue.Values = { InValue }; }

	// Set an FString array on a String DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set String Values"))
	static void SetStringValues(const TArray<FString>& InValues, UPARAM(Ref) FFlowDataPinValue_String& StringValue) { StringValue.Values = InValues; }

	// Get a single FString from an output String DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get String Value"))
	static FString GetStringValue(UPARAM(Ref) const FFlowDataPinValue_String& StringValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full FString array from an output String DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get String Values"))
	static TArray<FString> GetStringValues(UPARAM(Ref) FFlowDataPinValue_String& StringValue);

	// Set a single FText on a Text DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Text Value"))
	static void SetTextValue(const FText& InValue, UPARAM(Ref) FFlowDataPinValue_Text& TextValue) { TextValue.Values = { InValue }; }

	// Set an FText array on a Text DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Text Values"))
	static void SetTextValues(const TArray<FText>& InValues, UPARAM(Ref) FFlowDataPinValue_Text& TextValue) { TextValue.Values = InValues; }

	// Get a single FText from an output Text DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Text Value"))
	static FText GetTextValue(UPARAM(Ref) const FFlowDataPinValue_Text& TextValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full FText array from an output Text DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Text Values"))
	static TArray<FText> GetTextValues(UPARAM(Ref) FFlowDataPinValue_Text& TextValue);

	// Set a single enum value (as uint8) on an Enum DataPin Value (input or output pin). Requires EnumClass to be set on the struct.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Enum Value"))
	static void SetEnumValue(uint8 InValue, UPARAM(Ref) FFlowDataPinValue_Enum& EnumValue);

	// Set an enum value array (as uint8) on an Enum DataPin Value (input or output pin). Requires EnumClass to be set on the struct.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Enum Values"))
	static void SetEnumValues(const TArray<uint8>& InValues, UPARAM(Ref) FFlowDataPinValue_Enum& EnumValue);

	// Get a single enum value (as uint8) from an output Enum DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Enum Value"))
	static uint8 GetEnumValue(UPARAM(Ref) const FFlowDataPinValue_Enum& EnumValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full enum value array (as uint8) from an output Enum DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Enum Values"))
	static TArray<uint8> GetEnumValues(UPARAM(Ref) const FFlowDataPinValue_Enum& EnumValue);

	// Set a single FVector on a Vector DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Vector Value"))
	static void SetVectorValue(const FVector& InValue, UPARAM(Ref) FFlowDataPinValue_Vector& VectorValue) { VectorValue.Values = { InValue }; }

	// Set an FVector array on a Vector DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Vector Values"))
	static void SetVectorValues(const TArray<FVector>& InValues, UPARAM(Ref) FFlowDataPinValue_Vector& VectorValue) { VectorValue.Values = InValues; }

	// Get a single FVector from an output Vector DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Vector Value"))
	static FVector GetVectorValue(UPARAM(Ref) const FFlowDataPinValue_Vector& VectorValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full FVector array from an output Vector DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Vector Values"))
	static TArray<FVector> GetVectorValues(UPARAM(Ref) FFlowDataPinValue_Vector& VectorValue);

	// Set a single FRotator on a Rotator DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Rotator Value"))
	static void SetRotatorValue(const FRotator& InValue, UPARAM(Ref) FFlowDataPinValue_Rotator& RotatorValue) { RotatorValue.Values = { InValue }; }

	// Set an FRotator array on a Rotator DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Rotator Values"))
	static void SetRotatorValues(const TArray<FRotator>& InValues, UPARAM(Ref) FFlowDataPinValue_Rotator& RotatorValue) { RotatorValue.Values = InValues; }

	// Get a single FRotator from an output Rotator DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Rotator Value"))
	static FRotator GetRotatorValue(UPARAM(Ref) const FFlowDataPinValue_Rotator& RotatorValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full FRotator array from an output Rotator DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Rotator Values"))
	static TArray<FRotator> GetRotatorValues(UPARAM(Ref) FFlowDataPinValue_Rotator& RotatorValue);

	// Set a single FTransform on a Transform DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Transform Value"))
	static void SetTransformValue(const FTransform& InValue, UPARAM(Ref) FFlowDataPinValue_Transform& TransformValue) { TransformValue.Values = { InValue }; }

	// Set an FTransform array on a Transform DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Transform Values"))
	static void SetTransformValues(const TArray<FTransform>& InValues, UPARAM(Ref) FFlowDataPinValue_Transform& TransformValue) { TransformValue.Values = InValues; }

	// Get a single FTransform from an output Transform DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Transform Value"))
	static FTransform GetTransformValue(UPARAM(Ref) const FFlowDataPinValue_Transform& TransformValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full FTransform array from an output Transform DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Transform Values"))
	static TArray<FTransform> GetTransformValues(UPARAM(Ref) FFlowDataPinValue_Transform& TransformValue);

	// Set a single FGameplayTag on a GameplayTag DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set GameplayTag Value"))
	static void SetGameplayTagValue(FGameplayTag InValue, UPARAM(Ref) FFlowDataPinValue_GameplayTag& GameplayTagValue) { GameplayTagValue.Values = { InValue }; }

	// Set an FGameplayTag array on a GameplayTag DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set GameplayTag Values"))
	static void SetGameplayTagValues(const TArray<FGameplayTag>& InValues, UPARAM(Ref) FFlowDataPinValue_GameplayTag& GameplayTagValue) { GameplayTagValue.Values = InValues; }

	// Get a single FGameplayTag from an output GameplayTag DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get GameplayTag Value"))
	static FGameplayTag GetGameplayTagValue(UPARAM(Ref) const FFlowDataPinValue_GameplayTag& GameplayTagValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full FGameplayTag array from an output GameplayTag DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get GameplayTag Values"))
	static TArray<FGameplayTag> GetGameplayTagValues(UPARAM(Ref) FFlowDataPinValue_GameplayTag& GameplayTagValue);

	// Set a GameplayTagContainer on a GameplayTagContainer DataPin Value (input or output pin, scalar only).
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set GameplayTagContainer Value"))
	static void SetGameplayTagContainerValue(const FGameplayTagContainer& InValue, UPARAM(Ref) FFlowDataPinValue_GameplayTagContainer& GameplayTagContainerValue) { GameplayTagContainerValue.Values = InValue; }

	// Get the GameplayTagContainer from an output GameplayTagContainer DataPin Value (scalar only). DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get GameplayTagContainer Value"))
	static FGameplayTagContainer GetGameplayTagContainerValue(UPARAM(Ref) const FFlowDataPinValue_GameplayTagContainer& GameplayTagContainerValue);

	// Set a single FInstancedStruct on an InstancedStruct DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set InstancedStruct Value"))
	static void SetInstancedStructValue(const FInstancedStruct& InValue, UPARAM(Ref) FFlowDataPinValue_InstancedStruct& InstancedStructValue) { InstancedStructValue.Values = { InValue }; }

	// Set an FInstancedStruct array on an InstancedStruct DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set InstancedStruct Values"))
	static void SetInstancedStructValues(const TArray<FInstancedStruct>& InValues, UPARAM(Ref) FFlowDataPinValue_InstancedStruct& InstancedStructValue) { InstancedStructValue.Values = InValues; }

	// Get a single FInstancedStruct from an output InstancedStruct DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get InstancedStruct Value"))
	static FInstancedStruct GetInstancedStructValue(UPARAM(Ref) const FFlowDataPinValue_InstancedStruct& InstancedStructValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full FInstancedStruct array from an output InstancedStruct DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get InstancedStruct Values"))
	static TArray<FInstancedStruct> GetInstancedStructValues(UPARAM(Ref) FFlowDataPinValue_InstancedStruct& InstancedStructValue);

	// Set a single UObject* on an Object DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Object Value"))
	static void SetObjectValue(UObject* InValue, UPARAM(Ref) FFlowDataPinValue_Object& ObjectValue) { ObjectValue.Values = { InValue }; }

	// Set a UObject* array on an Object DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Object Values"))
	static void SetObjectValues(const TArray<UObject*>& InValues, UPARAM(Ref) FFlowDataPinValue_Object& ObjectValue) { ObjectValue.Values = InValues; }

	// Get a single UObject* from an output Object DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Object Value"))
	static UObject* GetObjectValue(UPARAM(Ref) const FFlowDataPinValue_Object& ObjectValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full UObject* array from an output Object DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Object Values"))
	static TArray<UObject*> GetObjectValues(UPARAM(Ref) FFlowDataPinValue_Object& ObjectValue);

	// Set a single FSoftClassPath on a Class DataPin Value (input or output pin). Replaces any existing values.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Class Value"))
	static void SetClassValue(const FSoftClassPath& InValue, UPARAM(Ref) FFlowDataPinValue_Class& ClassValue) { ClassValue.Values = { InValue }; }

	// Set an FSoftClassPath array on a Class DataPin Value (input or output pin). Replaces the entire array.
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Set Class Values"))
	static void SetClassValues(const TArray<FSoftClassPath>& InValues, UPARAM(Ref) FFlowDataPinValue_Class& ClassValue) { ClassValue.Values = InValues; }

	// Get a single FSoftClassPath from an output Class DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Class Value"))
	static FSoftClassPath GetClassValue(UPARAM(Ref) const FFlowDataPinValue_Class& ClassValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue);

	// Get the full FSoftClassPath array from an output Class DataPin Value. DO NOT use on input pins — will error in editor, use Resolve As... functions instead!
	UFUNCTION(BlueprintCallable, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Get Class Values"))
	static TArray<FSoftClassPath> GetClassValues(UPARAM(Ref) FFlowDataPinValue_Class& ClassValue);

	// --------- Make Flow DataPin Result - for use in blueprint TrySupplyDataPin implementations ----------
	
	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Empty(EFlowDataPinResolveResult Result) { return FFlowDataPinResult(Result); }

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Bool Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Bool(const TArray<bool>& BoolValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Int Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Int(const TArray<int32>& IntValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Int64 Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Int64(const TArray<int64>& Int64Values);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Float Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Float(const TArray<float>& FloatValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Double Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Double(const TArray<double>& DoubleValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Name Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Name(const TArray<FName>& NameValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make String Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_String(const TArray<FString>& StringValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Text Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Text(const TArray<FText>& TextValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Enum Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Enum(const TArray<uint8>& EnumValues, UEnum* EnumClass);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Vector Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Vector(const TArray<FVector>& VectorValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Rotator Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Rotator(const TArray<FRotator>& RotatorValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Transform Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Transform(const TArray<FTransform>& TransformValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make GameplayTag Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_GameplayTag(const TArray<FGameplayTag>& GameplayTagValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make GameplayTagContainer Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_GameplayTagContainer(FGameplayTagContainer GameplayTagContainerValue);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make InstancedStruct Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_InstancedStruct(const TArray<FInstancedStruct>& InstancedStructValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Object Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Object(const TArray<UObject*>& ObjectValues);

	UFUNCTION(BlueprintPure, Category = DataPins, meta = (DisplayName = "Make Class Flow DataPin Result"))
	static FFlowDataPinResult MakeFlowDataPinResult_Class(const TArray<FSoftClassPath>& ClassValues);

	// ---------- Override the Make functions to discourage use ----------
	// Ideally, we would forbid Make altogether, but this is the best work-around I have found.

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Bool Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetBoolValue(s) instead"))
	static FFlowDataPinValue_Bool MakeStructBool(const FFlowDataPinValue_Bool& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Int Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetIntValue(s) instead"))
	static FFlowDataPinValue_Int MakeStructInt(const FFlowDataPinValue_Int& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Int64 Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetInt64Value(s) instead"))
	static FFlowDataPinValue_Int64 MakeStructInt64(const FFlowDataPinValue_Int64& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Float Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetFloatValue(s) instead"))
	static FFlowDataPinValue_Float MakeStructFloat(const FFlowDataPinValue_Float& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Double Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetDoubleValue(s) instead"))
	static FFlowDataPinValue_Double MakeStructDouble(const FFlowDataPinValue_Double& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Name Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetNameValue(s) instead"))
	static FFlowDataPinValue_Name MakeStructName(const FFlowDataPinValue_Name& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make String Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetStringValue(s) instead"))
	static FFlowDataPinValue_String MakeStructString(const FFlowDataPinValue_String& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Text Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetTextValue(s) instead"))
	static FFlowDataPinValue_Text MakeStructText(const FFlowDataPinValue_Text& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Enum Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetEnumValue(s) instead"))
	static FFlowDataPinValue_Enum MakeStructEnum(const FFlowDataPinValue_Enum& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Vector Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetVectorValue(s) instead"))
	static FFlowDataPinValue_Vector MakeStructVector(const FFlowDataPinValue_Vector& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Rotator Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetRotatorValue(s) instead"))
	static FFlowDataPinValue_Rotator MakeStructRotator(const FFlowDataPinValue_Rotator& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Transform Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetTransformValue(s) instead"))
	static FFlowDataPinValue_Transform MakeStructTransform(const FFlowDataPinValue_Transform& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make GameplayTag Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetGameplayTagValue(s) instead"))
	static FFlowDataPinValue_GameplayTag MakeStructGameplayTag(const FFlowDataPinValue_GameplayTag& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make GameplayTagContainer Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetGameplayTagContainerValue(s) instead"))
	static FFlowDataPinValue_GameplayTagContainer MakeStructGameplayTagContainer(const FFlowDataPinValue_GameplayTagContainer& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make InstancedStruct Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetInstancedStructValue(s) instead"))
	static FFlowDataPinValue_InstancedStruct MakeStructInstancedStruct(const FFlowDataPinValue_InstancedStruct& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Object Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetObjectValue(s) instead"))
	static FFlowDataPinValue_Object MakeStructObject(const FFlowDataPinValue_Object& OtherValueStruct) { return OtherValueStruct; }

	UFUNCTION(BlueprintPure, Category = DataPins, Meta = (BlueprintThreadSafe, DisplayName = "Make Class Flow DataPin Value", DeprecatedFunction, DeprecationMessage = "use SetClassValue(s) instead"))
	static FFlowDataPinValue_Class MakeStructClass(const FFlowDataPinValue_Class& OtherValueStruct) { return OtherValueStruct; }
};
