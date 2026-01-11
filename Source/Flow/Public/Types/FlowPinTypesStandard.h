// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowPinType.h"
#include "Nodes/FlowPin.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Class.h"

#if WITH_EDITOR
#include "GraphEditorSettings.h"
#endif

#include "FlowPinTypesStandard.generated.h"

// Forward declarations
struct FFlowDataPinValue_Bool;
struct FFlowDataPinValue_Int;
struct FFlowDataPinValue_Int64;
struct FFlowDataPinValue_Float;
struct FFlowDataPinValue_Double;
struct FFlowDataPinValue_Name;
struct FFlowDataPinValue_String;
struct FFlowDataPinValue_Text;
struct FFlowDataPinValue_Enum;
struct FFlowDataPinValue_Vector;
struct FFlowDataPinValue_Rotator;
struct FFlowDataPinValue_Transform;
struct FFlowDataPinValue_GameplayTag;
struct FFlowDataPinValue_GameplayTagContainer;
struct FFlowDataPinValue_InstancedStruct;
struct FFlowDataPinValue_Object;
struct FFlowDataPinValue_Class;

// #FlowDataPinLegacy
struct FFlowDataPinOutputProperty_Bool;
struct FFlowDataPinOutputProperty_Int32;
struct FFlowDataPinOutputProperty_Int64;
struct FFlowDataPinOutputProperty_Float;
struct FFlowDataPinOutputProperty_Double;
struct FFlowDataPinOutputProperty_Name;
struct FFlowDataPinOutputProperty_String;
struct FFlowDataPinOutputProperty_Text;
struct FFlowDataPinOutputProperty_Enum;
struct FFlowDataPinOutputProperty_Vector;
struct FFlowDataPinOutputProperty_Rotator;
struct FFlowDataPinOutputProperty_Transform;
struct FFlowDataPinOutputProperty_GameplayTag;
struct FFlowDataPinOutputProperty_GameplayTagContainer;
struct FFlowDataPinOutputProperty_InstancedStruct;
struct FFlowDataPinOutputProperty_Object;
struct FFlowDataPinOutputProperty_Class;
// --

// Exec
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Exec : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = void;
	using WrapperType = void;
	using MainPropertyType = void;
	using LegacyWrapperType = void;

private:
	static const FFlowPinTypeName PinTypeNameExec;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameExec; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameExec; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->ExecutionPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
#endif
};

// Bool
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Bool : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = bool;
	using WrapperType = FFlowDataPinValue_Bool;
	using MainPropertyType = FBoolProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Bool;

private:
	static const FFlowPinTypeName PinTypeNameBool;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameBool; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameBool; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->BooleanPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Int
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Int : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = int32;
	using WrapperType = FFlowDataPinValue_Int;
	using MainPropertyType = FIntProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Int32;

private:
	static const FFlowPinTypeName PinTypeNameInt;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameInt; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameInt; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->IntPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Int64
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Int64 : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = int64;
	using WrapperType = FFlowDataPinValue_Int64;
	using MainPropertyType = FInt64Property;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Int64;

private:
	static const FFlowPinTypeName PinTypeNameInt64;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameInt64; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameInt64; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->IntPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Float
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Float : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = float;
	using WrapperType = FFlowDataPinValue_Float;
	using MainPropertyType = FFloatProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Float;

private:
	static const FFlowPinTypeName PinTypeNameFloat;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameFloat; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameFloat; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->FloatPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Double
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Double : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = double;
	using WrapperType = FFlowDataPinValue_Double;
	using MainPropertyType = FDoubleProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Double;

private:
	static const FFlowPinTypeName PinTypeNameDouble;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameDouble; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameDouble; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->FloatPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Name
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Name : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = FName;
	using WrapperType = FFlowDataPinValue_Name;
	using MainPropertyType = FNameProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Name;

private:
	static const FFlowPinTypeName PinTypeNameName;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameName; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameName; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->NamePinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// String
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_String : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = FString;
	using WrapperType = FFlowDataPinValue_String;
	using MainPropertyType = FStrProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_String;

private:
	static const FFlowPinTypeName PinTypeNameString;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameString; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameString; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->StringPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Text
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Text : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = FText;
	using WrapperType = FFlowDataPinValue_Text;
	using MainPropertyType = FTextProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Text;

private:
	static const FFlowPinTypeName PinTypeNameText;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameText; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameText; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->TextPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Enum
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Enum : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = FName;
	using WrapperType = FFlowDataPinValue_Enum;
	using MainPropertyType = FEnumProperty;
	using FieldType = UEnum;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Enum;

private:
	static const FFlowPinTypeName PinTypeNameEnum;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameEnum; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameEnum; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->DefaultPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
	virtual UObject* GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Vector
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Vector : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = FVector;
	using WrapperType = FFlowDataPinValue_Vector;
	using MainPropertyType = FStructProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Vector;

private:
	static const FFlowPinTypeName PinTypeNameVector;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameVector; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameVector; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->StructPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
	virtual UObject* GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Rotator
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Rotator : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = FRotator;
	using WrapperType = FFlowDataPinValue_Rotator;
	using MainPropertyType = FStructProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Rotator;

private:
	static const FFlowPinTypeName PinTypeNameRotator;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameRotator; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameRotator; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->StructPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
	virtual UObject* GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Transform
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Transform : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = FTransform;
	using WrapperType = FFlowDataPinValue_Transform;
	using MainPropertyType = FStructProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Transform;

private:
	static const FFlowPinTypeName PinTypeNameTransform;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameTransform; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameTransform; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->StructPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
	virtual UObject* GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// GameplayTag
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_GameplayTag : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = FGameplayTag;
	using WrapperType = FFlowDataPinValue_GameplayTag;
	using MainPropertyType = FStructProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_GameplayTag;

private:
	static const FFlowPinTypeName PinTypeNameGameplayTag;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameGameplayTag; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameGameplayTag; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->DefaultPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
	virtual UObject* GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// GameplayTagContainer
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_GameplayTagContainer : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = FGameplayTagContainer;
	using WrapperType = FFlowDataPinValue_GameplayTagContainer;
	using MainPropertyType = FStructProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_GameplayTagContainer;

private:
	static const FFlowPinTypeName PinTypeNameGameplayTagContainer;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameGameplayTagContainer; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameGameplayTagContainer; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->DefaultPinTypeColor; }
	virtual bool SupportsMultiType(EFlowDataMultiType Mode) const { FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2); return (Mode == EFlowDataMultiType::Single); }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
	virtual UObject* GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// InstancedStruct
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_InstancedStruct : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = FInstancedStruct;
	using WrapperType = FFlowDataPinValue_InstancedStruct;
	using MainPropertyType = FStructProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_InstancedStruct;

private:
	static const FFlowPinTypeName PinTypeNameInstancedStruct;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameInstancedStruct; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameInstancedStruct; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->StructPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
	virtual UObject* GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Object
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Object : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = TObjectPtr<UObject>;
	using WrapperType = FFlowDataPinValue_Object;
	using MainPropertyType = FObjectProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Object;

private:
	static const FFlowPinTypeName PinTypeNameObject;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameObject; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameObject; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->ObjectPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
	virtual UObject* GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const override;

	static UClass* TryGetObjectClassFromProperty(const FProperty& MetaDataProperty);
	static UClass* TryGetMetaClassFromProperty(const FProperty& MetaDataProperty);
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};

// Class
USTRUCT(BlueprintType)
struct FLOW_API FFlowPinType_Class : public FFlowPinType
{
	GENERATED_BODY()

	using ValueType = TObjectPtr<UClass>;
	using WrapperType = FFlowDataPinValue_Class;
	using MainPropertyType = FClassProperty;
	using LegacyWrapperType = FFlowDataPinOutputProperty_Class;

private:
	static const FFlowPinTypeName PinTypeNameClass;
public:
	static const FFlowPinTypeName& GetPinTypeNameStatic() { return PinTypeNameClass; }
	virtual const FFlowPinTypeName& GetPinTypeName() const override { return PinTypeNameClass; }

#if WITH_EDITOR
	virtual FLinearColor GetPinColor() const override { return GetDefault<UGraphEditorSettings>()->ClassPinTypeColor; }
	virtual bool ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const override;
	virtual UObject* GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const override;
#endif

	virtual bool PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const override;
};