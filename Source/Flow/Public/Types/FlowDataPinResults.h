// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowPinEnums.h"

#include "GameplayTagContainer.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif

#include "FlowDataPinResults.generated.h"

struct FInstancedStruct;
struct FFlowDataPinOutputProperty_Object;
struct FFlowDataPinOutputProperty_Class;

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result")
struct FFlowDataPinResult
{
	GENERATED_BODY()

public:

	// Result for the DataPin resolve attempt
	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	EFlowDataPinResolveResult Result = EFlowDataPinResolveResult::FailedUnimplemented;

public:
	FLOW_API FFlowDataPinResult() { }
	FLOW_API explicit FFlowDataPinResult(EFlowDataPinResolveResult InResult) : Result(InResult) { }
};

// Recommend implementing FFlowDataPinResult... for every EFlowPinType
FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Bool)")
struct FFlowDataPinResult_Bool : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	bool Value = false;

public:

	FLOW_API FFlowDataPinResult_Bool() { }
	FLOW_API FFlowDataPinResult_Bool(bool InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Int)")
struct FFlowDataPinResult_Int : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	int64 Value = 0;

public:

	FLOW_API FFlowDataPinResult_Int() { }
	FLOW_API FFlowDataPinResult_Int(int64 InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Float)")
struct FFlowDataPinResult_Float : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	double Value = 0;

public:

	FLOW_API FFlowDataPinResult_Float() { }
	FLOW_API FFlowDataPinResult_Float(double InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
	{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Name)")
struct FFlowDataPinResult_Name : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FName Value = NAME_None;

public:

	FLOW_API FFlowDataPinResult_Name() { }
	FLOW_API FFlowDataPinResult_Name(const FName& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }

	FLOW_API void SetValue(const FName& FromName) { Value = FromName; }
	FLOW_API void SetValue(const FString& FromString) { Value = FName(FromString); }
	FLOW_API void SetValue(const FText& FromText) { Value = FName(FromText.ToString()); }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (String)")
struct FFlowDataPinResult_String : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FString Value;

public:

	FLOW_API FFlowDataPinResult_String() { }
	FLOW_API FFlowDataPinResult_String(const FString& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }

	FLOW_API void SetValue(const FName& FromName) { Value = FromName.ToString(); }
	FLOW_API void SetValue(const FString& FromString) { Value = FromString; }
	FLOW_API void SetValue(const FText& FromText) { Value = FromText.ToString(); }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Text)")
struct FFlowDataPinResult_Text : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FText Value;

public:

	FLOW_API FFlowDataPinResult_Text() { }
	FLOW_API FFlowDataPinResult_Text(const FText& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }

	FLOW_API void SetValue(const FName& FromName) { Value = FText::FromName(FromName); }
	FLOW_API void SetValue(const FString& FromString) { Value = FText::FromString(FromString); }
	FLOW_API void SetValue(const FText& FromText) { Value = FromText; }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Enum)")
struct FFlowDataPinResult_Enum : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	// The selected enum Value
	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FName Value = NAME_None;

	// Class for this enum
	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	TObjectPtr<UEnum> EnumClass = nullptr;

public:

	FLOW_API FFlowDataPinResult_Enum() { }
	FLOW_API FFlowDataPinResult_Enum(const FName& InValue, UEnum* InEnumClass)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		, EnumClass(InEnumClass)
		{ }
	FLOW_API explicit FFlowDataPinResult_Enum(EFlowDataPinResolveResult InResult) : Super(InResult) { }
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

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Vector)")
struct FFlowDataPinResult_Vector : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FVector Value = FVector::ZeroVector;

public:

	FLOW_API FFlowDataPinResult_Vector() { }
	FLOW_API FFlowDataPinResult_Vector(const FVector& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Rotator)")
struct FFlowDataPinResult_Rotator : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FRotator Value = FRotator::ZeroRotator;

public:

	FLOW_API FFlowDataPinResult_Rotator() { }
	FLOW_API FFlowDataPinResult_Rotator(const FRotator& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
	{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Transform)")
struct FFlowDataPinResult_Transform : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FTransform Value;

public:

	FLOW_API FFlowDataPinResult_Transform() { }
	FLOW_API FFlowDataPinResult_Transform(const FTransform& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
	{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (GameplayTag)")
struct FFlowDataPinResult_GameplayTag : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FGameplayTag Value;

public:

	FLOW_API FFlowDataPinResult_GameplayTag() { }
	FLOW_API FFlowDataPinResult_GameplayTag(const FGameplayTag& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (GameplayTagContainer)")
struct FFlowDataPinResult_GameplayTagContainer : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FGameplayTagContainer Value;

public:

	FLOW_API FFlowDataPinResult_GameplayTagContainer() { }
	FLOW_API FFlowDataPinResult_GameplayTagContainer(const FGameplayTagContainer& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
		{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (InstancedStruct)")
struct FFlowDataPinResult_InstancedStruct : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FInstancedStruct Value;

public:

	FLOW_API FFlowDataPinResult_InstancedStruct() { }
	FLOW_API FFlowDataPinResult_InstancedStruct(const FInstancedStruct& InValue)
		: Super(EFlowDataPinResolveResult::Success)
		, Value(InValue)
	{ }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Object)")
struct FFlowDataPinResult_Object : public FFlowDataPinResult
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	TObjectPtr<UObject> Value;

public:

	FLOW_API FFlowDataPinResult_Object() { }
	FLOW_API FFlowDataPinResult_Object(UObject* InValue);

	FLOW_API void SetValueFromPropertyWrapper(const FFlowDataPinOutputProperty_Object& InPropertyWrapper);
	FLOW_API FORCEINLINE void SetValueFromSoftPath(const FSoftObjectPath& SoftPath) { Value = SoftPath.ResolveObject(); }
	FLOW_API FORCEINLINE void SetValueFromObjectPtr(UObject* ObjectPtr) { Value = ObjectPtr; }
};

USTRUCT(BlueprintType, DisplayName = "Flow DataPin Result (Class)")
struct FFlowDataPinResult_Class : public FFlowDataPinResult
{
	GENERATED_BODY()

protected:

	// SoftClassPath version of the result
	// (both the SoftClassPath and the UClass (if available) will be set for the result)
	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	FSoftClassPath ValuePath;

	// UClass version of the result
	// (both the SoftClassPath and the UClass (if available) will be set for the result)
	UPROPERTY(Transient, BlueprintReadWrite, Category = DataPins)
	TObjectPtr<UClass> ValueClass = nullptr;

public:

	FLOW_API FFlowDataPinResult_Class() { }
	FLOW_API FFlowDataPinResult_Class(const FSoftClassPath& InValuePath);
	FLOW_API FFlowDataPinResult_Class(UClass* InValueClass);

	FLOW_API void SetValueFromPropertyWrapper(const FFlowDataPinOutputProperty_Class& PropertyWrapper);
	FLOW_API void SetValueSoftClassAndClassPtr(const FSoftClassPath& SoftPath, UClass* ObjectPtr);
	FLOW_API void SetValueFromSoftPath(const FSoftObjectPath& SoftObjectPath);
	FLOW_API FORCEINLINE void SetValueFromObjectPtr(UClass* ClassPtr) { SetValueSoftClassAndClassPtr(FSoftClassPath(ClassPtr), ClassPtr); }

	FLOW_API UClass* GetOrResolveClass() const { return IsValid(ValueClass) ? ValueClass.Get() : ValuePath.ResolveClass(); }
	FLOW_API FSoftClassPath GetAsSoftClass() const;
};
