// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowSubsystemVariables.h"

#define GET_BAG_VALUE(Type) if (MemberName == NAME_None) \
{ \
	return false; \
} \
const auto Bag = PropertyReference.RuntimeBag.IsValid() ? PropertyReference.RuntimeBag : PropertyReference.Properties; \
const auto Result = Bag.GetValue##Type(MemberName); \
if (Result.IsValid()) \
{ \
	OutValue = Result.GetValue(); \
	return true; \
} \
return false

#define GET_STRUCT_VALUE(Type) if (MemberName == NAME_None) \
{ \
	return false; \
} \
const auto Bag = PropertyReference.RuntimeBag.IsValid() ? PropertyReference.RuntimeBag : PropertyReference.Properties; \
const auto Result = Bag.GetValueStruct<Type>(MemberName); \
if (Result.IsValid()) \
{ \
	OutValue = *Result.GetValue(); \
	return true; \
} \
return false

#define SET_FLOW_VALUE(Type) if (MemberName == NAME_None) \
{ \
	return EFlowVariableSetResult::MemberNameNone; \
} \
auto& Bag = Property.RuntimeBag; \
if (Bag.FindPropertyDescByName(MemberName) == nullptr) \
{ \
	Bag.AddProperty(MemberName, EPropertyBagPropertyType::Type); \
} \
auto Result = Bag.SetValue##Type(MemberName, Value); \
switch (Result) \
{ \
	case EPropertyBagResult::TypeMismatch: \
		return EFlowVariableSetResult::TypeMismatch; \
	case EPropertyBagResult::OutOfBounds: \
		return EFlowVariableSetResult::OutOfBounds; \
	case EPropertyBagResult::PropertyNotFound: \
		return EFlowVariableSetResult::PropertyNotFound; \
	default: ; \
} \
return EFlowVariableSetResult::Success

#define SET_STRUCT_VALUE(Type) if (MemberName == NAME_None) \
{ \
return EFlowVariableSetResult::MemberNameNone; \
} \
auto& Bag = Property.RuntimeBag; \
if (Bag.FindPropertyDescByName(MemberName) == nullptr) \
{ \
Bag.AddProperty(MemberName, EPropertyBagPropertyType::Struct, TBaseStructure<Type>::Get()); \
} \
auto Result = Bag.SetValueStruct(MemberName, Value); \
switch (Result) \
{ \
case EPropertyBagResult::TypeMismatch: \
return EFlowVariableSetResult::TypeMismatch; \
case EPropertyBagResult::OutOfBounds: \
return EFlowVariableSetResult::OutOfBounds; \
case EPropertyBagResult::PropertyNotFound: \
return EFlowVariableSetResult::PropertyNotFound; \
default: ; \
} \
return EFlowVariableSetResult::Success

EFlowVariableSetResult UFlowSubsystemVariables::SetBoolVariable(FFlowPropertyBag& Property, const FName MemberName, const bool Value)
{
	SET_FLOW_VALUE(Bool);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetByteVariable(FFlowPropertyBag& Property, const FName MemberName, const uint8 Value)
{
	SET_FLOW_VALUE(Byte);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetInt32Variable(FFlowPropertyBag& Property, const FName MemberName, const int32 Value)
{
	SET_FLOW_VALUE(Int32);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetInt64Variable(FFlowPropertyBag& Property, const FName MemberName, const int64 Value)
{
	SET_FLOW_VALUE(Int64);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetFloatVariable(FFlowPropertyBag& Property, const FName MemberName, const float Value)
{
	SET_FLOW_VALUE(Float);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetDoubleVariable(FFlowPropertyBag& Property, const FName MemberName, const double Value)
{
	SET_FLOW_VALUE(Double);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetNameVariable(FFlowPropertyBag& Property, const FName MemberName, const FName Value)
{
	SET_FLOW_VALUE(Name);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetStringVariable(FFlowPropertyBag& Property, const FName MemberName, const FString Value)
{
	SET_FLOW_VALUE(String);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetTextVariable(FFlowPropertyBag& Property, const FName MemberName, const FText Value)
{
	SET_FLOW_VALUE(Text);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetObjectVariable(FFlowPropertyBag& Property, const FName MemberName, UObject* Value)
{
	SET_FLOW_VALUE(Object);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetClassVariable(FFlowPropertyBag& Property, const FName MemberName, UClass* Value)
{
	SET_FLOW_VALUE(Class);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetVectorVariable(FFlowPropertyBag& Property, const FName MemberName, const FVector Value)
{
	SET_STRUCT_VALUE(FVector);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetVector2DVariable(FFlowPropertyBag& Property, const FName MemberName, const FVector2D Value)
{
	SET_STRUCT_VALUE(FRotator);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetVector4Variable(FFlowPropertyBag& Property, const FName MemberName, const FVector4 Value)
{
	SET_STRUCT_VALUE(FVector4);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetIntVectorVariable(FFlowPropertyBag& Property, const FName MemberName, const FIntVector Value)
{
	SET_STRUCT_VALUE(FIntVector);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetIntVector2DVariable(FFlowPropertyBag& Property, const FName MemberName, const FIntPoint Value)
{
	SET_STRUCT_VALUE(FIntPoint);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetRotatorVariable(FFlowPropertyBag& Property, const FName MemberName, const FRotator Value)
{
	SET_STRUCT_VALUE(FRotator);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetTransformVariable(FFlowPropertyBag& Property, const FName MemberName, const FTransform Value)
{
	SET_STRUCT_VALUE(FTransform);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetGameplayTagVariable(FFlowPropertyBag& Property, const FName MemberName, const FGameplayTag Value)
{
	SET_STRUCT_VALUE(FGameplayTag);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetGameplayTagContainerVariable(FFlowPropertyBag& Property, const FName MemberName, const FGameplayTagContainer Value)
{
	SET_STRUCT_VALUE(FGameplayTagContainer);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetDateTimeVariable(FFlowPropertyBag& Property, const FName MemberName, const FDateTime Value)
{
	SET_STRUCT_VALUE(FDateTime);
}

EFlowVariableSetResult UFlowSubsystemVariables::SetQuatVariable(FFlowPropertyBag& Property, const FName MemberName, const FQuat Value)
{
	SET_STRUCT_VALUE(FQuat);
}

bool UFlowSubsystemVariables::GetVariableBool(const FFlowPropertyBag& PropertyReference, const FName MemberName, bool& OutValue)
{
	GET_BAG_VALUE(Bool);
}

bool UFlowSubsystemVariables::GetVariableByte(const FFlowPropertyBag& PropertyReference, const FName MemberName, uint8& OutValue)
{
	GET_BAG_VALUE(Byte);
}

bool UFlowSubsystemVariables::GetVariableInteger(const FFlowPropertyBag& PropertyReference, const FName MemberName, int32& OutValue)
{
	GET_BAG_VALUE(Int32);
}

bool UFlowSubsystemVariables::GetVariableInteger64(const FFlowPropertyBag& PropertyReference, const FName MemberName,int64& OutValue)
{
	GET_BAG_VALUE(Int64);
}

bool UFlowSubsystemVariables::GetVariableFloat(const FFlowPropertyBag& PropertyReference, const FName MemberName, float& OutValue)
{
	GET_BAG_VALUE(Float);
}

bool UFlowSubsystemVariables::GetVariableDouble(const FFlowPropertyBag& PropertyReference, const FName MemberName, double& OutValue)
{
	GET_BAG_VALUE(Double);
}

bool UFlowSubsystemVariables::GetVariableName(const FFlowPropertyBag& PropertyReference, const FName MemberName, FName& OutValue)
{
	GET_BAG_VALUE(Name);
}

bool UFlowSubsystemVariables::GetVariableString(const FFlowPropertyBag& PropertyReference, const FName MemberName, FString& OutValue)
{
	GET_BAG_VALUE(String);
}

bool UFlowSubsystemVariables::GetVariableText(const FFlowPropertyBag& PropertyReference, const FName MemberName, FText& OutValue)
{
	GET_BAG_VALUE(Text);
}

bool UFlowSubsystemVariables::GetVariableObject(const FFlowPropertyBag& PropertyReference, const FName MemberName, UObject*& OutValue)
{
	GET_BAG_VALUE(Object);
}

bool UFlowSubsystemVariables::GetVariableClass(const FFlowPropertyBag& PropertyReference, const FName MemberName, UClass*& OutValue)
{
	GET_BAG_VALUE(Class);
}

bool UFlowSubsystemVariables::GetVariableVector(const FFlowPropertyBag& PropertyReference, const FName MemberName, FVector& OutValue)
{
	GET_STRUCT_VALUE(FVector);
}

bool UFlowSubsystemVariables::GetVariableVector2D(const FFlowPropertyBag& PropertyReference, const FName MemberName, FVector2D& OutValue)
{
	GET_STRUCT_VALUE(FVector2D);
}

bool UFlowSubsystemVariables::GetVariableVector4(const FFlowPropertyBag& PropertyReference, const FName MemberName, FVector4& OutValue)
{
	GET_STRUCT_VALUE(FVector4);
}

bool UFlowSubsystemVariables::GetVariableIntVector(const FFlowPropertyBag& PropertyReference, const FName MemberName, FIntVector& OutValue)
{
	GET_STRUCT_VALUE(FIntVector);
}

bool UFlowSubsystemVariables::GetVariableIntVector2(const FFlowPropertyBag& PropertyReference, const FName MemberName, FIntPoint& OutValue)
{
	GET_STRUCT_VALUE(FIntPoint);
}

bool UFlowSubsystemVariables::GetVariableRotator(const FFlowPropertyBag& PropertyReference, const FName MemberName, FRotator& OutValue)
{
	GET_STRUCT_VALUE(FRotator);
}

bool UFlowSubsystemVariables::GetVariableTransform(const FFlowPropertyBag& PropertyReference, const FName MemberName, FTransform& OutValue)
{
	GET_STRUCT_VALUE(FTransform);
}

bool UFlowSubsystemVariables::GetVariableGameplayTag(const FFlowPropertyBag& PropertyReference, const FName MemberName, FGameplayTag& OutValue)
{
	GET_STRUCT_VALUE(FGameplayTag);
}

bool UFlowSubsystemVariables::GetVariableGameplayTagContainer(const FFlowPropertyBag& PropertyReference, const FName MemberName, FGameplayTagContainer& OutValue)
{
	GET_STRUCT_VALUE(FGameplayTagContainer);
}

bool UFlowSubsystemVariables::GetVariableDateTime(const FFlowPropertyBag& PropertyReference, const FName MemberName, FDateTime& OutValue)
{
	GET_STRUCT_VALUE(FDateTime);
}

bool UFlowSubsystemVariables::GetVariableQuat(const FFlowPropertyBag& PropertyReference, const FName MemberName, FQuat& OutValue)
{
	GET_STRUCT_VALUE(FQuat);
}

#undef GET_BAG_VALUE
#undef GET_STRUCT_VALUE
#undef SET_FLOW_VALUE
#undef SET_STRUCT_VALUE
