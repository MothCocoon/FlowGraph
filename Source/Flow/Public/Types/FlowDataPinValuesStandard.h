// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowDataPinValue.h"
#include "Types/FlowDataPinTypeNamesStandard.h"

#include "StructUtils/InstancedStruct.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Class.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"

#include "FlowDataPinValuesStandard.generated.h"

// Bool
USTRUCT(BlueprintType, DisplayName = "Bool - Flow DataPin Value", meta = (FlowPinType = "Bool"))
struct FFlowDataPinValue_Bool : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef bool FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<bool> Values{ false };

	FFlowDataPinValue_Bool() {}
	FFlowDataPinValue_Bool(bool InValue) : Values({ InValue }) {}
	FFlowDataPinValue_Bool(const TArray<bool>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameBool; }
};

// Int (int32)
USTRUCT(BlueprintType, DisplayName = "Int - Flow DataPin Value", meta = (FlowPinType = "Int"))
struct FFlowDataPinValue_Int : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef int32 FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<int32> Values{ 0 };

	FFlowDataPinValue_Int() {}
	FFlowDataPinValue_Int(FValueType InValue) : Values({ InValue }) {}
	FFlowDataPinValue_Int(const TArray<FValueType>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameInt; }
};

// Int64
USTRUCT(BlueprintType, DisplayName = "Int64 - Flow DataPin Value", meta = (FlowPinType = "Int64"))
struct FFlowDataPinValue_Int64 : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef int64 FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<int64> Values{ 0 };

	FFlowDataPinValue_Int64() {}
	FFlowDataPinValue_Int64(int64 InValue) : Values({ InValue }) {}
	FFlowDataPinValue_Int64(const TArray<int64>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameInt64; }
};

// Float
USTRUCT(BlueprintType, DisplayName = "Float - Flow DataPin Value", meta = (FlowPinType = "Float"))
struct FFlowDataPinValue_Float : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef float FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<float> Values{ 0.f };

	FFlowDataPinValue_Float() {}
	FFlowDataPinValue_Float(float InValue) : Values({ InValue }) {}
	FFlowDataPinValue_Float(const TArray<float>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameFloat; }
};

// Double
USTRUCT(BlueprintType, DisplayName = "Double - Flow DataPin Value", meta = (FlowPinType = "Double"))
struct FFlowDataPinValue_Double : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef double FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<double> Values{ 0.0 };

	FFlowDataPinValue_Double() {}
	FFlowDataPinValue_Double(double InValue) : Values({ InValue }) {}
	FFlowDataPinValue_Double(const TArray<double>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameDouble; }
};

// Enum
USTRUCT(BlueprintType, DisplayName = "Enum - Flow DataPin Value", meta = (FlowPinType = "Enum"))
struct FFlowDataPinValue_Enum : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef FName FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FName> Values;

	// Enum asset reference (advanced)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins, meta = (NoClear, AdvancedDisplay))
	TSoftObjectPtr<UEnum> EnumClass;

#if WITH_EDITORONLY_DATA
	// Native C++ enum name (advanced)
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AdvancedDisplay))
	FString EnumName;

	// Lock source (asset/name). Does NOT lock enumerator selection.
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (DisplayName = "Lock Enum Class"))
	bool bLockEnumClass = false;
#endif

	FFlowDataPinValue_Enum() {}
	FFlowDataPinValue_Enum(const TSoftObjectPtr<UEnum>& InEnumClass, FName InValue)
		: Values({ InValue }), EnumClass(InEnumClass) {
	}
	FFlowDataPinValue_Enum(const TSoftObjectPtr<UEnum>& InEnumClass, const TArray<FName>& InValues)
		: Values(InValues), EnumClass(InEnumClass) {
	}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameEnum; }
#if WITH_EDITOR
	virtual UObject* GetSubCategoryObject() const override { return EnumClass.Get(); }

	FLOW_API void OnEnumNameChanged();
#endif
};

// Name
USTRUCT(BlueprintType, DisplayName = "Name - Flow DataPin Value", meta = (FlowPinType = "Name"))
struct FFlowDataPinValue_Name : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef FName FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FName> Values;

	FFlowDataPinValue_Name() {}
	FFlowDataPinValue_Name(const FName& InValue) : Values({ InValue }) {}
	FFlowDataPinValue_Name(const TArray<FName>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameName; }
};

// String
USTRUCT(BlueprintType, DisplayName = "String - Flow DataPin Value", meta = (FlowPinType = "String"))
struct FFlowDataPinValue_String : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef FString FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FString> Values;

	FFlowDataPinValue_String() {}
	FFlowDataPinValue_String(const FString& InValue) : Values({ InValue }) {}
	FFlowDataPinValue_String(const TArray<FString>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameString; }
};

// Text
USTRUCT(BlueprintType, DisplayName = "Text - Flow DataPin Value", meta = (FlowPinType = "Text"))
struct FFlowDataPinValue_Text : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef FText FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FText> Values;

	FFlowDataPinValue_Text() {}
	FFlowDataPinValue_Text(const FText& InValue) : Values({ InValue }) {}
	FFlowDataPinValue_Text(const TArray<FText>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameText; }
};

// Vector
USTRUCT(BlueprintType, DisplayName = "Vector - Flow DataPin Value", meta = (FlowPinType = "Vector"))
struct FFlowDataPinValue_Vector : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef FVector FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FVector> Values;

	FFlowDataPinValue_Vector() {}
	FFlowDataPinValue_Vector(const FVector& InValue) : Values({ InValue }) {}
	FFlowDataPinValue_Vector(const TArray<FVector>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameVector; }
#if WITH_EDITOR
	virtual UObject* GetSubCategoryObject() const override { return TBaseStructure<FVector>::Get(); }
#endif
};

// Rotator
USTRUCT(BlueprintType, DisplayName = "Rotator - Flow DataPin Value", meta = (FlowPinType = "Rotator"))
struct FFlowDataPinValue_Rotator : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef FRotator FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FRotator> Values;

	FFlowDataPinValue_Rotator() {}
	FFlowDataPinValue_Rotator(const FRotator& InValue) : Values({ InValue }) {}
	FFlowDataPinValue_Rotator(const TArray<FRotator>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameRotator; }
#if WITH_EDITOR
	virtual UObject* GetSubCategoryObject() const override { return TBaseStructure<FRotator>::Get(); }
#endif
};

// Transform
USTRUCT(BlueprintType, DisplayName = "Transform - Flow DataPin Value", meta = (FlowPinType = "Transform"))
struct FFlowDataPinValue_Transform : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef FTransform FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FTransform> Values;

	FFlowDataPinValue_Transform() {}
	FFlowDataPinValue_Transform(const FTransform& InValue) : Values({ InValue }) {}
	FFlowDataPinValue_Transform(const TArray<FTransform>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameTransform; }
#if WITH_EDITOR
	virtual UObject* GetSubCategoryObject() const override { return TBaseStructure<FTransform>::Get(); }
#endif
};

// GameplayTag
USTRUCT(BlueprintType, DisplayName = "GameplayTag - Flow DataPin Value", meta = (FlowPinType = "GameplayTag"))
struct FFlowDataPinValue_GameplayTag : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef FGameplayTag FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FGameplayTag> Values;

	FFlowDataPinValue_GameplayTag() {}
	FFlowDataPinValue_GameplayTag(const FGameplayTag& InValue) : Values({ InValue }) {}
	FFlowDataPinValue_GameplayTag(const TArray<FGameplayTag>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameGameplayTag; }
#if WITH_EDITOR
	virtual UObject* GetSubCategoryObject() const override { return TBaseStructure<FGameplayTag>::Get(); }
#endif
};

// GameplayTagContainer
USTRUCT(BlueprintType, DisplayName = "GameplayTagContainer - Flow DataPin Value", meta = (FlowPinType = "GameplayTagContainer"))
struct FFlowDataPinValue_GameplayTagContainer : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef FGameplayTagContainer FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FGameplayTagContainer> Values;

	FFlowDataPinValue_GameplayTagContainer() {}
	FFlowDataPinValue_GameplayTagContainer(const FGameplayTagContainer& InValue) : Values({ InValue }) {}
	FFlowDataPinValue_GameplayTagContainer(const TArray<FGameplayTagContainer>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameGameplayTagContainer; }
#if WITH_EDITOR
	virtual UObject* GetSubCategoryObject() const override { return TBaseStructure<FGameplayTagContainer>::Get(); }
#endif
};

// InstancedStruct
USTRUCT(BlueprintType, DisplayName = "InstancedStruct - Flow DataPin Value", meta = (FlowPinType = "InstancedStruct"))
struct FFlowDataPinValue_InstancedStruct : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef FInstancedStruct FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FInstancedStruct> Values;

	FFlowDataPinValue_InstancedStruct() {}
	FFlowDataPinValue_InstancedStruct(const FInstancedStruct& InValue) : Values({ InValue }) {}
	FFlowDataPinValue_InstancedStruct(const TArray<FInstancedStruct>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameInstancedStruct; }
#if WITH_EDITOR
	virtual UObject* GetSubCategoryObject() const override { return TBaseStructure<FInstancedStruct>::Get(); }
#endif
};

// Object (reference)
USTRUCT(BlueprintType, DisplayName = "Object - Flow DataPin Value", meta = (FlowPinType = "Object"))
struct FFlowDataPinValue_Object : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef UObject* FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<TObjectPtr<UObject>> Values;

#if WITH_EDITORONLY_DATA
	// Class filter (advanced)
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AllowAbstract, AdvancedDisplay))
	TObjectPtr<UClass> ClassFilter = UObject::StaticClass();

	UPROPERTY(EditAnywhere, Category = DataPins, meta = (DisplayName = "Lock Class Filter"))
	bool bLockClassFilter = false;
#endif

	FFlowDataPinValue_Object() {}
	FFlowDataPinValue_Object(UObject* InObject)
	{
		if (InObject)
		{
			Values.Add(InObject);
		}
	}
	FFlowDataPinValue_Object(const TArray<UObject*>& InObjects)
	{
		for (UObject* Obj : InObjects)
		{
			if (Obj)
			{
				Values.Add(Obj);
			}
		}
	}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameObject; }

#if WITH_EDITOR
	virtual UObject* GetSubCategoryObject() const override { return ClassFilter; }
#endif
};

// InstancedObject (inline / instanced editing)
USTRUCT(BlueprintType, DisplayName = "InstancedObject - Flow DataPin Value", meta = (FlowPinType = "InstancedObject"))
struct FFlowDataPinValue_InstancedObject : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef UObject* FValueType;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = DataPins)
	TArray<TObjectPtr<UObject>> Values;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AllowAbstract, AdvancedDisplay))
	TObjectPtr<UClass> ClassFilter = UObject::StaticClass();

	UPROPERTY(EditAnywhere, Category = DataPins, meta = (DisplayName = "Lock Class Filter"))
	bool bLockClassFilter = false;
#endif

	FFlowDataPinValue_InstancedObject() {}
	FFlowDataPinValue_InstancedObject(UObject* InObject)
	{
		if (InObject)
		{
			Values.Add(InObject);
		}
	}
	FFlowDataPinValue_InstancedObject(const TArray<UObject*>& InObjects)
	{
		for (UObject* Obj : InObjects)
		{
			if (Obj)
			{
				Values.Add(Obj);
			}
		}
	}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameInstancedObject; }
#if WITH_EDITOR
	virtual UObject* GetSubCategoryObject() const override { return ClassFilter; }
#endif
};

// Class (soft class paths)
USTRUCT(BlueprintType, DisplayName = "Class - Flow DataPin Value", meta = (FlowPinType = "Class"))
struct FFlowDataPinValue_Class : public FFlowDataPinValue
{
	GENERATED_BODY()

public:
	typedef UClass* FValueType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TArray<FSoftClassPath> Values;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AllowAbstract, AdvancedDisplay))
	TObjectPtr<UClass> ClassFilter = UObject::StaticClass();

	UPROPERTY(EditAnywhere, Category = DataPins, meta = (DisplayName = "Lock Class Filter"))
	bool bLockClassFilter = false;
#endif

	FFlowDataPinValue_Class() {}
	FFlowDataPinValue_Class(const FSoftClassPath& InPath) : Values({ InPath }) {}
	FFlowDataPinValue_Class(const TArray<FSoftClassPath>& InValues) : Values(InValues) {}

	virtual const FFlowPinTypeName& GetPinTypeName() const override { return FFlowDataPinTypeNamesStandard::ValueTypeNameClass; }
#if WITH_EDITOR
	virtual UObject* GetSubCategoryObject() const override { return ClassFilter; }
#endif
};