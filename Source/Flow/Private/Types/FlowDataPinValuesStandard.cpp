// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowDataPinValuesStandard.h"
#include "Nodes/FlowPin.h"
#include "Types/FlowArray.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDataPinValuesStandard)

//======================================================================
// Bool
//======================================================================
FFlowDataPinValue_Bool::FFlowDataPinValue_Bool(ValueType InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Bool::FFlowDataPinValue_Bool(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_Bool::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](ValueType b)
		{
			return b ? TEXT("true") : TEXT("false");
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// Int (int32)
//======================================================================
FFlowDataPinValue_Int::FFlowDataPinValue_Int(ValueType InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Int::FFlowDataPinValue_Int(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_Int::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](ValueType Val)
		{
			return FString::FromInt(Val);
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// Int64
//======================================================================
FFlowDataPinValue_Int64::FFlowDataPinValue_Int64(ValueType InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Int64::FFlowDataPinValue_Int64(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_Int64::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](ValueType Val)
		{
			return FString::Printf(TEXT("%lld"), Val);
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// Float
//======================================================================
FFlowDataPinValue_Float::FFlowDataPinValue_Float(ValueType InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Float::FFlowDataPinValue_Float(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_Float::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](ValueType Val)
		{
			return FString::SanitizeFloat(Val);
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// Double
//======================================================================
FFlowDataPinValue_Double::FFlowDataPinValue_Double(ValueType InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Double::FFlowDataPinValue_Double(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_Double::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](ValueType Val)
		{
			return FString::SanitizeFloat(Val);
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// Name
//======================================================================
FFlowDataPinValue_Name::FFlowDataPinValue_Name(const ValueType& InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Name::FFlowDataPinValue_Name(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_Name::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](const ValueType& Val)
		{
			return Val.ToString();
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// String
//======================================================================
FFlowDataPinValue_String::FFlowDataPinValue_String(const ValueType& InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_String::FFlowDataPinValue_String(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_String::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](const ValueType& Val)
		{
			return Val;
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// Text
//======================================================================
FFlowDataPinValue_Text::FFlowDataPinValue_Text(const ValueType& InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Text::FFlowDataPinValue_Text(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_Text::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](const ValueType& Val)
		{
			return Val.ToString();
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// Enum
//======================================================================
FFlowDataPinValue_Enum::FFlowDataPinValue_Enum(const TSoftObjectPtr<UEnum>& InEnumClass, const ValueType& InValue)
	: Values({ InValue }), EnumClass(InEnumClass)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Enum::FFlowDataPinValue_Enum(const TSoftObjectPtr<UEnum>& InEnumClass, const TArray<ValueType>& InValues)
	: Values(InValues), EnumClass(InEnumClass)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

#if WITH_EDITOR
void FFlowDataPinValue_Enum::OnEnumNameChanged()
{
	if (!EnumName.IsEmpty())
	{
		EnumClass = UClass::TryFindTypeSlow<UEnum>(EnumName, EFindFirstObjectOptions::ExactClass);

		if (EnumClass != nullptr && !FFlowPin::ValidateEnum(*EnumClass))
		{
			EnumClass = nullptr;
		}
	}
}
#endif

UField* FFlowDataPinValue_Enum::GetFieldType() const
{
	return EnumClass.LoadSynchronous();
}

bool FFlowDataPinValue_Enum::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](const ValueType& Val) { return Val.ToString(); },
		StringArraySeparator);
	return true;
}

//======================================================================
// Vector
//======================================================================
FFlowDataPinValue_Vector::FFlowDataPinValue_Vector(const ValueType& InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Vector::FFlowDataPinValue_Vector(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_Vector::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](const ValueType& V)
		{
			return V.ToCompactString();
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// Rotator
//======================================================================
FFlowDataPinValue_Rotator::FFlowDataPinValue_Rotator(const ValueType& InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Rotator::FFlowDataPinValue_Rotator(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_Rotator::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](const ValueType& R)
		{
			return R.ToCompactString();
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// Transform
//======================================================================
FFlowDataPinValue_Transform::FFlowDataPinValue_Transform(const ValueType& InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Transform::FFlowDataPinValue_Transform(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_Transform::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](const ValueType& T)
		{
			return T.ToString();
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// GameplayTag
//======================================================================
FFlowDataPinValue_GameplayTag::FFlowDataPinValue_GameplayTag(const ValueType& InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_GameplayTag::FFlowDataPinValue_GameplayTag(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

bool FFlowDataPinValue_GameplayTag::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](const ValueType& Tag)
		{
			return Tag.ToString();
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// GameplayTagContainer
//======================================================================
FFlowDataPinValue_GameplayTagContainer::FFlowDataPinValue_GameplayTagContainer(const FGameplayTag& InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_GameplayTagContainer::FFlowDataPinValue_GameplayTagContainer(const FGameplayTagContainer& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_GameplayTagContainer::FFlowDataPinValue_GameplayTagContainer(const TArray<FGameplayTag>& InValues)
	: Values(FGameplayTagContainer::CreateFromArray(InValues))
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

FFlowDataPinValue_GameplayTagContainer::FFlowDataPinValue_GameplayTagContainer(const TArray<FGameplayTagContainer>& InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif

	for (const FGameplayTagContainer& Container : InValues)
	{
		Values.AppendTags(Container);
	}
}

bool FFlowDataPinValue_GameplayTagContainer::TryConvertValuesToString(FString& OutString) const
{
	OutString = Values.ToStringSimple();
	return true;
}

//======================================================================
// InstancedStruct
//======================================================================
FFlowDataPinValue_InstancedStruct::FFlowDataPinValue_InstancedStruct(const ValueType& InValue)
	: Values({ InValue })
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_InstancedStruct::FFlowDataPinValue_InstancedStruct(const TArray<ValueType>& InValues)
	: Values(InValues)
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

//======================================================================
// Object
//======================================================================
FFlowDataPinValue_Object::FFlowDataPinValue_Object(TObjectPtr<UObject> InObject, UClass* InClassFilter)
#if WITH_EDITOR
	: ClassFilter(InClassFilter)
#endif
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
	Values = { InObject };
}

FFlowDataPinValue_Object::FFlowDataPinValue_Object(const TArray<TObjectPtr<UObject>>& InObjects, UClass* InClassFilter)
#if WITH_EDITOR
	: ClassFilter(InClassFilter)
#endif
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
	Values = InObjects;
}

FFlowDataPinValue_Object::FFlowDataPinValue_Object(AActor* InActor, UClass* InClassFilter)
#if WITH_EDITOR
	: ClassFilter(InClassFilter ? InClassFilter : AActor::StaticClass())
#endif
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
	Values = { InActor };
}

FFlowDataPinValue_Object::FFlowDataPinValue_Object(const TArray<AActor*>& InActors, UClass* InClassFilter)
#if WITH_EDITOR
	: ClassFilter(InClassFilter ? InClassFilter : AActor::StaticClass())
#endif
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif

	Values.Reset();
	Values.Reserve(InActors.Num());
	for (AActor* Actor : InActors)
	{
		Values.Add(Cast<UObject>(Actor));
	}
}

bool FFlowDataPinValue_Object::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<ValueType>(Values,
		[](UObject* Obj) 
		{ 
			return Obj ? Obj->GetName() : TEXT("None");
		},
		StringArraySeparator);
	return true;
}

//======================================================================
// Class
//======================================================================
FFlowDataPinValue_Class::FFlowDataPinValue_Class(const FSoftClassPath& InPath, UClass* InClassFilter)
	: Values({ InPath })
#if WITH_EDITOR
	, ClassFilter(InClassFilter)
#endif
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Class::FFlowDataPinValue_Class(const TArray<FSoftClassPath>& InPaths, UClass* InClassFilter)
	: Values(InPaths)
#if WITH_EDITOR
	, ClassFilter(InClassFilter)
#endif
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif
}

FFlowDataPinValue_Class::FFlowDataPinValue_Class(const UClass* InClass, UClass* InClassFilter)
	: Values({ FSoftClassPath(InClass) })
#if WITH_EDITOR
	, ClassFilter(InClassFilter)
#endif
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Single;
#endif
}

FFlowDataPinValue_Class::FFlowDataPinValue_Class(const TArray<UClass*>& InClasses, UClass* InClassFilter)
#if WITH_EDITOR
	: ClassFilter(InClassFilter)
#endif
{
#if WITH_EDITOR
	MultiType = EFlowDataMultiType::Array;
#endif

	Values.Reset();
	Values.Reserve(InClasses.Num());
	for (UClass* Class : InClasses)
	{
		Values.Add(FSoftClassPath(Class));
	}
}

bool FFlowDataPinValue_Class::TryConvertValuesToString(FString& OutString) const
{
	OutString = FlowArray::FormatArrayString<FSoftClassPath>(Values,
		[](const FSoftClassPath& Path) 
		{ 
			return Path.IsValid() ? Path.GetAssetName() : TEXT("None");
		},
		StringArraySeparator);
	return true;
}