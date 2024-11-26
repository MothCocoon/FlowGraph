// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Nodes/FlowPin.h"

#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/Class.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif

#include "FlowDataPinProperties.generated.h"

class FStructProperty;
class UScriptStruct;

USTRUCT(BlueprintType, DisplayName = "Base - Flow DataPin Property")
struct FFlowDataPinProperty
{
	GENERATED_BODY()

	FFlowDataPinProperty() { }

	virtual ~FFlowDataPinProperty() { }

	virtual EFlowPinType GetFlowPinType() const { return EFlowPinType::Invalid; }

#if WITH_EDITOR
	FLOW_API static FFlowPin CreateFlowPin(const FName& PinName, const TInstancedStruct<FFlowDataPinProperty>& DataPinProperty);

	template <typename TFlowDataPinPropertyType, typename TUnrealType>
	static UScriptStruct* FindScriptStructForFlowDataPinProperty(const FProperty& Property)
	{
		// Find the ScriptStruct of the wrapped struct in a wrapper (eg, FFlowDataPinOutputProperty_Vector) or the struct itself (eg, FVector)
		const FStructProperty* StructProperty = CastField<FStructProperty>(&Property);
		if (!StructProperty)
		{
			return nullptr;
		}

		UScriptStruct* ScriptStruct = TFlowDataPinPropertyType::StaticStruct();
		if (StructProperty->Struct == ScriptStruct)
		{
			static UScriptStruct* UnrealType = TBaseStructure<TUnrealType>::Get();

			return UnrealType;
		}
		else
		{
			return StructProperty->Struct;
		}
	}
#endif
};

// Recommend implementing FFlowDataPinProperty... for every EFlowPinType
FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

// Wrapper struct for a bool that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Bool - Output Flow Data Pin Property", meta = (FlowPinType = "Bool"))
struct FFlowDataPinOutputProperty_Bool : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	bool Value = false;

public:

	FFlowDataPinOutputProperty_Bool() { }
	FFlowDataPinOutputProperty_Bool(bool InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Bool; }
};

// Wrapper struct for a int64 that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Int64 - Output Flow Data Pin Property", meta = (FlowPinType = "Int"))
struct FFlowDataPinOutputProperty_Int64 : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	int64 Value = 0;

public:

	FFlowDataPinOutputProperty_Int64() { }
	FFlowDataPinOutputProperty_Int64(int64 InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Int; }
};

// Wrapper struct for a int32 that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Int - Output Flow Data Pin Property", meta = (FlowPinType = "Int"))
struct FFlowDataPinOutputProperty_Int32 : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	int32 Value = 0;

public:

	FFlowDataPinOutputProperty_Int32() { }
	FFlowDataPinOutputProperty_Int32(int32 InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Int; }
};

// Wrapper struct for a Double (64bit float) that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Double (float64) - Output Flow Data Pin Property", meta = (FlowPinType = "Float"))
struct FFlowDataPinOutputProperty_Double : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	double Value = 0;

public:

	FFlowDataPinOutputProperty_Double() { }
	FFlowDataPinOutputProperty_Double(double InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Float; }
};

// Wrapper struct for a Float (32bit) that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Float - Output Flow Data Pin Property", meta = (FlowPinType = "Float"))
struct FFlowDataPinOutputProperty_Float : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	float Value = 0.0f;

public:

	FFlowDataPinOutputProperty_Float() { }
	FFlowDataPinOutputProperty_Float(float InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Float; }
};

// Wrapper struct for a FName that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Name - Output Flow Data Pin Property", meta = (FlowPinType = "Name"))
struct FFlowDataPinOutputProperty_Name : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FName Value = NAME_None;

public:

	FFlowDataPinOutputProperty_Name() { }
	FFlowDataPinOutputProperty_Name(const FName& InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Name; }
};

// Wrapper struct for a FString that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "String - Output Flow Data Pin Property", meta = (FlowPinType = "String"))
struct FFlowDataPinOutputProperty_String : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FString Value;

public:

	FFlowDataPinOutputProperty_String() { }
	FFlowDataPinOutputProperty_String(const FString& InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::String; }
};

// Wrapper struct for a FText that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Text - Output Flow Data Pin Property", meta = (FlowPinType = "Text"))
struct FFlowDataPinOutputProperty_Text : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FText Value;

public:

	FFlowDataPinOutputProperty_Text() { }
	FFlowDataPinOutputProperty_Text(const FText& InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Text; }
};

// Wrapper struct for an enum that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Enum - Output Flow Data Pin Property", meta = (FlowPinType = "Enum"))
struct FFlowDataPinOutputProperty_Enum : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	// The selected enum Value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FName Value = NAME_None;

	// Class for this enum
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	TObjectPtr<UEnum> EnumClass = nullptr;

#if WITH_EDITORONLY_DATA
	// name of enum defined in c++ code, will take priority over asset from EnumType property
	//  (this is a work-around because EnumClass cannot find C++ Enums, 
	//   so you need to type the name of the enum in here, manually)
	// See also: UBlackboardKeyType_Enum::PostEditChangeProperty()
	UPROPERTY(EditAnywhere, Category = Blackboard)
	FString EnumName;
#endif // WITH_EDITORONLY_DATA

public:

	FFlowDataPinOutputProperty_Enum() { }
	FFlowDataPinOutputProperty_Enum(const FName& InValue, UEnum* InEnumClass)
		: Value(InValue)
		, EnumClass(InEnumClass)
		{ }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Enum; }

#if WITH_EDITOR
	FLOW_API void OnEnumNameChanged();
#endif // WITH_EDITOR
};

// Wrapper struct for a FVector that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Vector - Output Flow Data Pin Property", meta = (FlowPinType = "Vector"))
struct FFlowDataPinOutputProperty_Vector : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FVector Value = FVector::ZeroVector;

public:

	FFlowDataPinOutputProperty_Vector() {}
	FFlowDataPinOutputProperty_Vector(const FVector& InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Vector; }
};

// Wrapper struct for a FRotator that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Rotator - Output Flow Data Pin Property", meta = (FlowPinType = "Rotator"))
struct FFlowDataPinOutputProperty_Rotator : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FRotator Value = FRotator::ZeroRotator;

public:

	FFlowDataPinOutputProperty_Rotator() {}
	FFlowDataPinOutputProperty_Rotator(const FRotator& InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Rotator; }
};

// Wrapper struct for a FTransform that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Transform - Output Flow Data Pin Property", meta = (FlowPinType = "Transform"))
struct FFlowDataPinOutputProperty_Transform : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FTransform Value;

public:

	FFlowDataPinOutputProperty_Transform() {}
	FFlowDataPinOutputProperty_Transform(const FTransform& InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Transform; }
};

// Wrapper struct for a FGameplayTag that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "GameplayTag - Output Flow Data Pin Property", meta = (FlowPinType = "GameplayTag"))
struct FFlowDataPinOutputProperty_GameplayTag : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FGameplayTag Value;

public:

	FFlowDataPinOutputProperty_GameplayTag() {}
	FFlowDataPinOutputProperty_GameplayTag(const FGameplayTag& InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::GameplayTag; }
};

// Wrapper struct for a FGameplayTagContainer that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "GameplayTagContainer - Output Flow DataPin Property", meta = (FlowPinType = "GameplayTagContainer"))
struct FFlowDataPinOutputProperty_GameplayTagContainer : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FGameplayTagContainer Value;

public:

	FFlowDataPinOutputProperty_GameplayTagContainer() {}
	FFlowDataPinOutputProperty_GameplayTagContainer(const FGameplayTagContainer& InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::GameplayTagContainer; }
};

// Wrapper struct for a FInstancedStruct that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "InstancedStruct - Output Flow DataPin Property", meta = (FlowPinType = "InstancedStruct"))
struct FFlowDataPinOutputProperty_InstancedStruct : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FInstancedStruct Value;

public:

	FFlowDataPinOutputProperty_InstancedStruct() {}
	FFlowDataPinOutputProperty_InstancedStruct(const FInstancedStruct& InValue) : Value(InValue) { }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::InstancedStruct; }
};

// Wrapper struct for a UObject that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Object - Output Flow DataPin Property", meta = (FlowPinType = "Object"))
struct FFlowDataPinOutputProperty_Object : public FFlowDataPinProperty
{
	GENERATED_BODY()

	friend class FFlowDataPinProperty_ObjectCustomizationBase;

protected:

	// These pointers are separate so that the default value for the object can be configured 
	// in the editor according to the type of object that it is (instanced or not).

	// Object reference if the object is a non-instanced UObject type (ie, not EditInlineNew)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins, DisplayName = "Value (reference)", meta = (EditCondition = "InlineValue == nullptr"))
	TObjectPtr<UObject> ReferenceValue = nullptr;

	// Ofject reference if the object is an instanced UObject type (ie, EditInlineNew)
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = DataPins, DisplayName = "Value (inline)", meta = (EditCondition = "ReferenceValue == nullptr"))
	TObjectPtr<UObject> InlineValue = nullptr;

public:

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AllowAbstract))
	TObjectPtr<UClass> ClassFilter = nullptr;
#endif // WITH_EDITORONLY_DATA

public:

	FFlowDataPinOutputProperty_Object() {}
	FLOW_API FFlowDataPinOutputProperty_Object(UObject* InValue, UClass* InClassFilter);

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Object; }

	UObject* GetObjectValue() const { return ReferenceValue ? ReferenceValue : InlineValue; }
	void SetObjectValue(UObject* InValue);

#if WITH_EDITOR
	UClass* DeriveObjectClass(const FProperty& MetaDataProperty) const;
	FLOW_API static UClass* TryGetObjectClassFromProperty(const FProperty& MetaDataProperty);
#endif
};

// Wrapper struct for a UClass that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Class - Output Flow DataPin Property", meta = (FlowPinType = "Class"))
struct FFlowDataPinOutputProperty_Class : public FFlowDataPinProperty
{
	GENERATED_BODY()

	friend class FFlowDataPinProperty_ClassCustomizationBase;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FSoftClassPath Value;

public:

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AllowAbstract))
	TObjectPtr<UClass> ClassFilter = nullptr;
#endif // WITH_EDITORONLY_DATA

public:

	FFlowDataPinOutputProperty_Class() {}
	FFlowDataPinOutputProperty_Class(const FSoftClassPath& InValue, UClass* InClassFilter)
		: Value(InValue)
#if WITH_EDITOR
		, ClassFilter(InClassFilter)
#endif
	{ }

	virtual EFlowPinType GetFlowPinType() const override { return EFlowPinType::Class; }

#if WITH_EDITOR
	UClass* DeriveMetaClass(const FProperty& MetaDataProperty) const;
	FLOW_API static UClass* TryGetMetaClassFromProperty(const FProperty& MetaDataProperty);
#endif

	const FSoftClassPath& GetAsSoftClass() const { return Value; }
	UClass* GetResolvedClass() const { return Value.ResolveClass(); }
};

// Wrapper for FFlowDataPinProperty that is used for flow nodes that add 
// dynamic properties, with associated data pins, on the flow node instance
// (as opposed to C++ or blueprint compile-time).
USTRUCT(BlueprintType, DisplayName = "Flow Named Output DataPin Property")
struct FFlowNamedDataPinOutputProperty
{
	GENERATED_BODY()

public:

	// Name of this instanced property
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FName Name;

	// DataPinProperty payload
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (ExcludeBaseStruct, NoClear))
	TInstancedStruct<FFlowDataPinProperty> DataPinProperty;

public:

	FFlowNamedDataPinOutputProperty() { }

	bool IsValid() const { return Name != NAME_None && DataPinProperty.GetPtr() != nullptr; }

#if WITH_EDITOR
	FFlowPin CreateFlowPin() const { return FFlowDataPinProperty::CreateFlowPin(Name, DataPinProperty); }

	FLOW_API FText BuildHeaderText() const;
#endif // WITH_EDITOR
};

// Wrapper-structs for a blueprint defaulted input pin types
//   "Hidden" to keep them out of the TInstancedStruct<FFlowDataPinProperty> selection list (but they can still be authored as properties in blueprint)
//   "DefaultForInputFlowPin" to change them to an Defaulted-Input property (rather than an output property)

USTRUCT(BlueprintType, DisplayName = "Bool - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Bool"))
struct FFlowDataPinInputProperty_Bool : public FFlowDataPinOutputProperty_Bool
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Bool(bool InValue = false) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "Int64 - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Int"))
struct FFlowDataPinInputProperty_Int64 : public FFlowDataPinOutputProperty_Int64
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Int64(int64 InValue = 0) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "Int - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Int"))
struct FFlowDataPinInputProperty_Int32 : public FFlowDataPinOutputProperty_Int32
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Int32(int32 InValue = 0) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "Double (float64) - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Float"))
struct FFlowDataPinInputProperty_Double : public FFlowDataPinOutputProperty_Double
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Double(double InValue = 0.0) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "Float - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Float"))
struct FFlowDataPinInputProperty_Float : public FFlowDataPinOutputProperty_Float
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Float(float InValue = 0.0f) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "Name - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Name"))
struct FFlowDataPinInputProperty_Name : public FFlowDataPinOutputProperty_Name
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Name() : Super() { }
	FFlowDataPinInputProperty_Name(const FName& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "String - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "String"))
struct FFlowDataPinInputProperty_String : public FFlowDataPinOutputProperty_String
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_String() : Super() { }
	FFlowDataPinInputProperty_String(const FString& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "Text - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Text"))
struct FFlowDataPinInputProperty_Text : public FFlowDataPinOutputProperty_Text
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Text() : Super() { }
	FFlowDataPinInputProperty_Text(const FText& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "Enum - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Enum"))
struct FFlowDataPinInputProperty_Enum : public FFlowDataPinOutputProperty_Enum
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Enum() : Super() { }
	FFlowDataPinInputProperty_Enum(const FName& InValue, UEnum* InEnumClass) : Super(InValue, InEnumClass) { }
};

USTRUCT(BlueprintType, DisplayName = "Vector - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Vector"))
struct FFlowDataPinInputProperty_Vector : public FFlowDataPinOutputProperty_Vector
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Vector() : Super() { }
	FFlowDataPinInputProperty_Vector(const FVector& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "Rotator - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Rotator"))
struct FFlowDataPinInputProperty_Rotator : public FFlowDataPinOutputProperty_Rotator
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Rotator() : Super() { }
	FFlowDataPinInputProperty_Rotator(const FRotator& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "Transform - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Transform"))
struct FFlowDataPinInputProperty_Transform : public FFlowDataPinOutputProperty_Transform
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Transform() : Super() { }
	FFlowDataPinInputProperty_Transform(const FTransform& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "GameplayTag - Input Flow Data Pin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "GameplayTag"))
struct FFlowDataPinInputProperty_GameplayTag : public FFlowDataPinOutputProperty_GameplayTag
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_GameplayTag() : Super() { }
	FFlowDataPinInputProperty_GameplayTag(const FGameplayTag& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "GameplayTagContainer - Input Flow DataPin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "GameplayTagContainer"))
struct FFlowDataPinInputProperty_GameplayTagContainer : public FFlowDataPinOutputProperty_GameplayTagContainer
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_GameplayTagContainer() : Super() { }
	FFlowDataPinInputProperty_GameplayTagContainer(const FGameplayTagContainer& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "InstancedStruct - Input Flow DataPin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "InstancedStruct"))
struct FFlowDataPinInputProperty_InstancedStruct : public FFlowDataPinOutputProperty_InstancedStruct
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_InstancedStruct() : Super() { }
	FFlowDataPinInputProperty_InstancedStruct(const FInstancedStruct& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "Object - Input Flow DataPin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Object"))
struct FFlowDataPinInputProperty_Object : public FFlowDataPinOutputProperty_Object
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Object() : Super() { }
	FFlowDataPinInputProperty_Object(UObject* InValue, UClass* InClassFilter) : Super(InValue, InClassFilter) { }
};

USTRUCT(BlueprintType, DisplayName = "Class - Input Flow DataPin Property", meta = (Hidden, DefaultForInputFlowPin, FlowPinType = "Class"))
struct FFlowDataPinInputProperty_Class : public FFlowDataPinOutputProperty_Class
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Class() : Super() { }
	FFlowDataPinInputProperty_Class(const FSoftClassPath& InValue, UClass* InClassFilter) : Super(InValue, InClassFilter) { }
};

