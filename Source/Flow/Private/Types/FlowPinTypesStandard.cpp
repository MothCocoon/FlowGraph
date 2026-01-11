// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowPinTypesStandard.h"
#include "Types/FlowClassUtils.h"
#include "Nodes/FlowNode.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "Types/FlowDataPinResults.h"
#include "Types/FlowPinTypeTemplates.h"
#include "Types/FlowPinTypeNodeTemplates.h"
#include "FlowLogChannels.h"

#if WITH_EDITOR
#include "EditorClassUtils.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowPinTypesStandard)

// NOTE (gtaylor) Beware static initialization order if attempting to use these in static initialization.
// Instead, consider sourcing directly from the FFlowPinTypeNamesStandard's TCHAR form in those cases.
const FFlowPinTypeName FFlowPinType_Exec::PinTypeNameExec = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameExec);
const FFlowPinTypeName FFlowPinType_Bool::PinTypeNameBool = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameBool);
const FFlowPinTypeName FFlowPinType_Int::PinTypeNameInt = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameInt);
const FFlowPinTypeName FFlowPinType_Int64::PinTypeNameInt64 = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameInt64);
const FFlowPinTypeName FFlowPinType_Float::PinTypeNameFloat = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameFloat);
const FFlowPinTypeName FFlowPinType_Double::PinTypeNameDouble = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameDouble);
const FFlowPinTypeName FFlowPinType_Enum::PinTypeNameEnum = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameEnum);
const FFlowPinTypeName FFlowPinType_Name::PinTypeNameName = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameName);
const FFlowPinTypeName FFlowPinType_String::PinTypeNameString = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameString);
const FFlowPinTypeName FFlowPinType_Text::PinTypeNameText = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameText);
const FFlowPinTypeName FFlowPinType_Vector::PinTypeNameVector = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameVector);
const FFlowPinTypeName FFlowPinType_Rotator::PinTypeNameRotator = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameRotator);
const FFlowPinTypeName FFlowPinType_Transform::PinTypeNameTransform = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameTransform);
const FFlowPinTypeName FFlowPinType_GameplayTag::PinTypeNameGameplayTag = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameGameplayTag);
const FFlowPinTypeName FFlowPinType_GameplayTagContainer::PinTypeNameGameplayTagContainer = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameGameplayTagContainer);
const FFlowPinTypeName FFlowPinType_InstancedStruct::PinTypeNameInstancedStruct = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameInstancedStruct);
const FFlowPinTypeName FFlowPinType_Object::PinTypeNameObject = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameObject);
const FFlowPinTypeName FFlowPinType_Class::PinTypeNameClass = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameClass);

bool FFlowPinType_Bool::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Bool>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Int::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Int>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Int64::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Int64>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Float::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Float>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Double::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Double>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Enum::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	using TFlowPinType = FFlowPinType_Enum;
	using TValue = TFlowPinType::ValueType;
	using TWrapper = TFlowPinType::WrapperType;
	using Traits = FlowPinType::FFlowDataPinValueTraits<TFlowPinType>;

	TInstancedStruct<FFlowDataPinValue> ValueStruct;
	const FProperty* FoundProperty = nullptr;

	if (!Node.TryFindPropertyByPinName(PropertyOwnerObject, Pin.PinName, FoundProperty, ValueStruct))
	{
		OutResult.Result = EFlowDataPinResolveResult::FailedUnknownPin;
		return false;
	}

	if (ValueStruct.IsValid() && ValueStruct.Get<FFlowDataPinValue>().GetPinTypeName() == TFlowPinType::GetPinTypeNameStatic())
	{
		OutResult.ResultValue = ValueStruct;
		OutResult.Result = EFlowDataPinResolveResult::Success;
		return true;
	}

	TArray<TValue> Values;
	TSoftObjectPtr<UEnum> EnumClass;
	if (FlowPinType::IsSuccess(Traits::ExtractFromProperty(FoundProperty, &PropertyOwnerObject, Values, EnumClass)))
	{
		OutResult.ResultValue = TInstancedStruct<TWrapper>::Make(EnumClass, Values);
		OutResult.Result = EFlowDataPinResolveResult::Success;
		return true;
	}

	OutResult.Result = EFlowDataPinResolveResult::FailedMismatchedType;
	return false;
}

bool FFlowPinType_Name::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Name>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_String::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_String>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Text::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Text>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Vector::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Vector>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Rotator::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Rotator>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Transform::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Transform>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_GameplayTag::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_GameplayTag>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_GameplayTagContainer::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_GameplayTagContainer>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_InstancedStruct::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_InstancedStruct>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Object::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Object>(PropertyOwnerObject, Node, Pin, OutResult);
}

bool FFlowPinType_Class::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	return FlowPinType::PopulateResultTemplate<FFlowPinType_Class>(PropertyOwnerObject, Node, Pin, OutResult);
}

#if WITH_EDITOR

UObject* FFlowPinType_Vector::GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const
{
	static UObject* PinSubCategoryObject = TBaseStructure<FVector>::Get();
	return PinSubCategoryObject;
}

UObject* FFlowPinType_Rotator::GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const
{
	static UObject* PinSubCategoryObject = TBaseStructure<FRotator>::Get();
	return PinSubCategoryObject;
}

UObject* FFlowPinType_Transform::GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const
{
	static UObject* PinSubCategoryObject = TBaseStructure<FTransform>::Get();
	return PinSubCategoryObject;
}

UObject* FFlowPinType_GameplayTag::GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const
{
	static UObject* PinSubCategoryObject = TBaseStructure<FGameplayTag>::Get();
	return PinSubCategoryObject;
}

UObject* FFlowPinType_GameplayTagContainer::GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const
{
	static UObject* PinSubCategoryObject = TBaseStructure<FGameplayTagContainer>::Get();
	return PinSubCategoryObject;
}

UObject* FFlowPinType_InstancedStruct::GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const
{
	static UObject* PinSubCategoryObject = TBaseStructure<FInstancedStruct>::Get();
	return PinSubCategoryObject;
}

UObject* FFlowPinType_Enum::GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const
{
	UEnum* EnumClass = nullptr;
	if (Wrapper && Wrapper->GetPinTypeName() == FFlowPinType_Enum::GetPinTypeNameStatic())
	{
		const FFlowDataPinValue_Enum* EnumWrapper = static_cast<const FFlowDataPinValue_Enum*>(Wrapper);
		TSoftObjectPtr<UEnum> EnumClassPtr = EnumWrapper->EnumClass;
		EnumClass = EnumClassPtr.LoadSynchronous();
	}
	else if (Property)
	{
		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			const UStruct* ScriptStruct = FFlowDataPinValue_Enum::StaticStruct();
			if (StructProperty->Struct == ScriptStruct)
			{
				FFlowDataPinValue_Enum ValueStruct;
				StructProperty->GetValue_InContainer(InContainer, &ValueStruct);
				EnumClass = ValueStruct.EnumClass.LoadSynchronous();
			}
		}
		else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			EnumClass = EnumProperty->GetEnum();
		}
		else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			if (ByteProperty->Enum)
			{
				EnumClass = ByteProperty->Enum;
			}
		}
		else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			if (const FEnumProperty* InnerEnumProperty = CastField<FEnumProperty>(ArrayProperty->Inner))
			{
				EnumClass = InnerEnumProperty->GetEnum();
			}
			else if (const FByteProperty* InnerByteProperty = CastField<FByteProperty>(ArrayProperty->Inner))
			{
				if (InnerByteProperty->Enum)
				{
					EnumClass = InnerByteProperty->Enum;
				}
			}
		}
	}

	return EnumClass;
}

UClass* FFlowPinType_Object::TryGetObjectClassFromProperty(const FProperty& MetaDataProperty)
{
	if (UClass* MetaClass = FFlowPinType_Object::TryGetMetaClassFromProperty(MetaDataProperty))
	{
		return MetaClass;
	}

	// FSoftObjectPath can use the "AllowedClasses" to define what classes are allowed for the object.
	// Using the "AllowedClasses" metadata tag, but we only support a single class, due to singular return value for this function.
	const FString AllowedClassesString = MetaDataProperty.GetMetaData("AllowedClasses");
	const TArray<UClass*> AllowedClasses = FlowClassUtils::GetClassesFromMetadataString(AllowedClassesString);

	if (AllowedClasses.Num() > 1)
	{
		UE_LOG(LogFlow, Error, TEXT("Only a single AllowedClasses entry is allowed for flow data pin properties (multiple found: %s) for property %s"), *AllowedClassesString, *MetaDataProperty.GetName());

		return nullptr;
	}

	if (AllowedClasses.IsEmpty())
	{
		return nullptr;
	}

	if (UClass* AllowedClass = AllowedClasses[0])
	{
		return AllowedClass;
	}
	else
	{
		UE_LOG(LogFlow, Error, TEXT("Could not resolve AllowedClasses '%s' for property %s"), *AllowedClassesString, *MetaDataProperty.GetName());
	}

	return nullptr;
}

UClass* FFlowPinType_Object::TryGetMetaClassFromProperty(const FProperty& MetaDataProperty)
{
	const FString& MetaClassName = MetaDataProperty.GetMetaData("MetaClass");

	if (!MetaClassName.IsEmpty())
	{
		if (UClass* FoundClass = FEditorClassUtils::GetClassFromString(MetaClassName))
		{
			return FoundClass;
		}
		else
		{
			UE_LOG(LogFlow, Error, TEXT("Could not resolve MetaClass named %s for property %s"), *MetaClassName, *MetaDataProperty.GetName());
		}
	}

	return nullptr;
}

UObject* FFlowPinType_Object::GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const
{
	UClass* Class = nullptr;
	if (Wrapper && Wrapper->GetPinTypeName() == FFlowPinType_Object::GetPinTypeNameStatic())
	{
		const FFlowDataPinValue_Object* ObjectWrapper = static_cast<const FFlowDataPinValue_Object*>(Wrapper);
		Class = ObjectWrapper->ClassFilter;
	}
	else if (Property)
	{
		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			const UStruct* ScriptStruct = FFlowDataPinValue_Object::StaticStruct();
			static const UStruct* SoftObjectPathStruct = TBaseStructure<FSoftObjectPath>::Get();
			if (StructProperty->Struct == ScriptStruct)
			{
				FFlowDataPinValue_Object ValueStruct;
				StructProperty->GetValue_InContainer(InContainer, &ValueStruct);
				Class = ValueStruct.ClassFilter;
			}
			else if (StructProperty->Struct == SoftObjectPathStruct)
			{
				Class = FFlowPinType_Object::TryGetObjectClassFromProperty(*StructProperty);
			}
		}
		else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			Class = ObjectProperty->PropertyClass;
		}
		else if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
		{
			Class = SoftObjectProperty->PropertyClass;
		}
		else if (const FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Property))
		{
			Class = WeakObjectProperty->PropertyClass;
		}
		else if (const FLazyObjectProperty* LazyObjectProperty = CastField<FLazyObjectProperty>(Property))
		{
			Class = LazyObjectProperty->PropertyClass;
		}
		else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			if (const FObjectProperty* InnerObjectProperty = CastField<FObjectProperty>(ArrayProperty->Inner))
			{
				Class = InnerObjectProperty->PropertyClass;
			}
			else if (const FSoftObjectProperty* InnerSoftObjectProperty = CastField<FSoftObjectProperty>(ArrayProperty->Inner))
			{
				Class = InnerSoftObjectProperty->PropertyClass;
			}
			else if (const FWeakObjectProperty* InnerWeakObjectProperty = CastField<FWeakObjectProperty>(ArrayProperty->Inner))
			{
				Class = InnerWeakObjectProperty->PropertyClass;
			}
			else if (const FLazyObjectProperty* InnerLazyObjectProperty = CastField<FLazyObjectProperty>(ArrayProperty->Inner))
			{
				Class = InnerLazyObjectProperty->PropertyClass;
			}
		}
	}

	return Class;
}

UObject* FFlowPinType_Class::GetPinSubCategoryObjectFromProperty(const FProperty* Property, void const* InContainer, const FFlowDataPinValue* Wrapper) const
{
	UClass* Class = nullptr;
	if (Wrapper && Wrapper->GetPinTypeName() == FFlowPinType_Class::GetPinTypeNameStatic())
	{
		const FFlowDataPinValue_Class* ClassWrapper = static_cast<const FFlowDataPinValue_Class*>(Wrapper);
		Class = ClassWrapper->ClassFilter;
	}
	else if (Property)
	{
		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			const UStruct* ScriptStruct = FFlowDataPinValue_Class::StaticStruct();
			static const UStruct* SoftClassPathStruct = TBaseStructure<FSoftClassPath>::Get();
			if (StructProperty->Struct == ScriptStruct)
			{
				FFlowDataPinValue_Class ValueStruct;
				StructProperty->GetValue_InContainer(InContainer, &ValueStruct);
				Class = ValueStruct.ClassFilter;
			}
			else if (StructProperty->Struct == SoftClassPathStruct)
			{
				Class = FFlowPinType_Object::TryGetMetaClassFromProperty(*StructProperty);
			}
		}
		else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
		{
			Class = ClassProperty->MetaClass;
		}
		else if (const FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
		{
			Class = SoftClassProperty->MetaClass;
		}
		else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			if (const FClassProperty* InnerClassProperty = CastField<FClassProperty>(ArrayProperty->Inner))
			{
				Class = InnerClassProperty->MetaClass;
			}
			else if (const FSoftClassProperty* InnerSoftClassProperty = CastField<FSoftClassProperty>(ArrayProperty->Inner))
			{
				Class = InnerSoftClassProperty->MetaClass;
			}
		}
	}

	return Class;
}

bool FFlowPinType_Exec::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	OutValue = FFormatArgumentValue(FText::FromString(TEXT("Exec")));
	return true;
}

bool FFlowPinType_Bool::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Bool>(Node, PinName, OutValue, [](const bool& Value) { return Value ? TEXT("true") : TEXT("false"); });
}

bool FFlowPinType_Int::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Int>(Node, PinName, OutValue, [](const int32& Value) { return FString::FromInt(Value); });
}

bool FFlowPinType_Int64::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Int64>(Node, PinName, OutValue, [](const int64& Value) { return FString::Printf(TEXT("%lld"), Value); });
}

bool FFlowPinType_Float::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Float>(Node, PinName, OutValue, [](const float& Value) { return FString::SanitizeFloat(Value); });
}

bool FFlowPinType_Double::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Double>(Node, PinName, OutValue, [](const double& Value) { return FString::SanitizeFloat(Value); });
}

bool FFlowPinType_Enum::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Enum>(Node, PinName, OutValue, [](const FName& Value) { return Value.ToString(); });
}

bool FFlowPinType_Name::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Name>(Node, PinName, OutValue, [](const FName& Value) { return Value.ToString(); });
}

bool FFlowPinType_String::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_String>(Node, PinName, OutValue, [](const FString& Value) { return Value; });
}

bool FFlowPinType_Text::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Text>(Node, PinName, OutValue, [](const FText& Value) { return Value.ToString(); });
}

bool FFlowPinType_Vector::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Vector>(Node, PinName, OutValue, [](const FVector& Value) { return Value.ToString(); });
}

bool FFlowPinType_Rotator::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Rotator>(Node, PinName, OutValue, [](const FRotator& Value) { return Value.ToString(); });
}

bool FFlowPinType_Transform::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Transform>(Node, PinName, OutValue, [](const FTransform& Value) { return Value.ToString(); });
}

bool FFlowPinType_GameplayTag::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_GameplayTag>(Node, PinName, OutValue, [](const FGameplayTag& Value) { return Value.ToString(); });
}

bool FFlowPinType_GameplayTagContainer::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_GameplayTagContainer>(Node, PinName, OutValue, [](const FGameplayTagContainer& Value) { return Value.ToString(); });
}

bool FFlowPinType_InstancedStruct::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_InstancedStruct>(Node, PinName, OutValue, [](const FInstancedStruct& Value) { return Value.GetScriptStruct() ? Value.GetScriptStruct()->GetName() : TEXT("None"); });
}

bool FFlowPinType_Object::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Object>(Node, PinName, OutValue, [](const UObject* Value) { return Value ? Value->GetName() : TEXT("None"); });
}

bool FFlowPinType_Class::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return FlowPinType::ResolveAndFormatArray<FFlowPinType_Class>(Node, PinName, OutValue, [](const UClass* Value) { return Value ? Value->GetName() : TEXT("None"); });
}
#endif