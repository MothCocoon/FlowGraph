// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowDataPinValue.h"
#include "Types/FlowPinTypesStandard.h"

#include "StructUtils/InstancedStruct.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Class.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"

#include "FlowDataPinValuesStandard.generated.h"

//======================================================================
// Bool
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Bool - Flow DataPin Value", meta = (FlowPinType = "Bool", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructBool"))
struct FFlowDataPinValue_Bool : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Bool;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<bool> Values{ false };

	FLOW_API FFlowDataPinValue_Bool() = default;
	FLOW_API FFlowDataPinValue_Bool(ValueType InValue);
	FLOW_API FFlowDataPinValue_Bool(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// Int (int32)
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Int - Flow DataPin Value", meta = (FlowPinType = "Int", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructInt"))
struct FFlowDataPinValue_Int : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Int;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<int32> Values{ 0 };

	FLOW_API FFlowDataPinValue_Int() = default;
	FLOW_API FFlowDataPinValue_Int(ValueType InValue);
	FLOW_API FFlowDataPinValue_Int(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// Int64
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Int64 - Flow DataPin Value", meta = (FlowPinType = "Int64", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructInt64"))
struct FFlowDataPinValue_Int64 : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Int64;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<int64> Values{ 0 };

	FLOW_API FFlowDataPinValue_Int64() = default;
	FLOW_API FFlowDataPinValue_Int64(ValueType InValue);
	FLOW_API FFlowDataPinValue_Int64(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// Float
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Float - Flow DataPin Value", meta = (FlowPinType = "Float", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructFloat"))
struct FFlowDataPinValue_Float : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Float;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<float> Values{ 0.0f };

	FLOW_API FFlowDataPinValue_Float() = default;
	FLOW_API FFlowDataPinValue_Float(ValueType InValue);
	FLOW_API FFlowDataPinValue_Float(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// Double
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Double - Flow DataPin Value", meta = (FlowPinType = "Double", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructDouble"))
struct FFlowDataPinValue_Double : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Double;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<double> Values{ 0.0 };

	FLOW_API FFlowDataPinValue_Double() = default;
	FLOW_API FFlowDataPinValue_Double(ValueType InValue);
	FLOW_API FFlowDataPinValue_Double(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// Name
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Name - Flow DataPin Value", meta = (FlowPinType = "Name", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructName"))
struct FFlowDataPinValue_Name : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Name;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FName> Values{ NAME_None };

	FLOW_API FFlowDataPinValue_Name() = default;
	FLOW_API FFlowDataPinValue_Name(const ValueType& InValue);
	FLOW_API FFlowDataPinValue_Name(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// String
//======================================================================
USTRUCT(BlueprintType, DisplayName = "String - Flow DataPin Value", meta = (FlowPinType = "String", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructString"))
struct FFlowDataPinValue_String : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_String;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FString> Values;

	FLOW_API FFlowDataPinValue_String() = default;
	FLOW_API FFlowDataPinValue_String(const ValueType& InValue);
	FLOW_API FFlowDataPinValue_String(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// Text
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Text - Flow DataPin Value", meta = (FlowPinType = "Text", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructText"))
struct FFlowDataPinValue_Text : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Text;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FText> Values;

	FLOW_API FFlowDataPinValue_Text() = default;
	FLOW_API FFlowDataPinValue_Text(const ValueType& InValue);
	FLOW_API FFlowDataPinValue_Text(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// Enum
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Enum - Flow DataPin Value", meta = (FlowPinType = "Enum", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructEnum"))
struct FFlowDataPinValue_Enum : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Enum;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FName> Values;

	// Enum asset reference (advanced)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins, meta = (NoClear, AdvancedDisplay))
	TSoftObjectPtr<UEnum> EnumClass;

#if WITH_EDITORONLY_DATA
	// Native C++ enum name (advanced)
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AdvancedDisplay))
	FString EnumName;
#endif

	FLOW_API FFlowDataPinValue_Enum() = default;
	FLOW_API FFlowDataPinValue_Enum(const TSoftObjectPtr<UEnum>& InEnumClass, const ValueType& InValue);
	FLOW_API FFlowDataPinValue_Enum(const TSoftObjectPtr<UEnum>& InEnumClass, const TArray<ValueType>& InValues);

#if WITH_EDITOR
	FLOW_API void OnEnumNameChanged();
#endif

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual UField* GetFieldType() const override;
	virtual bool TryConvertValuesToString(FString& OutString) const override;

	// Helper templates
	template <typename TUnrealNativeEnumType>
	static bool TryGetEnumValueByName(const UEnum* EnumClass, const FName& EnumValueName, TUnrealNativeEnumType& OutValue, EGetByNameFlags GetByNameFlags = EGetByNameFlags::ErrorIfNotFound)
	{
		if (!IsValid(EnumClass))
		{
			return false;
		}

		const int32 EnumIndex = EnumClass->GetIndexByName(EnumValueName, GetByNameFlags);
		if (EnumIndex != INDEX_NONE)
		{
			OutValue = static_cast<TUnrealNativeEnumType>(EnumClass->GetValueByIndex(EnumIndex));
			return true;
		}
		return false;
	}

	template <typename TUnrealNativeEnumType>
	EFlowDataPinResolveResult TryGetSingleEnumValue(TUnrealNativeEnumType& OutEnumValue, EFlowSingleFromArray SingleFromArray, EGetByNameFlags GetByNameFlags = EGetByNameFlags::ErrorIfNotFound) const
	{
		const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, Values.Num());
		if (!Values.IsValidIndex(Index))
		{
			return EFlowDataPinResolveResult::FailedInsufficientValues;
		}

		UEnum* EnumClassPtr = EnumClass.LoadSynchronous();
		if (!TryGetEnumValueByName(EnumClassPtr, Values[Index], OutEnumValue, GetByNameFlags))
		{
			return EFlowDataPinResolveResult::FailedUnknownEnumValue;
		}
		return EFlowDataPinResolveResult::Success;
	}

	template <typename TUnrealNativeEnumType>
	EFlowDataPinResolveResult TryGetAllNativeEnumValues(TArray<TUnrealNativeEnumType>& OutEnumValues, EGetByNameFlags GetByNameFlags = EGetByNameFlags::ErrorIfNotFound) const
	{
		if (Values.IsEmpty())
		{
			return EFlowDataPinResolveResult::FailedInsufficientValues;
		}

		UEnum* EnumClassPtr = EnumClass.LoadSynchronous();
		OutEnumValues.Reserve(Values.Num());

		for (const ValueType& ValueName : Values)
		{
			TUnrealNativeEnumType EnumValue;
			if (!TryGetEnumValueByName(EnumClassPtr, ValueName, EnumValue, GetByNameFlags))
			{
				return EFlowDataPinResolveResult::FailedUnknownEnumValue;
			}
			OutEnumValues.Add(EnumValue);
		}
		return EFlowDataPinResolveResult::Success;
	}
};

//======================================================================
// Vector
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Vector - Flow DataPin Value", meta = (FlowPinType = "Vector", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructVector"))
struct FFlowDataPinValue_Vector : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Vector;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FVector> Values{ FVector::ZeroVector };

	FLOW_API FFlowDataPinValue_Vector() = default;
	FLOW_API FFlowDataPinValue_Vector(const ValueType& InValue);
	FLOW_API FFlowDataPinValue_Vector(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// Rotator
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Rotator - Flow DataPin Value", meta = (FlowPinType = "Rotator", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructRotator"))
struct FFlowDataPinValue_Rotator : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Rotator;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FRotator> Values{ FRotator::ZeroRotator };

	FLOW_API FFlowDataPinValue_Rotator() = default;
	FLOW_API FFlowDataPinValue_Rotator(const ValueType& InValue);
	FLOW_API FFlowDataPinValue_Rotator(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// Transform
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Transform - Flow DataPin Value", meta = (FlowPinType = "Transform", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructTransform"))
struct FFlowDataPinValue_Transform : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Transform;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FTransform> Values{ FTransform::Identity };

	FLOW_API FFlowDataPinValue_Transform() = default;
	FLOW_API FFlowDataPinValue_Transform(const ValueType& InValue);
	FLOW_API FFlowDataPinValue_Transform(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// GameplayTag
//======================================================================
USTRUCT(BlueprintType, DisplayName = "GameplayTag - Flow DataPin Value", meta = (FlowPinType = "GameplayTag", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructGameplayTag"))
struct FFlowDataPinValue_GameplayTag : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_GameplayTag;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FGameplayTag> Values;

	FLOW_API FFlowDataPinValue_GameplayTag() = default;
	FLOW_API FFlowDataPinValue_GameplayTag(const ValueType& InValue);
	FLOW_API FFlowDataPinValue_GameplayTag(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// GameplayTagContainer
//======================================================================
USTRUCT(BlueprintType, DisplayName = "GameplayTagContainer - Flow DataPin Value", meta = (FlowPinType = "GameplayTagContainer", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructGameplayTagContainer"))
struct FFlowDataPinValue_GameplayTagContainer : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_GameplayTagContainer;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FGameplayTagContainer Values;

	FLOW_API FFlowDataPinValue_GameplayTagContainer() = default;
	FLOW_API FFlowDataPinValue_GameplayTagContainer(const FGameplayTag& InValue);
	FLOW_API FFlowDataPinValue_GameplayTagContainer(const FGameplayTagContainer& InValues);
	FLOW_API FFlowDataPinValue_GameplayTagContainer(const TArray<FGameplayTag>& InValues);
	FLOW_API FFlowDataPinValue_GameplayTagContainer(const TArray<FGameplayTagContainer>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// InstancedStruct
//======================================================================
USTRUCT(BlueprintType, DisplayName = "InstancedStruct - Flow DataPin Value", meta = (FlowPinType = "InstancedStruct", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructInstancedStruct"))
struct FFlowDataPinValue_InstancedStruct : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_InstancedStruct;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FInstancedStruct> Values;

	FLOW_API FFlowDataPinValue_InstancedStruct() = default;
	FLOW_API FFlowDataPinValue_InstancedStruct(const ValueType& InValue);
	FLOW_API FFlowDataPinValue_InstancedStruct(const TArray<ValueType>& InValues);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
};

//======================================================================
// Object
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Object - Flow DataPin Value", meta = (FlowPinType = "Object", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructObject"))
struct FFlowDataPinValue_Object : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Object;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<TObjectPtr<UObject>> Values;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AllowAbstract, AdvancedDisplay))
	TObjectPtr<UClass> ClassFilter = UObject::StaticClass();
#endif

	FLOW_API FFlowDataPinValue_Object() = default;
	FLOW_API FFlowDataPinValue_Object(TObjectPtr<UObject> InObject, UClass* InClassFilter = UObject::StaticClass());
	FLOW_API FFlowDataPinValue_Object(const TArray<TObjectPtr<UObject>>& InObjects, UClass* InClassFilter = UObject::StaticClass());
	FLOW_API FFlowDataPinValue_Object(AActor* InActor, UClass* InClassFilter = nullptr /* nullptr here defaults to AActor::StaticClass() */ );
	FLOW_API FFlowDataPinValue_Object(const TArray<AActor*>& InActors, UClass* InClassFilter = nullptr /* nullptr here defaults to AActor::StaticClass() */);

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};

//======================================================================
// Class
//======================================================================
USTRUCT(BlueprintType, DisplayName = "Class - Flow DataPin Value", meta = (FlowPinType = "Class", HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStructClass"))
struct FFlowDataPinValue_Class : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	using PinType = FFlowPinType_Class;
	using ValueType = PinType::ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FSoftClassPath> Values;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AllowAbstract, AdvancedDisplay))
	TObjectPtr<UClass> ClassFilter = UObject::StaticClass();
#endif

	FLOW_API FFlowDataPinValue_Class() = default;
	FLOW_API FFlowDataPinValue_Class(const FSoftClassPath& InPath, UClass* InClassFilter = UObject::StaticClass());
	FLOW_API FFlowDataPinValue_Class(const TArray<FSoftClassPath>& InPaths, UClass* InClassFilter = UObject::StaticClass());
	FLOW_API FFlowDataPinValue_Class(const UClass* InClass, UClass* InClassFilter = UObject::StaticClass());
	FLOW_API FFlowDataPinValue_Class(const TArray<UClass*>& InClasses, UClass* InClassFilter = UObject::StaticClass());

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinType::GetPinTypeNameStatic(); }
	virtual bool TryConvertValuesToString(FString& OutString) const override;
};