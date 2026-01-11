// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Class.h"

#include "FlowDataPinProperties.generated.h"

// #FlowDataPinLegacy
USTRUCT(DisplayName = "Base - Flow DataPin Property", meta = (Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinProperty
{
	GENERATED_BODY()

	FFlowDataPinProperty() = default;

	virtual ~FFlowDataPinProperty() { }
};

// Wrapper struct for a bool that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Bool - Output Flow Data Pin Property", meta = (FlowPinType = "Bool", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Bool : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	bool Value = false;

public:

	FFlowDataPinOutputProperty_Bool() { }
	FFlowDataPinOutputProperty_Bool(bool InValue) : Value(InValue) { }
};

// Wrapper struct for an int64 that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Int64 - Output Flow Data Pin Property", meta = (FlowPinType = "Int64", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Int64 : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	int64 Value = 0;

public:

	FFlowDataPinOutputProperty_Int64() { }
	FFlowDataPinOutputProperty_Int64(int64 InValue) : Value(InValue) { }
};

// Wrapper struct for an int32 that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Int - Output Flow Data Pin Property", meta = (FlowPinType = "Int", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Int32 : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	int32 Value = 0;

public:

	FFlowDataPinOutputProperty_Int32() { }
	FFlowDataPinOutputProperty_Int32(int32 InValue) : Value(InValue) { }
};

// Wrapper struct for a Double (64bit float) that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Double (float64) - Output Flow Data Pin Property", meta = (FlowPinType = "Double", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Double : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	double Value = 0;

public:

	FFlowDataPinOutputProperty_Double() { }
	FFlowDataPinOutputProperty_Double(double InValue) : Value(InValue) { }
};

// Wrapper struct for a Float (32bit) that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Float - Output Flow Data Pin Property", meta = (FlowPinType = "Float", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Float : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	float Value = 0.0f;

public:

	FFlowDataPinOutputProperty_Float() { }
	FFlowDataPinOutputProperty_Float(float InValue) : Value(InValue) { }
};

// Wrapper struct for a FName that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Name - Output Flow Data Pin Property", meta = (FlowPinType = "Name", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Name : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FName Value = NAME_None;

public:

	FFlowDataPinOutputProperty_Name() { }
	FFlowDataPinOutputProperty_Name(const FName& InValue) : Value(InValue) { }
};

// Wrapper struct for a FString that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] String - Output Flow Data Pin Property", meta = (FlowPinType = "String", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_String : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FString Value;

public:

	FFlowDataPinOutputProperty_String() { }
	FFlowDataPinOutputProperty_String(const FString& InValue) : Value(InValue) { }
};

// Wrapper struct for a FText that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Text - Output Flow Data Pin Property", meta = (FlowPinType = "Text", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Text : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FText Value;

public:

	FFlowDataPinOutputProperty_Text() { }
	FFlowDataPinOutputProperty_Text(const FText& InValue) : Value(InValue) { }
};

// Wrapper struct for an enum that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Enum - Output Flow Data Pin Property", meta = (FlowPinType = "Enum", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
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
	{
	}
};

// Wrapper struct for a FVector that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Vector - Output Flow Data Pin Property", meta = (FlowPinType = "Vector", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Vector : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FVector Value = FVector::ZeroVector;

public:

	FFlowDataPinOutputProperty_Vector() {}
	FFlowDataPinOutputProperty_Vector(const FVector& InValue) : Value(InValue) { }
};

// Wrapper struct for a FRotator that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "Rotator - Output Flow Data Pin Property", meta = (FlowPinType = "Rotator", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Rotator : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FRotator Value = FRotator::ZeroRotator;

public:

	FFlowDataPinOutputProperty_Rotator() {}
	FFlowDataPinOutputProperty_Rotator(const FRotator& InValue) : Value(InValue) { }
};

// Wrapper struct for a FTransform that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Transform - Output Flow Data Pin Property", meta = (FlowPinType = "Transform", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Transform : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FTransform Value;

public:

	FFlowDataPinOutputProperty_Transform() {}
	FFlowDataPinOutputProperty_Transform(const FTransform& InValue) : Value(InValue) { }
};

// Wrapper struct for a FGameplayTag that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "GameplayTag - Output Flow Data Pin Property", meta = (FlowPinType = "GameplayTag", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_GameplayTag : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FGameplayTag Value;

public:

	FFlowDataPinOutputProperty_GameplayTag() {}
	FFlowDataPinOutputProperty_GameplayTag(const FGameplayTag& InValue) : Value(InValue) { }
};

// Wrapper struct for a FGameplayTagContainer that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] GameplayTagContainer - Output Flow DataPin Property", meta = (FlowPinType = "GameplayTagContainer", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_GameplayTagContainer : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FGameplayTagContainer Value;

public:

	FFlowDataPinOutputProperty_GameplayTagContainer() {}
	FFlowDataPinOutputProperty_GameplayTagContainer(const FGameplayTagContainer& InValue) : Value(InValue) { }
};

// Wrapper struct for a FInstancedStruct that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "InstancedStruct - Output Flow DataPin Property", meta = (FlowPinType = "InstancedStruct", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_InstancedStruct : public FFlowDataPinProperty
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FInstancedStruct Value;

public:

	FFlowDataPinOutputProperty_InstancedStruct() {}
	FFlowDataPinOutputProperty_InstancedStruct(const FInstancedStruct& InValue) : Value(InValue) { }
};

// Wrapper struct for a UObject that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Object - Output Flow DataPin Property", meta = (FlowPinType = "Object", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Object : public FFlowDataPinProperty
{
	GENERATED_BODY()

	friend class FFlowDataPinProperty_ObjectCustomizationBase;

public:

	// These pointers are separate so that the default value for the object can be configured 
	// in the editor according to the type of object that it is (instanced or not).

	// Object reference if the object is a non-instanced UObject type (ie, not EditInlineNew)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins, DisplayName = "Value (reference)", meta = (EditCondition = "InlineValue == nullptr"))
	TObjectPtr<UObject> ReferenceValue = nullptr;

	// Ofject reference if the object is an instanced UObject type (ie, EditInlineNew)
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = DataPins, DisplayName = "Value (inline)", meta = (EditCondition = "ReferenceValue == nullptr"))
	TObjectPtr<UObject> InlineValue = nullptr;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AllowAbstract))
	TObjectPtr<UClass> ClassFilter = UObject::StaticClass();
#endif // WITH_EDITORONLY_DATA

public:

	FFlowDataPinOutputProperty_Object() {}
	FLOW_API FFlowDataPinOutputProperty_Object(UObject* InValue, UClass* InClassFilter = nullptr);

	UObject* GetObjectValue() const { return ReferenceValue ? ReferenceValue : InlineValue; }
};

// Wrapper struct for a UClass that will generate and link to a Data Pin with its same name
USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Class - Output Flow DataPin Property", meta = (FlowPinType = "Class", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinOutputProperty_Class : public FFlowDataPinProperty
{
	GENERATED_BODY()

	friend class FFlowDataPinProperty_ClassCustomizationBase;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DataPins)
	FSoftClassPath Value;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = DataPins, meta = (AllowAbstract))
	TObjectPtr<UClass> ClassFilter = UObject::StaticClass();
#endif // WITH_EDITORONLY_DATA

public:

	FFlowDataPinOutputProperty_Class() {}
	FFlowDataPinOutputProperty_Class(const FSoftClassPath& InValue, UClass* InClassFilter = nullptr)
		: Value(InValue)
#if WITH_EDITOR
		, ClassFilter(InClassFilter)
#endif
	{
	}

	UClass* GetObjectValue() const { return Value.ResolveClass(); }
};

// Wrapper-structs for a blueprint defaulted input pin types
//   "Hidden" to keep them out of the TInstancedStruct<FFlowDataPinValue> selection list (but they can still be authored as properties in blueprint)
//   "DefaultForInputFlowPin" to change them to a Defaulted-Input property (rather than an output property)

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Bool - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Bool", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Bool : public FFlowDataPinOutputProperty_Bool
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Bool(bool InValue = false) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Int64 - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Int64", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Int64 : public FFlowDataPinOutputProperty_Int64
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Int64(int64 InValue = 0) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Int - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Int", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Int32 : public FFlowDataPinOutputProperty_Int32
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Int32(int32 InValue = 0) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Double (float64) - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Double", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Double : public FFlowDataPinOutputProperty_Double
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Double(double InValue = 0.0) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Float - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Float", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Float : public FFlowDataPinOutputProperty_Float
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Float(float InValue = 0.0f) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Name - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Name", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Name : public FFlowDataPinOutputProperty_Name
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Name() : Super() { }
	FFlowDataPinInputProperty_Name(const FName& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "String - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "String", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_String : public FFlowDataPinOutputProperty_String
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_String() : Super() { }
	FFlowDataPinInputProperty_String(const FString& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Text - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Text", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Text : public FFlowDataPinOutputProperty_Text
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Text() : Super() { }
	FFlowDataPinInputProperty_Text(const FText& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Enum - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Enum", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Enum : public FFlowDataPinOutputProperty_Enum
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Enum() : Super() { }
	FFlowDataPinInputProperty_Enum(const FName& InValue, UEnum* InEnumClass) : Super(InValue, InEnumClass) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Vector - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Vector", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Vector : public FFlowDataPinOutputProperty_Vector
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Vector() : Super() { }
	FFlowDataPinInputProperty_Vector(const FVector& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Rotator - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Rotator", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Rotator : public FFlowDataPinOutputProperty_Rotator
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Rotator() : Super() { }
	FFlowDataPinInputProperty_Rotator(const FRotator& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Transform - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Transform", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Transform : public FFlowDataPinOutputProperty_Transform
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Transform() : Super() { }
	FFlowDataPinInputProperty_Transform(const FTransform& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] GameplayTag - Input Flow Data Pin Property", meta = (DefaultForInputFlowPin, FlowPinType = "GameplayTag", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_GameplayTag : public FFlowDataPinOutputProperty_GameplayTag
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_GameplayTag() : Super() { }
	FFlowDataPinInputProperty_GameplayTag(const FGameplayTag& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] GameplayTagContainer - Input Flow DataPin Property", meta = (DefaultForInputFlowPin, FlowPinType = "GameplayTagContainer", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_GameplayTagContainer : public FFlowDataPinOutputProperty_GameplayTagContainer
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_GameplayTagContainer() : Super() { }
	FFlowDataPinInputProperty_GameplayTagContainer(const FGameplayTagContainer& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] InstancedStruct - Input Flow DataPin Property", meta = (DefaultForInputFlowPin, FlowPinType = "InstancedStruct", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_InstancedStruct : public FFlowDataPinOutputProperty_InstancedStruct
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_InstancedStruct() : Super() { }
	FFlowDataPinInputProperty_InstancedStruct(const FInstancedStruct& InValue) : Super(InValue) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Object - Input Flow DataPin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Object", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Object : public FFlowDataPinOutputProperty_Object
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Object() : Super() { }
	FFlowDataPinInputProperty_Object(UObject* InValue, UClass* InClassFilter) : Super(InValue, InClassFilter) { }
};

USTRUCT(BlueprintType, DisplayName = "[DEPRECATED] Class - Input Flow DataPin Property", meta = (DefaultForInputFlowPin, FlowPinType = "Class", Deprecated, DeprecationMessage = "Use FFlowDataPinValue* instead"))
struct FFlowDataPinInputProperty_Class : public FFlowDataPinOutputProperty_Class
{
	GENERATED_BODY()

	FFlowDataPinInputProperty_Class() : Super() { }
	FFlowDataPinInputProperty_Class(const FSoftClassPath& InValue, UClass* InClassFilter) : Super(InValue, InClassFilter) { }
};
