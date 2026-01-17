// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowDataPinBlueprintLibrary.h"
#include "Types/FlowDataPinValue.h"
#include "Nodes/FlowNodeBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDataPinBlueprintLibrary)

void UFlowDataPinBlueprintLibrary::ResolveAndExtract_Impl(
	UFlowNodeBase* Target,
	FName PinName,
	EFlowDataPinResolveSimpleResult& SimpleResult,
	EFlowDataPinResolveResult& ResultEnum,
	auto&& ExtractLambda)
{
	using namespace FlowPinType;

	if (!IsValid(Target))
	{
		ResultEnum = EFlowDataPinResolveResult::FailedNullFlowNodeBase;
		SimpleResult = ConvertToSimpleResult(ResultEnum);
		return;
	}

	ResultEnum = ExtractLambda();
	SimpleResult = ConvertToSimpleResult(ResultEnum);
}

void UFlowDataPinBlueprintLibrary::ResolveAsBool(UFlowNodeBase* Target, const FFlowDataPinValue_Bool& BoolValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, bool& Value, EFlowSingleFromArray SingleMode)
{
	Value = false;
	ResolveAndExtract_Impl(Target, BoolValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_Bool>(BoolValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsBoolArray(UFlowNodeBase* Target, const FFlowDataPinValue_Bool& BoolValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<bool>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, BoolValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_Bool>(BoolValue.PropertyPinName, Values);
		});
}

// Int
void UFlowDataPinBlueprintLibrary::ResolveAsInt(UFlowNodeBase* Target, const FFlowDataPinValue_Int& IntValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, int32& Value, EFlowSingleFromArray SingleMode)
{
	Value = 0;
	ResolveAndExtract_Impl(Target, IntValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_Int>(IntValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsIntArray(UFlowNodeBase* Target, const FFlowDataPinValue_Int& IntValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<int32>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, IntValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_Int>(IntValue.PropertyPinName, Values);
		});
}

// Int64
void UFlowDataPinBlueprintLibrary::ResolveAsInt64(UFlowNodeBase* Target, const FFlowDataPinValue_Int64& Int64Value, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, int64& Value, EFlowSingleFromArray SingleMode)
{
	Value = 0;
	ResolveAndExtract_Impl(Target, Int64Value.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_Int64>(Int64Value.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsInt64Array(UFlowNodeBase* Target, const FFlowDataPinValue_Int64& Int64Value, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<int64>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, Int64Value.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_Int64>(Int64Value.PropertyPinName, Values);
		});
}

// Float
void UFlowDataPinBlueprintLibrary::ResolveAsFloat(UFlowNodeBase* Target, const FFlowDataPinValue_Float& FloatValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, float& Value, EFlowSingleFromArray SingleMode)
{
	Value = 0.0f;
	ResolveAndExtract_Impl(Target, FloatValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_Float>(FloatValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsFloatArray(UFlowNodeBase* Target, const FFlowDataPinValue_Float& FloatValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<float>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, FloatValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_Float>(FloatValue.PropertyPinName, Values);
		});
}

// Double
void UFlowDataPinBlueprintLibrary::ResolveAsDouble(UFlowNodeBase* Target, const FFlowDataPinValue_Double& DoubleValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, double& Value, EFlowSingleFromArray SingleMode)
{
	Value = 0.0;
	ResolveAndExtract_Impl(Target, DoubleValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_Double>(DoubleValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsDoubleArray(UFlowNodeBase* Target, const FFlowDataPinValue_Double& DoubleValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<double>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, DoubleValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_Double>(DoubleValue.PropertyPinName, Values);
		});
}

// Name
void UFlowDataPinBlueprintLibrary::ResolveAsName(UFlowNodeBase* Target, const FFlowDataPinValue_Name& NameValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FName& Value, EFlowSingleFromArray SingleMode)
{
	Value = NAME_None;
	ResolveAndExtract_Impl(Target, NameValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_Name>(NameValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsNameArray(UFlowNodeBase* Target, const FFlowDataPinValue_Name& NameValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FName>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, NameValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_Name>(NameValue.PropertyPinName, Values);
		});
}

// String
void UFlowDataPinBlueprintLibrary::ResolveAsString(UFlowNodeBase* Target, const FFlowDataPinValue_String& StringValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FString& Value, EFlowSingleFromArray SingleMode)
{
	Value = FString();
	ResolveAndExtract_Impl(Target, StringValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_String>(StringValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsStringArray(UFlowNodeBase* Target, const FFlowDataPinValue_String& StringValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FString>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, StringValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_String>(StringValue.PropertyPinName, Values);
		});
}

// Text
void UFlowDataPinBlueprintLibrary::ResolveAsText(UFlowNodeBase* Target, const FFlowDataPinValue_Text& TextValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FText& Value, EFlowSingleFromArray SingleMode)
{
	Value = FText::GetEmpty();
	ResolveAndExtract_Impl(Target, TextValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_Text>(TextValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsTextArray(UFlowNodeBase* Target, const FFlowDataPinValue_Text& TextValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FText>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, TextValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_Text>(TextValue.PropertyPinName, Values);
		});
}

// Enum
void UFlowDataPinBlueprintLibrary::ResolveAsEnum(UFlowNodeBase* Target, const FFlowDataPinValue_Enum& EnumValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, uint8& Value, EFlowSingleFromArray SingleMode)
{
	Value = 0;
	ResolveAndExtract_Impl(Target, EnumValue.PropertyPinName, Result, ResultEnum, [&]() {
		FName ExtractedName;
		UEnum* EnumClass = nullptr;
		const EFlowDataPinResolveResult ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Enum>(EnumValue.PropertyPinName, ExtractedName, EnumClass, SingleMode);
		if (FlowPinType::IsSuccess(ResolveResult) && ensure(IsValid(EnumClass)))
		{
			const int64 IntValue = EnumClass->GetValueByName(ExtractedName);
			Value = static_cast<uint8>(IntValue);
		}
		return ResolveResult;
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsEnumArray(UFlowNodeBase* Target, const FFlowDataPinValue_Enum& EnumValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<uint8>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, EnumValue.PropertyPinName, Result, ResultEnum, [&]() {
		TArray<FName> Names;
		UEnum* EnumClass = nullptr;
		const EFlowDataPinResolveResult ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Enum>(EnumValue.PropertyPinName, Names, EnumClass);
		if (FlowPinType::IsSuccess(ResolveResult) && ensure(IsValid(EnumClass)))
		{
			Values.Reserve(Names.Num());
			for (const FName& Name : Names)
			{
				const int64 IntValue = EnumClass->GetValueByName(Name);
				Values.Add(static_cast<uint8>(IntValue));
			}
		}
		return ResolveResult;
		});
}

// Vector
void UFlowDataPinBlueprintLibrary::ResolveAsVector(UFlowNodeBase* Target, const FFlowDataPinValue_Vector& VectorValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FVector& Value, EFlowSingleFromArray SingleMode)
{
	Value = FVector::ZeroVector;
	ResolveAndExtract_Impl(Target, VectorValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_Vector>(VectorValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsVectorArray(UFlowNodeBase* Target, const FFlowDataPinValue_Vector& VectorValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FVector>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, VectorValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_Vector>(VectorValue.PropertyPinName, Values);
		});
}

// Rotator
void UFlowDataPinBlueprintLibrary::ResolveAsRotator(UFlowNodeBase* Target, const FFlowDataPinValue_Rotator& RotatorValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FRotator& Value, EFlowSingleFromArray SingleMode)
{
	Value = FRotator::ZeroRotator;
	ResolveAndExtract_Impl(Target, RotatorValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_Rotator>(RotatorValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsRotatorArray(UFlowNodeBase* Target, const FFlowDataPinValue_Rotator& RotatorValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FRotator>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, RotatorValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_Rotator>(RotatorValue.PropertyPinName, Values);
		});
}

// Transform
void UFlowDataPinBlueprintLibrary::ResolveAsTransform(UFlowNodeBase* Target, const FFlowDataPinValue_Transform& TransformValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FTransform& Value, EFlowSingleFromArray SingleMode)
{
	Value = FTransform::Identity;
	ResolveAndExtract_Impl(Target, TransformValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_Transform>(TransformValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsTransformArray(UFlowNodeBase* Target, const FFlowDataPinValue_Transform& TransformValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FTransform>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, TransformValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_Transform>(TransformValue.PropertyPinName, Values);
		});
}

// GameplayTag
void UFlowDataPinBlueprintLibrary::ResolveAsGameplayTag(UFlowNodeBase* Target, const FFlowDataPinValue_GameplayTag& GameplayTagValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FGameplayTag& Value, EFlowSingleFromArray SingleMode)
{
	Value = FGameplayTag();
	ResolveAndExtract_Impl(Target, GameplayTagValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_GameplayTag>(GameplayTagValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsGameplayTagArray(UFlowNodeBase* Target, const FFlowDataPinValue_GameplayTag& GameplayTagValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FGameplayTag>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, GameplayTagValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_GameplayTag>(GameplayTagValue.PropertyPinName, Values);
		});
}

// GameplayTagContainer
void UFlowDataPinBlueprintLibrary::ResolveAsGameplayTagContainer(UFlowNodeBase* Target, const FFlowDataPinValue_GameplayTagContainer& GameplayTagContainerValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FGameplayTagContainer& Value)
{
	Value = FGameplayTagContainer();
	ResolveAndExtract_Impl(Target, GameplayTagContainerValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_GameplayTagContainer>(GameplayTagContainerValue.PropertyPinName, Value, EFlowSingleFromArray::FirstValue);
		});
}

// InstancedStruct
void UFlowDataPinBlueprintLibrary::ResolveAsInstancedStruct(UFlowNodeBase* Target, const FFlowDataPinValue_InstancedStruct& InstancedStructValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, FInstancedStruct& Value, EFlowSingleFromArray SingleMode)
{
	Value = FInstancedStruct();
	ResolveAndExtract_Impl(Target, InstancedStructValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValue<FFlowPinType_InstancedStruct>(InstancedStructValue.PropertyPinName, Value, SingleMode);
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsInstancedStructArray(UFlowNodeBase* Target, const FFlowDataPinValue_InstancedStruct& InstancedStructValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<FInstancedStruct>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, InstancedStructValue.PropertyPinName, Result, ResultEnum, [&]() {
		return Target->TryResolveDataPinValues<FFlowPinType_InstancedStruct>(InstancedStructValue.PropertyPinName, Values);
		});
}

// Object
void UFlowDataPinBlueprintLibrary::ResolveAsObject(UFlowNodeBase* Target, const FFlowDataPinValue_Object& ObjectValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, UObject*& Value, EFlowSingleFromArray SingleMode)
{
	Value = nullptr;
	ResolveAndExtract_Impl(Target, ObjectValue.PropertyPinName, Result, ResultEnum, [&]() {
		TObjectPtr<UObject> Obj;
		const EFlowDataPinResolveResult ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Object>(ObjectValue.PropertyPinName, Obj, SingleMode);
		Value = Obj;
		return ResolveResult;
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsObjectArray(UFlowNodeBase* Target, const FFlowDataPinValue_Object& ObjectValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<UObject*>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, ObjectValue.PropertyPinName, Result, ResultEnum, [&]() {
		TArray<TObjectPtr<UObject>> ObjArray;
		const EFlowDataPinResolveResult ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Object>(ObjectValue.PropertyPinName, ObjArray);
		Values = ObjArray;
		return ResolveResult;
		});
}

// Class
void UFlowDataPinBlueprintLibrary::ResolveAsClass(UFlowNodeBase* Target, const FFlowDataPinValue_Class& ClassValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, UClass*& Value, EFlowSingleFromArray SingleMode)
{
	Value = nullptr;
	ResolveAndExtract_Impl(Target, ClassValue.PropertyPinName, Result, ResultEnum, [&]() {
		TObjectPtr<UClass> ClassObj;
		const EFlowDataPinResolveResult ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Class>(ClassValue.PropertyPinName, ClassObj, SingleMode);
		Value = ClassObj;
		return ResolveResult;
		});
}

void UFlowDataPinBlueprintLibrary::ResolveAsClassArray(UFlowNodeBase* Target, const FFlowDataPinValue_Class& ClassValue, EFlowDataPinResolveSimpleResult& Result, EFlowDataPinResolveResult& ResultEnum, TArray<UClass*>& Values)
{
	Values.Reset();
	ResolveAndExtract_Impl(Target, ClassValue.PropertyPinName, Result, ResultEnum, [&]() {
		TArray<TObjectPtr<UClass>> ClassArray;
		const EFlowDataPinResolveResult ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Class>(ClassValue.PropertyPinName, ClassArray);
		Values = ClassArray;
		return ResolveResult;
		});
}

// Bool
bool UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsBool(const FFlowDataPinValue_Bool& BoolValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	bool Extracted = false;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Bool>(BoolValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Bool failed on pin '%s': %s"),
			*BoolValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<bool> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsBoolArray(const FFlowDataPinValue_Bool& BoolValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<bool> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Bool>(BoolValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Bool Array failed on pin '%s': %s"),
			*BoolValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<bool>();
	}

	return Extracted;
}

// Int
int32 UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsInt(const FFlowDataPinValue_Int& IntValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	int32 Extracted = 0;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Int>(IntValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Int failed on pin '%s': %s"),
			*IntValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<int32> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsIntArray(const FFlowDataPinValue_Int& IntValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<int32> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Int>(IntValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Int Array failed on pin '%s': %s"),
			*IntValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<int32>();
	}

	return Extracted;
}

// Int64
int64 UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsInt64(const FFlowDataPinValue_Int64& Int64Value, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	int64 Extracted = 0;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Int64>(Int64Value.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Int64 failed on pin '%s': %s"),
			*Int64Value.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<int64> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsInt64Array(const FFlowDataPinValue_Int64& Int64Value, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<int64> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Int64>(Int64Value.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Int64 Array failed on pin '%s': %s"),
			*Int64Value.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<int64>();
	}

	return Extracted;
}

// Float
float UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsFloat(const FFlowDataPinValue_Float& FloatValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	float Extracted = 0.0f;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Float>(FloatValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Float failed on pin '%s': %s"),
			*FloatValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<float> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsFloatArray(const FFlowDataPinValue_Float& FloatValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<float> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Float>(FloatValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Float Array failed on pin '%s': %s"),
			*FloatValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<float>();
	}

	return Extracted;
}

// Double
double UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsDouble(const FFlowDataPinValue_Double& DoubleValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	double Extracted = 0.0;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Double>(DoubleValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Double failed on pin '%s': %s"),
			*DoubleValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<double> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsDoubleArray(const FFlowDataPinValue_Double& DoubleValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<double> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Double>(DoubleValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Double Array failed on pin '%s': %s"),
			*DoubleValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<double>();
	}

	return Extracted;
}

// Name
FName UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsName(const FFlowDataPinValue_Name& NameValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	FName Extracted = NAME_None;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Name>(NameValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Name failed on pin '%s': %s"),
			*NameValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<FName> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsNameArray(const FFlowDataPinValue_Name& NameValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<FName> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Name>(NameValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Name Array failed on pin '%s': %s"),
			*NameValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<FName>();
	}

	return Extracted;
}

// String
FString UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsString(const FFlowDataPinValue_String& StringValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	FString Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_String>(StringValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to String failed on pin '%s': %s"),
			*StringValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<FString> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsStringArray(const FFlowDataPinValue_String& StringValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<FString> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_String>(StringValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to String Array failed on pin '%s': %s"),
			*StringValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<FString>();
	}

	return Extracted;
}

// Text
FText UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsText(const FFlowDataPinValue_Text& TextValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	FText Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Text>(TextValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Text failed on pin '%s': %s"),
			*TextValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<FText> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsTextArray(const FFlowDataPinValue_Text& TextValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<FText> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Text>(TextValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Text Array failed on pin '%s': %s"),
			*TextValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<FText>();
	}

	return Extracted;
}

// Enum
uint8 UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsEnum(const FFlowDataPinValue_Enum& EnumValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	FName ExtractedName;
	UEnum* EnumClass = nullptr;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Enum>(EnumValue.PropertyPinName, ExtractedName, EnumClass, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Enum failed on pin '%s': %s"),
			*EnumValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return static_cast<uint8>(INDEX_NONE);
	}

	if (ensure(IsValid(EnumClass)))
	{
		const uint64 ValueInt = EnumClass->GetValueByName(ExtractedName);
		return static_cast<uint8>(ValueInt);
	}

	return static_cast<uint8>(INDEX_NONE);
}

TArray<uint8> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsEnumArray(const FFlowDataPinValue_Enum& EnumValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<FName> ExtractedNames;
	UEnum* EnumClass = nullptr;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Enum>(EnumValue.PropertyPinName, ExtractedNames, EnumClass);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Enum Array failed on pin '%s': %s"),
			*EnumValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<uint8>();
	}

	if (!ensure(IsValid(EnumClass)))
	{
		return TArray<uint8>();
	}

	TArray<uint8> Result;
	Result.Reserve(ExtractedNames.Num());
	for (const FName& Name : ExtractedNames)
	{
		const uint64 ValueInt = EnumClass->GetValueByName(Name);
		Result.Add(static_cast<uint8>(ValueInt));
	}

	return Result;
}

// Vector
FVector UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsVector(const FFlowDataPinValue_Vector& VectorValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	FVector Extracted = FVector::ZeroVector;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Vector>(VectorValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Vector failed on pin '%s': %s"),
			*VectorValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<FVector> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsVectorArray(const FFlowDataPinValue_Vector& VectorValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<FVector> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Vector>(VectorValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Vector Array failed on pin '%s': %s"),
			*VectorValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<FVector>();
	}

	return Extracted;
}

// Rotator
FRotator UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsRotator(const FFlowDataPinValue_Rotator& RotatorValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	FRotator Extracted = FRotator::ZeroRotator;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Rotator>(RotatorValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Rotator failed on pin '%s': %s"),
			*RotatorValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<FRotator> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsRotatorArray(const FFlowDataPinValue_Rotator& RotatorValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<FRotator> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Rotator>(RotatorValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Rotator Array failed on pin '%s': %s"),
			*RotatorValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<FRotator>();
	}

	return Extracted;
}

// Transform
FTransform UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsTransform(const FFlowDataPinValue_Transform& TransformValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	FTransform Extracted = FTransform::Identity;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Transform>(TransformValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Transform failed on pin '%s': %s"),
			*TransformValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<FTransform> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsTransformArray(const FFlowDataPinValue_Transform& TransformValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<FTransform> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Transform>(TransformValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Transform Array failed on pin '%s': %s"),
			*TransformValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<FTransform>();
	}

	return Extracted;
}

// GameplayTag
FGameplayTag UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsGameplayTag(const FFlowDataPinValue_GameplayTag& GameplayTagValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	FGameplayTag Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_GameplayTag>(GameplayTagValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to GameplayTag failed on pin '%s': %s"),
			*GameplayTagValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<FGameplayTag> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsGameplayTagArray(const FFlowDataPinValue_GameplayTag& GameplayTagValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<FGameplayTag> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_GameplayTag>(GameplayTagValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to GameplayTag Array failed on pin '%s': %s"),
			*GameplayTagValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<FGameplayTag>();
	}

	return Extracted;
}

// GameplayTagContainer
FGameplayTagContainer UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsGameplayTagContainer(const FFlowDataPinValue_GameplayTagContainer& GameplayTagContainerValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	FGameplayTagContainer Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_GameplayTagContainer>(GameplayTagContainerValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to GameplayTagContainer failed on pin '%s': %s"),
			*GameplayTagContainerValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

// InstancedStruct
FInstancedStruct UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsInstancedStruct(const FFlowDataPinValue_InstancedStruct& InstancedStructValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	FInstancedStruct Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_InstancedStruct>(InstancedStructValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to InstancedStruct failed on pin '%s': %s"),
			*InstancedStructValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<FInstancedStruct> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsInstancedStructArray(const FFlowDataPinValue_InstancedStruct& InstancedStructValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<FInstancedStruct> Extracted;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_InstancedStruct>(InstancedStructValue.PropertyPinName, Extracted);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to InstancedStruct Array failed on pin '%s': %s"),
			*InstancedStructValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<FInstancedStruct>();
	}

	return Extracted;
}

// Object
UObject* UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsObject(const FFlowDataPinValue_Object& ObjectValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	TObjectPtr<UObject> Extracted = nullptr;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Object>(ObjectValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Object failed on pin '%s': %s"),
			*ObjectValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

TArray<UObject*> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsObjectArray(const FFlowDataPinValue_Object& ObjectValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<TObjectPtr<UObject>> ExtractedTemp;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Object>(ObjectValue.PropertyPinName, ExtractedTemp);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Object Array failed on pin '%s': %s"),
			*ObjectValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<UObject*>();
	}

	TArray<UObject*> Result;
	Result.Reserve(ExtractedTemp.Num());
	for (const TObjectPtr<UObject>& Obj : ExtractedTemp)
	{
		Result.Add(Obj.Get());
	}
	return Result;
}

// Class
UClass* UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsClass(const FFlowDataPinValue_Class& ClassValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	TObjectPtr<UClass> Extracted = nullptr;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValue<FFlowPinType_Class>(ClassValue.PropertyPinName, Extracted, SingleMode);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Class failed on pin '%s': %s"),
			*ClassValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	return Extracted;
}

// Class
TArray<UClass*> UFlowDataPinBlueprintLibrary::AutoConvert_TryResolveAsClassArray(const FFlowDataPinValue_Class& ClassValue, const UFlowNodeBase* Target)
{
	using namespace FlowPinType;

	TArray<TObjectPtr<UClass>> ExtractedTemp;
	EFlowDataPinResolveResult ResolveResult = EFlowDataPinResolveResult::FailedNullFlowNodeBase;

	if (IsValid(Target))
	{
		ResolveResult = Target->TryResolveDataPinValues<FFlowPinType_Class>(ClassValue.PropertyPinName, ExtractedTemp);
	}

	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("Auto-Resolve to Class Array failed on pin '%s': %s"),
			*ClassValue.PropertyPinName.ToString(),
			*UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<UClass*>();
	}

	TArray<UClass*> Result;
	Result.Reserve(ExtractedTemp.Num());
	for (const TObjectPtr<UClass>& ClassPtr : ExtractedTemp)
	{
		Result.Add(ClassPtr.Get());
	}

	return Result;
}

bool UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractBool(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	bool ExtractedValue = false;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Bool>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractBool Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<bool> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractBoolArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<bool> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Bool>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractBoolArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

int32 UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractInt(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	int32 ExtractedValue = 0;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Int>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractInt Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<int32> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractIntArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<int32> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Int>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractIntArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

int64 UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractInt64(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	int64 ExtractedValue = 0;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Int64>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractInt64 Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<int64> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractInt64Array(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<int64> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Int64>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractInt64Array Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

float UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractFloat(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	float ExtractedValue = 0.0f;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Float>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractFloat Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<float> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractFloatArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<float> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Float>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractFloatArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

double UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractDouble(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	double ExtractedValue = 0.0;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Double>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractDouble Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<double> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractDoubleArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<double> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Double>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractDoubleArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

FName UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractName(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	FName ExtractedValue = NAME_None;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Name>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractName Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<FName> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractNameArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<FName> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Name>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractNameArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

FString UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractString(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	FString ExtractedValue;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_String>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractString Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<FString> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractStringArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<FString> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_String>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractStringArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

FText UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractText(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	FText ExtractedValue;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Text>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractText Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<FText> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractTextArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<FText> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Text>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractTextArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

uint8 UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractEnum(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	FName ExtractedValueAsName;
	UEnum* EnumClass = nullptr;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Enum>(DataPinResult, ExtractedValueAsName, EnumClass, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractEnum Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}

	if (ensure(IsValid(EnumClass)))
	{
		const uint64 EnumValueAsInt = EnumClass->GetValueByName(ExtractedValueAsName);
		return static_cast<uint8>(EnumValueAsInt);
	}

	return static_cast<uint8>(INDEX_NONE);
}

TArray<uint8> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractEnumArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;

	TArray<FName> ExtractedNames;
	UEnum* EnumClass = nullptr;

	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Enum>(DataPinResult, ExtractedNames, EnumClass);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractEnumArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
		return TArray<uint8>();
	}

	if (!ensure(IsValid(EnumClass)))
	{
		return TArray<uint8>();
	}

	TArray<uint8> ExtractedValues;
	ExtractedValues.Reserve(ExtractedNames.Num());

	for (const FName& Name : ExtractedNames)
	{
		const uint64 EnumValueAsInt = EnumClass->GetValueByName(Name);
		ExtractedValues.Add(static_cast<uint8>(EnumValueAsInt));
	}

	return ExtractedValues;
}

FVector UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractVector(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	FVector ExtractedValue = FVector::ZeroVector;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Vector>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractVector Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<FVector> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractVectorArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<FVector> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Vector>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractVectorArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

FRotator UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractRotator(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	FRotator ExtractedValue = FRotator::ZeroRotator;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Rotator>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractRotator Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<FRotator> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractRotatorArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<FRotator> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Rotator>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractRotatorArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

FTransform UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractTransform(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	FTransform ExtractedValue = FTransform::Identity;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Transform>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractTransform Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<FTransform> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractTransformArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<FTransform> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Transform>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractTransformArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

FGameplayTag UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractGameplayTag(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	FGameplayTag ExtractedValue;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_GameplayTag>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractGameplayTag Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<FGameplayTag> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractGameplayTagArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<FGameplayTag> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_GameplayTag>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractGameplayTagArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

FGameplayTagContainer UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractGameplayTagContainer(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	FGameplayTagContainer ExtractedValue;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_GameplayTagContainer>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractGameplayTagContainer Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

FInstancedStruct UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractInstancedStruct(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	FInstancedStruct ExtractedValue;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_InstancedStruct>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractInstancedStruct Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<FInstancedStruct> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractInstancedStructArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<FInstancedStruct> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_InstancedStruct>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractInstancedStructArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

UObject* UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractObject(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TObjectPtr<UObject> ExtractedValue = nullptr;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Object>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractObject Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<UObject*> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractObjectArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<TObjectPtr<UObject>> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Object>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractObjectArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

UClass* UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractClass(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TObjectPtr<UClass> ExtractedValue = nullptr;
	constexpr EFlowSingleFromArray SingleMode = EFlowSingleFromArray::LastValue;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValue<FFlowPinType_Class>(DataPinResult, ExtractedValue, SingleMode);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractClass Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValue;
}

TArray<UClass*> UFlowDataPinBlueprintLibrary::AutoConvert_TryExtractClassArray(const FFlowDataPinResult& DataPinResult)
{
	using namespace FlowPinType;
	TArray<TObjectPtr<UClass>> ExtractedValues;
	const EFlowDataPinResolveResult ResolveResult = TryExtractValues<FFlowPinType_Class>(DataPinResult, ExtractedValues);
	if (!IsSuccess(ResolveResult))
	{
		UE_LOG(LogFlow, Error, TEXT("TryExtractClassArray Error: %s"), *UEnum::GetDisplayValueAsText(ResolveResult).ToString());
	}
	return ExtractedValues;
}

bool UFlowDataPinBlueprintLibrary::GetBoolValue(const FFlowDataPinValue_Bool& BoolDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (BoolDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetBoolValue on an input pin, use ResolveAsBool instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, BoolDataPinValue.Values.Num());
	if (BoolDataPinValue.Values.IsValidIndex(Index))
	{
		return BoolDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Bool Data Pin Value."));
	return false;
}

TArray<bool> UFlowDataPinBlueprintLibrary::GetBoolValues(FFlowDataPinValue_Bool& BoolDataPinValue)
{
#if WITH_EDITOR
	if (BoolDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetBoolValues on an input pin, use ResolveAsBoolArray instead."));
	}
#endif
	return BoolDataPinValue.Values;
}

// Int
int32 UFlowDataPinBlueprintLibrary::GetIntValue(const FFlowDataPinValue_Int& IntDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (IntDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetIntValue on an input pin, use ResolveAsInt instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, IntDataPinValue.Values.Num());
	if (IntDataPinValue.Values.IsValidIndex(Index))
	{
		return IntDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Int Data Pin Value."));
	return 0;
}

TArray<int32> UFlowDataPinBlueprintLibrary::GetIntValues(FFlowDataPinValue_Int& IntDataPinValue)
{
#if WITH_EDITOR
	if (IntDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetIntValues on an input pin, use ResolveAsIntArray instead."));
	}
#endif
	return IntDataPinValue.Values;
}

// Int64
int64 UFlowDataPinBlueprintLibrary::GetInt64Value(const FFlowDataPinValue_Int64& Int64DataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (Int64DataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetInt64Value on an input pin, use ResolveAsInt64 instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, Int64DataPinValue.Values.Num());
	if (Int64DataPinValue.Values.IsValidIndex(Index))
	{
		return Int64DataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Int64 Data Pin Value."));
	return 0;
}

TArray<int64> UFlowDataPinBlueprintLibrary::GetInt64Values(FFlowDataPinValue_Int64& Int64DataPinValue)
{
#if WITH_EDITOR
	if (Int64DataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetInt64Values on an input pin, use ResolveAsInt64Array instead."));
	}
#endif
	return Int64DataPinValue.Values;
}

// Float
float UFlowDataPinBlueprintLibrary::GetFloatValue(const FFlowDataPinValue_Float& FloatDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (FloatDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetFloatValue on an input pin, use ResolveAsFloat instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, FloatDataPinValue.Values.Num());
	if (FloatDataPinValue.Values.IsValidIndex(Index))
	{
		return FloatDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Float Data Pin Value."));
	return 0.f;
}

TArray<float> UFlowDataPinBlueprintLibrary::GetFloatValues(FFlowDataPinValue_Float& FloatDataPinValue)
{
#if WITH_EDITOR
	if (FloatDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetFloatValues on an input pin, use ResolveAsFloatArray instead."));
	}
#endif
	return FloatDataPinValue.Values;
}

// Double
double UFlowDataPinBlueprintLibrary::GetDoubleValue(const FFlowDataPinValue_Double& DoubleDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (DoubleDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetDoubleValue on an input pin, use ResolveAsDouble instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, DoubleDataPinValue.Values.Num());
	if (DoubleDataPinValue.Values.IsValidIndex(Index))
	{
		return DoubleDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Double Data Pin Value."));
	return 0.0;
}

TArray<double> UFlowDataPinBlueprintLibrary::GetDoubleValues(FFlowDataPinValue_Double& DoubleDataPinValue)
{
#if WITH_EDITOR
	if (DoubleDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetDoubleValues on an input pin, use ResolveAsDoubleArray instead."));
	}
#endif
	return DoubleDataPinValue.Values;
}

// Name
FName UFlowDataPinBlueprintLibrary::GetNameValue(const FFlowDataPinValue_Name& NameDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (NameDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetNameValue on an input pin, use ResolveAsName instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, NameDataPinValue.Values.Num());
	if (NameDataPinValue.Values.IsValidIndex(Index))
	{
		return NameDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Name Data Pin Value."));
	return FName();
}

TArray<FName> UFlowDataPinBlueprintLibrary::GetNameValues(FFlowDataPinValue_Name& NameDataPinValue)
{
#if WITH_EDITOR
	if (NameDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetNameValues on an input pin, use ResolveAsNameArray instead."));
	}
#endif
	return NameDataPinValue.Values;
}

// String
FString UFlowDataPinBlueprintLibrary::GetStringValue(const FFlowDataPinValue_String& StringDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (StringDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetStringValue on an input pin, use ResolveAsString instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, StringDataPinValue.Values.Num());
	if (StringDataPinValue.Values.IsValidIndex(Index))
	{
		return StringDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in String Data Pin Value."));
	return FString();
}

TArray<FString> UFlowDataPinBlueprintLibrary::GetStringValues(FFlowDataPinValue_String& StringDataPinValue)
{
#if WITH_EDITOR
	if (StringDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetStringValues on an input pin, use ResolveAsStringArray instead."));
	}
#endif
	return StringDataPinValue.Values;
}

// Text
FText UFlowDataPinBlueprintLibrary::GetTextValue(const FFlowDataPinValue_Text& TextDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (TextDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetTextValue on an input pin, use ResolveAsText instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, TextDataPinValue.Values.Num());
	if (TextDataPinValue.Values.IsValidIndex(Index))
	{
		return TextDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Text Data Pin Value."));
	return FText::GetEmpty();
}

TArray<FText> UFlowDataPinBlueprintLibrary::GetTextValues(FFlowDataPinValue_Text& TextDataPinValue)
{
#if WITH_EDITOR
	if (TextDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetTextValues on an input pin, use ResolveAsTextArray instead."));
	}
#endif
	return TextDataPinValue.Values;
}

// ----- Enum Setters -----

void UFlowDataPinBlueprintLibrary::SetEnumValue(uint8 InValue, FFlowDataPinValue_Enum& EnumDataPinValue)
{
	UEnum* EnumClass = EnumDataPinValue.EnumClass.LoadSynchronous();
	if (!IsValid(EnumClass))
	{
		UE_LOG(LogFlow, Error, TEXT("SetEnumValue: Null EnumClass"));
		return;
	}

	const int64 ValueInt64 = static_cast<int64>(InValue);
	const FName EnumValueName = EnumClass->GetNameByValue(ValueInt64);
	if (EnumValueName.IsNone())
	{
		UE_LOG(LogFlow, Error, TEXT("SetEnumValue: Could not find enum name for value %d in %s"), InValue, *EnumClass->GetPathName());
		return;
	}

	EnumDataPinValue.EnumClass = EnumClass;
	EnumDataPinValue.Values = { EnumValueName };
#if WITH_EDITOR
	EnumDataPinValue.MultiType = EFlowDataMultiType::Single;
#endif
}

void UFlowDataPinBlueprintLibrary::SetEnumValues(const TArray<uint8>& InValues, FFlowDataPinValue_Enum& EnumDataPinValue)
{
	UEnum* EnumClass = EnumDataPinValue.EnumClass.LoadSynchronous();
	if (!IsValid(EnumClass))
	{
		UE_LOG(LogFlow, Error, TEXT("SetEnumValues: Null EnumClass"));
		return;
	}

	TArray<FName> Names;
	Names.Reserve(InValues.Num());

	for (uint8 RawValue : InValues)
	{
		const int64 ValueInt64 = static_cast<int64>(RawValue);
		const FName EnumValueName = EnumClass->GetNameByValue(ValueInt64);
		if (EnumValueName.IsNone())
		{
			UE_LOG(LogFlow, Error, TEXT("SetEnumValues: Could not find enum name for value %d in %s"), RawValue, *EnumClass->GetPathName());

			// Abort entire set to avoid partial data
			return; 
		}

		Names.Add(EnumValueName);
	}

	EnumDataPinValue.EnumClass = EnumClass;
	EnumDataPinValue.Values = MoveTemp(Names);
#if WITH_EDITOR
	EnumDataPinValue.MultiType = EFlowDataMultiType::Array;
#endif
}

uint8 UFlowDataPinBlueprintLibrary::GetEnumValue(const FFlowDataPinValue_Enum& EnumDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (EnumDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetEnumValue on an input pin, use ResolveAsEnum instead."));
	}
#endif

	UEnum* EnumClass = EnumDataPinValue.EnumClass.LoadSynchronous();
	if (!IsValid(EnumClass))
	{
		UE_LOG(LogFlow, Error, TEXT("GetEnumValue: Null EnumClass"));
		return 0;
	}

	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, EnumDataPinValue.Values.Num());
	if (!EnumDataPinValue.Values.IsValidIndex(Index))
	{
		UE_LOG(LogFlow, Error, TEXT("GetEnumValue: Insufficient values."));
		return 0;
	}

	const FName& EnumName = EnumDataPinValue.Values[Index];
	const int32 EnumIndex = EnumClass->GetIndexByName(EnumName);
	if (EnumIndex == INDEX_NONE)
	{
		UE_LOG(LogFlow, Error, TEXT("GetEnumValue: Name '%s' not found in enum %s"), *EnumName.ToString(), *EnumClass->GetPathName());
		return 0;
	}

	const int64 RawValue = EnumClass->GetValueByIndex(EnumIndex);
	return static_cast<uint8>(RawValue);
}

TArray<uint8> UFlowDataPinBlueprintLibrary::GetEnumValues(const FFlowDataPinValue_Enum& EnumDataPinValue)
{
#if WITH_EDITOR
	if (EnumDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetEnumValues on an input pin, use ResolveAsEnumArray instead."));
	}
#endif
	TArray<uint8> Values;

	UEnum* EnumClass = EnumDataPinValue.EnumClass.LoadSynchronous();
	if (!IsValid(EnumClass))
	{
		UE_LOG(LogFlow, Error, TEXT("GetEnumValues: Null EnumClass"));
		return Values;
	}

	Values.Reserve(EnumDataPinValue.Values.Num());
	for (const FName& EnumName : EnumDataPinValue.Values)
	{
		const int32 EnumIndex = EnumClass->GetIndexByName(EnumName);
		if (EnumIndex == INDEX_NONE)
		{
			UE_LOG(LogFlow, Error, TEXT("GetEnumValues: Name '%s' not found in enum %s"), *EnumName.ToString(), *EnumClass->GetPathName());

			// Abort entire set to avoid partial data
			return TArray<uint8>();
		}
		const int64 RawValue = EnumClass->GetValueByIndex(EnumIndex);
		Values.Add(static_cast<uint8>(RawValue));
	}

	return Values;
}

// Vector
FVector UFlowDataPinBlueprintLibrary::GetVectorValue(const FFlowDataPinValue_Vector& VectorDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (VectorDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetVectorValue on an input pin, use ResolveAsVector instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, VectorDataPinValue.Values.Num());
	if (VectorDataPinValue.Values.IsValidIndex(Index))
	{
		return VectorDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Vector Data Pin Value."));
	return FVector::ZeroVector;
}

TArray<FVector> UFlowDataPinBlueprintLibrary::GetVectorValues(FFlowDataPinValue_Vector& VectorDataPinValue)
{
#if WITH_EDITOR
	if (VectorDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetVectorValues on an input pin, use ResolveAsVectorArray instead."));
	}
#endif
	return VectorDataPinValue.Values;
}

// Rotator
FRotator UFlowDataPinBlueprintLibrary::GetRotatorValue(const FFlowDataPinValue_Rotator& RotatorDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (RotatorDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetRotatorValue on an input pin, use ResolveAsRotator instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, RotatorDataPinValue.Values.Num());
	if (RotatorDataPinValue.Values.IsValidIndex(Index))
	{
		return RotatorDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Rotator Data Pin Value."));
	return FRotator::ZeroRotator;
}

TArray<FRotator> UFlowDataPinBlueprintLibrary::GetRotatorValues(FFlowDataPinValue_Rotator& RotatorDataPinValue)
{
#if WITH_EDITOR
	if (RotatorDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetRotatorValues on an input pin, use ResolveAsRotatorArray instead."));
	}
#endif
	return RotatorDataPinValue.Values;
}

// Transform
FTransform UFlowDataPinBlueprintLibrary::GetTransformValue(const FFlowDataPinValue_Transform& TransformDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (TransformDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetTransformValue on an input pin, use ResolveAsTransform instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, TransformDataPinValue.Values.Num());
	if (TransformDataPinValue.Values.IsValidIndex(Index))
	{
		return TransformDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Transform Data Pin Value."));
	return FTransform::Identity;
}

TArray<FTransform> UFlowDataPinBlueprintLibrary::GetTransformValues(FFlowDataPinValue_Transform& TransformDataPinValue)
{
#if WITH_EDITOR
	if (TransformDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetTransformValues on an input pin, use ResolveAsTransformArray instead."));
	}
#endif
	return TransformDataPinValue.Values;
}

// GameplayTag
FGameplayTag UFlowDataPinBlueprintLibrary::GetGameplayTagValue(const FFlowDataPinValue_GameplayTag& GameplayTagDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (GameplayTagDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetGameplayTagValue on an input pin, use ResolveAsGameplayTag instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, GameplayTagDataPinValue.Values.Num());
	if (GameplayTagDataPinValue.Values.IsValidIndex(Index))
	{
		return GameplayTagDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in GameplayTag Data Pin Value."));
	return FGameplayTag();
}

TArray<FGameplayTag> UFlowDataPinBlueprintLibrary::GetGameplayTagValues(FFlowDataPinValue_GameplayTag& GameplayTagDataPinValue)
{
#if WITH_EDITOR
	if (GameplayTagDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetGameplayTagValues on an input pin, use ResolveAsGameplayTagArray instead."));
	}
#endif
	return GameplayTagDataPinValue.Values;
}

// GameplayTagContainer (scalar only)
FGameplayTagContainer UFlowDataPinBlueprintLibrary::GetGameplayTagContainerValue(const FFlowDataPinValue_GameplayTagContainer& GameplayTagContainerDataPinValue)
{
#if WITH_EDITOR
	if (GameplayTagContainerDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetGameplayTagContainerValue on an input pin, use ResolveAsGameplayTagContainer instead."));
	}
#endif
	return GameplayTagContainerDataPinValue.Values;
}

// InstancedStruct
FInstancedStruct UFlowDataPinBlueprintLibrary::GetInstancedStructValue(const FFlowDataPinValue_InstancedStruct& InstancedStructDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (InstancedStructDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetInstancedStructValue on an input pin, use ResolveAsInstancedStruct instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, InstancedStructDataPinValue.Values.Num());
	if (InstancedStructDataPinValue.Values.IsValidIndex(Index))
	{
		return InstancedStructDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in InstancedStruct Data Pin Value."));
	return FInstancedStruct();
}

TArray<FInstancedStruct> UFlowDataPinBlueprintLibrary::GetInstancedStructValues(FFlowDataPinValue_InstancedStruct& InstancedStructDataPinValue)
{
#if WITH_EDITOR
	if (InstancedStructDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetInstancedStructValues on an input pin, use ResolveAsInstancedStructArray instead."));
	}
#endif
	return InstancedStructDataPinValue.Values;
}

// Object
UObject* UFlowDataPinBlueprintLibrary::GetObjectValue(const FFlowDataPinValue_Object& ObjectDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (ObjectDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetObjectValue on an input pin, use ResolveAsObject instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, ObjectDataPinValue.Values.Num());
	if (ObjectDataPinValue.Values.IsValidIndex(Index))
	{
		return ObjectDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Object Data Pin Value."));
	return nullptr;
}

TArray<UObject*> UFlowDataPinBlueprintLibrary::GetObjectValues(FFlowDataPinValue_Object& ObjectDataPinValue)
{
#if WITH_EDITOR
	if (ObjectDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetObjectValues on an input pin, use ResolveAsObjectArray instead."));
	}
#endif
	return ObjectDataPinValue.Values;
}

// Class
FSoftClassPath UFlowDataPinBlueprintLibrary::GetClassValue(const FFlowDataPinValue_Class& ClassDataPinValue, EFlowSingleFromArray SingleFromArray)
{
#if WITH_EDITOR
	if (ClassDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetClassValue on an input pin, use ResolveAsClass instead."));
	}
#endif
	const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, ClassDataPinValue.Values.Num());
	if (ClassDataPinValue.Values.IsValidIndex(Index))
	{
		return ClassDataPinValue.Values[Index];
	}
	UE_LOG(LogFlow, Error, TEXT("Insufficient values in Class Data Pin Value."));
	return FSoftClassPath();
}

TArray<FSoftClassPath> UFlowDataPinBlueprintLibrary::GetClassValues(FFlowDataPinValue_Class& ClassDataPinValue)
{
#if WITH_EDITOR
	if (ClassDataPinValue.bIsInputPin)
	{
		UE_LOG(LogFlow, Error, TEXT("Should not call GetClassValues on an input pin, use ResolveAsClassArray instead."));
	}
#endif
	return ClassDataPinValue.Values;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Bool(const TArray<bool>& BoolValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Bool ValueStruct;
	ValueStruct.Values = BoolValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Int(const TArray<int32>& IntValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Int ValueStruct;
	ValueStruct.Values = IntValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Int64(const TArray<int64>& Int64Values)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Int64 ValueStruct;
	ValueStruct.Values = Int64Values;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Float(const TArray<float>& FloatValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Float ValueStruct;
	ValueStruct.Values = FloatValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Double(const TArray<double>& DoubleValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Double ValueStruct;
	ValueStruct.Values = DoubleValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Name(const TArray<FName>& NameValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Name ValueStruct;
	ValueStruct.Values = NameValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_String(const TArray<FString>& StringValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_String ValueStruct;
	ValueStruct.Values = StringValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Text(const TArray<FText>& TextValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Text ValueStruct;
	ValueStruct.Values = TextValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Enum(const TArray<uint8>& EnumValues, UEnum* EnumClass)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	if (EnumClass)
	{
		FFlowDataPinValue_Enum ValueStruct;
		ValueStruct.EnumClass = EnumClass;
		ValueStruct.Values.Reserve(EnumValues.Num());

		for (uint8 RawValue : EnumValues)
		{
			const FName EnumName = EnumClass->GetNameByValue(RawValue);
			ValueStruct.Values.Add(EnumName);
		}

		Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);
	}

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Vector(const TArray<FVector>& VectorValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Vector ValueStruct;
	ValueStruct.Values = VectorValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Rotator(const TArray<FRotator>& RotatorValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Rotator ValueStruct;
	ValueStruct.Values = RotatorValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Transform(const TArray<FTransform>& TransformValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Transform ValueStruct;
	ValueStruct.Values = TransformValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_GameplayTag(const TArray<FGameplayTag>& GameplayTagValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_GameplayTag ValueStruct;
	ValueStruct.Values = GameplayTagValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_GameplayTagContainer(FGameplayTagContainer GameplayTagContainerValue)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_GameplayTagContainer ValueStruct;
	ValueStruct.Values = GameplayTagContainerValue;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_InstancedStruct(const TArray<FInstancedStruct>& InstancedStructValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_InstancedStruct ValueStruct;
	ValueStruct.Values = InstancedStructValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Object(const TArray<UObject*>& ObjectValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Object ValueStruct;
	ValueStruct.Values = ObjectValues;
	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);

	return Out;
}

FFlowDataPinResult UFlowDataPinBlueprintLibrary::MakeFlowDataPinResult_Class(const TArray<FSoftClassPath>& ClassValues)
{
	FFlowDataPinResult Out(EFlowDataPinResolveResult::Success);

	FFlowDataPinValue_Class ValueStruct;
	ValueStruct.Values = ClassValues;

	Out.ResultValue = TInstancedStruct<FFlowDataPinValue>::Make(ValueStruct);
	return Out;
}