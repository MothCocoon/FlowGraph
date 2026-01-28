// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "Types/FlowPinEnums.h"
#include "FlowDataPinResults.generated.h"

struct FInstancedStruct;
struct FFlowDataPinValue;

// #FlowDataPinLegacy
struct FFlowDataPinOutputProperty_Object;
struct FFlowDataPinOutputProperty_Class;
// --

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result")
struct FFlowDataPinResult
{
	GENERATED_BODY()

public:

	// Result for the DataPin resolve attempt
	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	EFlowDataPinResolveResult Result = EFlowDataPinResolveResult::FailedUnimplemented;

public:
	FLOW_API explicit FFlowDataPinResult() = default;
	FLOW_API explicit FFlowDataPinResult(EFlowDataPinResolveResult InResult) : Result(InResult) { }

	template <typename TFlowDataPinValueSubclass>
	explicit FFlowDataPinResult(const TFlowDataPinValueSubclass& InValue) : Result(EFlowDataPinResolveResult::Success), ResultValue(TInstancedStruct<FFlowDataPinValue>::Make(InValue)) {}

public:
	UPROPERTY()
	TInstancedStruct<FFlowDataPinValue> ResultValue;
};

// #FlowDataPinLegacy

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Bool)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Bool : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	bool Value = false;

public:

	FLOW_API FFlowDataPinResult_Bool() { }
	FLOW_API FFlowDataPinResult_Bool(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Bool(bool InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Int)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Int : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	int64 Value = 0;

public:

	FLOW_API FFlowDataPinResult_Int() { }
	FLOW_API FFlowDataPinResult_Int(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Int(int64 InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Float)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Float : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	double Value = 0;

public:

	FLOW_API FFlowDataPinResult_Float() { }
	FLOW_API FFlowDataPinResult_Float(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Float(double InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
	{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Name)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Name : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FName Value = NAME_None;

public:

	FLOW_API FFlowDataPinResult_Name() { }
	FLOW_API FFlowDataPinResult_Name(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Name(const FName& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }

	FLOW_API void SetValue(const FName& FromName) { Value = FromName; }
	FLOW_API void SetValue(const FString& FromString) { Value = FName(FromString); }
	FLOW_API void SetValue(const FText& FromText) { Value = FName(FromText.ToString()); }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (String)", meta = (DeprecatedClass))
struct FFlowDataPinResult_String : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FString Value;

public:

	FLOW_API FFlowDataPinResult_String() { }
	FLOW_API FFlowDataPinResult_String(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_String(const FString& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }

	FLOW_API void SetValue(const FName& FromName) { Value = FromName.ToString(); }
	FLOW_API void SetValue(const FString& FromString) { Value = FromString; }
	FLOW_API void SetValue(const FText& FromText) { Value = FromText.ToString(); }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Text)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Text : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FText Value;

public:

	FLOW_API FFlowDataPinResult_Text() { }
	FLOW_API FFlowDataPinResult_Text(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Text(const FText& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }

	FLOW_API void SetValue(const FName& FromName) { Value = FText::FromName(FromName); }
	FLOW_API void SetValue(const FString& FromString) { Value = FText::FromString(FromString); }
	FLOW_API void SetValue(const FText& FromText) { Value = FromText; }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Enum)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Enum : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	// The selected enum Value
	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FName Value = NAME_None;

	// Class for this enum
	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	TObjectPtr<UEnum> EnumClass = nullptr;

public:

	FLOW_API FFlowDataPinResult_Enum() { }
	FLOW_API FFlowDataPinResult_Enum(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Enum(const FName& InValue, UEnum* InEnumClass)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		, EnumClass(InEnumClass)
		{ }
	FLOW_API explicit FFlowDataPinResult_Enum(uint8 InEnumAsIntValue, UEnum& InEnumClass)
		: Super(EFlowDataPinResolveResult::Success)
		, Value()
		, EnumClass(&InEnumClass)
		{
			const int32 EnumValueAsIndex = EnumClass->GetIndexByValue(InEnumAsIntValue);
			const FText DisplayValueText = EnumClass->GetDisplayNameTextByIndex(EnumValueAsIndex);
			const FName EnumValue = FName(DisplayValueText.ToString());

			Value = EnumValue;
			Result = EFlowDataPinResolveResult::Success;
		}

	template <typename TUnrealNativeEnumType>
	static FFlowDataPinResult_Enum BuildResultFromNativeEnumValue(TUnrealNativeEnumType EnumValue)
	{
		FFlowDataPinResult_Enum Result;
		Result.SetFromNativeEnumValue(EnumValue);

		return Result;
	}

	template <typename TUnrealNativeEnumType>
	void SetFromNativeEnumValue(TUnrealNativeEnumType InEnumValue)
	{
		EnumClass = StaticEnum<TUnrealNativeEnumType>();
		const FText DisplayValueText = EnumClass->GetDisplayValueAsText(InEnumValue);
		const FName EnumValue = FName(DisplayValueText.ToString());

		Value = EnumValue;
		Result = EFlowDataPinResolveResult::Success;
	}

	template <typename TUnrealNativeEnumType, TUnrealNativeEnumType InvalidValue>
	TUnrealNativeEnumType GetNativeEnumValue(EGetByNameFlags GetByNameFlags = EGetByNameFlags::None) const
	{
		if (!IsValid(EnumClass))
		{
			return InvalidValue;
		}

		int64 ValueAsInt = EnumClass->GetValueByName(Value, GetByNameFlags);
		if (ValueAsInt == INDEX_NONE)
		{
			return InvalidValue;
		}

		return static_cast<TUnrealNativeEnumType>(ValueAsInt);
	}
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Vector)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Vector : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FVector Value = FVector::ZeroVector;

public:

	FLOW_API FFlowDataPinResult_Vector() { }
	FLOW_API FFlowDataPinResult_Vector(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Vector(const FVector& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Rotator)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Rotator : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FRotator Value = FRotator::ZeroRotator;

public:

	FLOW_API FFlowDataPinResult_Rotator() { }
	FLOW_API FFlowDataPinResult_Rotator(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Rotator(const FRotator& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
	{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Transform)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Transform : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FTransform Value;

public:

	FLOW_API FFlowDataPinResult_Transform() { }
	FLOW_API FFlowDataPinResult_Transform(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Transform(const FTransform& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
	{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (GameplayTag)", meta = (DeprecatedClass))
struct FFlowDataPinResult_GameplayTag : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FGameplayTag Value;

public:

	FLOW_API FFlowDataPinResult_GameplayTag() { }
	FLOW_API FFlowDataPinResult_GameplayTag(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_GameplayTag(const FGameplayTag& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (GameplayTagContainer)", meta = (DeprecatedClass))
struct FFlowDataPinResult_GameplayTagContainer : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FGameplayTagContainer Value;

public:

	FLOW_API FFlowDataPinResult_GameplayTagContainer() { }
	FLOW_API FFlowDataPinResult_GameplayTagContainer(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_GameplayTagContainer(const FGameplayTagContainer& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (InstancedStruct)", meta = (DeprecatedClass))
struct FFlowDataPinResult_InstancedStruct : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FInstancedStruct Value;

public:

	FLOW_API FFlowDataPinResult_InstancedStruct() { }
	FLOW_API FFlowDataPinResult_InstancedStruct(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_InstancedStruct(const FInstancedStruct& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
	{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Object)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Object : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	TObjectPtr<UObject> Value;

public:

	FLOW_API FFlowDataPinResult_Object() { }
	FLOW_API FFlowDataPinResult_Object(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Object(UObject* InValue);

	FLOW_API FORCEINLINE void SetValueFromSoftPath(const FSoftObjectPath& SoftPath) { Value = SoftPath.ResolveObject(); }
	FLOW_API FORCEINLINE void SetValueFromObjectPtr(UObject* ObjectPtr) { Value = ObjectPtr; }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Class)", meta = (DeprecatedClass))
struct FFlowDataPinResult_Class : public FFlowDataPinResult
{
	GENERATED_BODY()

protected:

	// SoftClassPath version of the result
	// (both the SoftClassPath and the UClass (if available) will be set for the result)
	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	FSoftClassPath ValuePath;

	// UClass version of the result
	// (both the SoftClassPath and the UClass (if available) will be set for the result)
	UPROPERTY(BlueprintReadWrite, Category = DataPins)
	TObjectPtr<UClass> ValueClass = nullptr;

public:

	FLOW_API FFlowDataPinResult_Class() { }
	FLOW_API FFlowDataPinResult_Class(EFlowDataPinResolveResult InResult) : Super(InResult) { }
	FLOW_API FFlowDataPinResult_Class(const FSoftClassPath& InValuePath);
	FLOW_API FFlowDataPinResult_Class(UClass* InValueClass);

	FLOW_API void SetValueSoftClassAndClassPtr(const FSoftClassPath& SoftPath, UClass* ObjectPtr);
	FLOW_API void SetValueFromSoftPath(const FSoftObjectPath& SoftObjectPath);
	FLOW_API FORCEINLINE void SetValueFromObjectPtr(UClass* ClassPtr) { SetValueSoftClassAndClassPtr(FSoftClassPath(ClassPtr), ClassPtr); }

	FLOW_API UClass* GetOrResolveClass() const { return IsValid(ValueClass) ? ValueClass.Get() : ValuePath.ResolveClass(); }
	FLOW_API FSoftClassPath GetAsSoftClass() const;
};
// --