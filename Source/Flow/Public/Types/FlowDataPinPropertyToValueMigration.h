// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowDataPinValuesStandard.h"

// #FlowDataPinLegacy
#include "Types/FlowDataPinProperties.h"
// --

// #FlowDataPinLegacy

// Templated helper to migrate simple types (scalar Value to TArray<Scalar> Values)
template <typename SourceType, typename TargetType, typename ValueType>
static bool MigrateSimpleType(const TInstancedStruct<FFlowDataPinProperty>& Source, TInstancedStruct<FFlowDataPinValue>& Target)
{
	if (!Source.IsValid() || !Source.GetPtr<SourceType>())
	{
		return false;
	}

	const SourceType* SourceData = Source.GetPtr<SourceType>();
	Target.InitializeAsScriptStruct(TargetType::StaticStruct());
	TargetType* TargetData = Target.GetMutablePtr<TargetType>();
	if (TargetData)
	{
		TargetData->Values.Add(SourceData->Value);
		return true;
	}
	return false;
}

// Specialization for FGameplayTagContainer
template <>
bool MigrateSimpleType<FFlowDataPinOutputProperty_GameplayTagContainer, FFlowDataPinValue_GameplayTagContainer, FGameplayTagContainer>(const TInstancedStruct<FFlowDataPinProperty>& Source, TInstancedStruct<FFlowDataPinValue>& Target)
{
	if (!Source.IsValid() || !Source.GetPtr<FFlowDataPinOutputProperty_GameplayTagContainer>())
	{
		return false;
	}

	const FFlowDataPinOutputProperty_GameplayTagContainer* SourceData = Source.GetPtr<FFlowDataPinOutputProperty_GameplayTagContainer>();
	Target.InitializeAsScriptStruct(FFlowDataPinValue_GameplayTagContainer::StaticStruct());
	FFlowDataPinValue_GameplayTagContainer* TargetData = Target.GetMutablePtr<FFlowDataPinValue_GameplayTagContainer>();
	if (TargetData)
	{
		TargetData->Values.AppendTags(SourceData->Value);
		return true;
	}
	return false;
}

// Specialization for FGameplayTagContainer (Input)
template <>
bool MigrateSimpleType<
	FFlowDataPinInputProperty_GameplayTagContainer,
	FFlowDataPinValue_GameplayTagContainer,
	FGameplayTagContainer>(
		const TInstancedStruct<FFlowDataPinProperty>& Source,
		TInstancedStruct<FFlowDataPinValue>& Target)
{
	if (!Source.IsValid() || !Source.GetPtr<FFlowDataPinInputProperty_GameplayTagContainer>())
	{
		return false;
	}

	const FFlowDataPinInputProperty_GameplayTagContainer* SourceData =
		Source.GetPtr<FFlowDataPinInputProperty_GameplayTagContainer>();

	Target.InitializeAsScriptStruct(FFlowDataPinValue_GameplayTagContainer::StaticStruct());
	FFlowDataPinValue_GameplayTagContainer* TargetData =
		Target.GetMutablePtr<FFlowDataPinValue_GameplayTagContainer>();

	if (TargetData)
	{
		TargetData->Values.AppendTags(SourceData->Value);
		return true;
	}
	return false;
}

// Specialization for Enum (handles Value and EnumClass)
template <>
bool MigrateSimpleType<FFlowDataPinOutputProperty_Enum, FFlowDataPinValue_Enum, FName>(const TInstancedStruct<FFlowDataPinProperty>& Source, TInstancedStruct<FFlowDataPinValue>& Target)
{
	if (!Source.IsValid() || !Source.GetPtr<FFlowDataPinOutputProperty_Enum>())
	{
		return false;
	}

	const FFlowDataPinOutputProperty_Enum* SourceData = Source.GetPtr<FFlowDataPinOutputProperty_Enum>();
	Target.InitializeAsScriptStruct(FFlowDataPinValue_Enum::StaticStruct());
	FFlowDataPinValue_Enum* TargetData = Target.GetMutablePtr<FFlowDataPinValue_Enum>();
	if (TargetData)
	{
		TargetData->Values.Add(SourceData->Value);
		TargetData->EnumClass = SourceData->EnumClass;
#if WITH_EDITORONLY_DATA
		TargetData->EnumName = SourceData->EnumName;
#endif
		return true;
	}
	return false;
}

// Specialization for Enum (Input)
template <>
bool MigrateSimpleType<FFlowDataPinInputProperty_Enum, FFlowDataPinValue_Enum, FName>(const TInstancedStruct<FFlowDataPinProperty>& Source, TInstancedStruct<FFlowDataPinValue>& Target)
{
	if (!Source.IsValid() || !Source.GetPtr<FFlowDataPinInputProperty_Enum>())
	{
		return false;
	}

	const FFlowDataPinInputProperty_Enum* SourceData = Source.GetPtr<FFlowDataPinInputProperty_Enum>();
	Target.InitializeAsScriptStruct(FFlowDataPinValue_Enum::StaticStruct());
	FFlowDataPinValue_Enum* TargetData = Target.GetMutablePtr<FFlowDataPinValue_Enum>();
	if (TargetData)
	{
		TargetData->Values.Add(SourceData->Value);
		TargetData->EnumClass = SourceData->EnumClass;
#if WITH_EDITORONLY_DATA
		TargetData->EnumName = SourceData->EnumName;
#endif
		return true;
	}
	return false;
}

// Specialization for Object (handles ReferenceValue/InlineValue and ClassFilter)
template <>
bool MigrateSimpleType<FFlowDataPinOutputProperty_Object, FFlowDataPinValue_Object, UObject*>(const TInstancedStruct<FFlowDataPinProperty>& Source, TInstancedStruct<FFlowDataPinValue>& Target)
{
	if (!Source.IsValid() || !Source.GetPtr<FFlowDataPinOutputProperty_Object>())
	{
		return false;
	}

	const FFlowDataPinOutputProperty_Object* SourceData = Source.GetPtr<FFlowDataPinOutputProperty_Object>();
	UScriptStruct* TargetStruct = FFlowDataPinValue_Object::StaticStruct();
	Target.InitializeAsScriptStruct(TargetStruct);

	{
		FFlowDataPinValue_Object* TargetData = Target.GetMutablePtr<FFlowDataPinValue_Object>();
		if (TargetData && SourceData->ReferenceValue)
		{
			TargetData->Values.Add(SourceData->ReferenceValue);
#if WITH_EDITORONLY_DATA
			TargetData->ClassFilter = SourceData->ClassFilter;
#endif
			return true;
		}
	}
	return false;
}

// Specialization for Object (Input)
template <>
bool MigrateSimpleType<FFlowDataPinInputProperty_Object, FFlowDataPinValue_Object, UObject*>(const TInstancedStruct<FFlowDataPinProperty>& Source, TInstancedStruct<FFlowDataPinValue>& Target)
{
	if (!Source.IsValid() || !Source.GetPtr<FFlowDataPinInputProperty_Object>())
	{
		return false;
	}

	const FFlowDataPinInputProperty_Object* SourceData = Source.GetPtr<FFlowDataPinInputProperty_Object>();
	UScriptStruct* TargetStruct = FFlowDataPinValue_Object::StaticStruct();
	Target.InitializeAsScriptStruct(TargetStruct);

	{
		FFlowDataPinValue_Object* TargetData = Target.GetMutablePtr<FFlowDataPinValue_Object>();
		if (TargetData && SourceData->ReferenceValue)
		{
			TargetData->Values.Add(SourceData->ReferenceValue);
#if WITH_EDITORONLY_DATA
			TargetData->ClassFilter = SourceData->ClassFilter;
#endif
			return true;
		}
	}
	return false;
}

// Specialization for Class (handles Value and ClassFilter)
template <>
bool MigrateSimpleType<FFlowDataPinOutputProperty_Class, FFlowDataPinValue_Class, FSoftClassPath>(const TInstancedStruct<FFlowDataPinProperty>& Source, TInstancedStruct<FFlowDataPinValue>& Target)
{
	if (!Source.IsValid() || !Source.GetPtr<FFlowDataPinOutputProperty_Class>())
	{
		return false;
	}

	const FFlowDataPinOutputProperty_Class* SourceData = Source.GetPtr<FFlowDataPinOutputProperty_Class>();
	Target.InitializeAsScriptStruct(FFlowDataPinValue_Class::StaticStruct());
	FFlowDataPinValue_Class* TargetData = Target.GetMutablePtr<FFlowDataPinValue_Class>();
	if (TargetData)
	{
		TargetData->Values.Add(SourceData->Value);
#if WITH_EDITORONLY_DATA
		TargetData->ClassFilter = SourceData->ClassFilter;
#endif
		return true;
	}
	return false;
}

// Specialization for Class (Input)
template <>
bool MigrateSimpleType<FFlowDataPinInputProperty_Class, FFlowDataPinValue_Class, FSoftClassPath>(const TInstancedStruct<FFlowDataPinProperty>& Source, TInstancedStruct<FFlowDataPinValue>& Target)
{
	if (!Source.IsValid() || !Source.GetPtr<FFlowDataPinInputProperty_Class>())
	{
		return false;
	}

	const FFlowDataPinInputProperty_Class* SourceData = Source.GetPtr<FFlowDataPinInputProperty_Class>();
	Target.InitializeAsScriptStruct(FFlowDataPinValue_Class::StaticStruct());
	FFlowDataPinValue_Class* TargetData = Target.GetMutablePtr<FFlowDataPinValue_Class>();
	if (TargetData)
	{
		TargetData->Values.Add(SourceData->Value);
#if WITH_EDITORONLY_DATA
		TargetData->ClassFilter = SourceData->ClassFilter;
#endif
		return true;
	}
	return false;
}

bool FFlowNamedDataPinProperty::FixupDataPinProperty()
{
	// Skip if no data to migrate or target already has data
	if (!DataPinProperty.IsValid() || DataPinValue.IsValid())
	{
		DataPinProperty.Reset();
		return false;
	}

	// Get source struct type
	const UScriptStruct* SourceStruct = DataPinProperty.GetScriptStruct();
	if (!SourceStruct || !SourceStruct->IsChildOf(FFlowDataPinProperty::StaticStruct()))
	{
		DataPinProperty.Reset();
		return false;
	}

	// Map source struct to target struct and migrate data
	bool bSuccess = false;

	// Bool (Output and Input)
	if (SourceStruct == FFlowDataPinOutputProperty_Bool::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Bool, FFlowDataPinValue_Bool, bool>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Bool::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Bool, FFlowDataPinValue_Bool, bool>(DataPinProperty, DataPinValue);
	}
	// Int32 (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Int32::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Int32, FFlowDataPinValue_Int, int32>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Int32::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Int32, FFlowDataPinValue_Int, int32>(DataPinProperty, DataPinValue);
	}
	// Int64 (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Int64::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Int64, FFlowDataPinValue_Int64, int64>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Int64::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Int64, FFlowDataPinValue_Int64, int64>(DataPinProperty, DataPinValue);
	}
	// Float (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Float::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Float, FFlowDataPinValue_Float, float>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Float::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Float, FFlowDataPinValue_Float, float>(DataPinProperty, DataPinValue);
	}
	// Double (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Double::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Double, FFlowDataPinValue_Double, double>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Double::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Double, FFlowDataPinValue_Double, double>(DataPinProperty, DataPinValue);
	}
	// Name (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Name::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Name, FFlowDataPinValue_Name, FName>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Name::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Name, FFlowDataPinValue_Name, FName>(DataPinProperty, DataPinValue);
	}
	// String (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_String::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_String, FFlowDataPinValue_String, FString>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_String::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_String, FFlowDataPinValue_String, FString>(DataPinProperty, DataPinValue);
	}
	// Text (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Text::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Text, FFlowDataPinValue_Text, FText>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Text::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Text, FFlowDataPinValue_Text, FText>(DataPinProperty, DataPinValue);
	}
	// Enum (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Enum::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Enum, FFlowDataPinValue_Enum, FName>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Enum::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Enum, FFlowDataPinValue_Enum, FName>(DataPinProperty, DataPinValue);
	}
	// Vector (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Vector::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Vector, FFlowDataPinValue_Vector, FVector>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Vector::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Vector, FFlowDataPinValue_Vector, FVector>(DataPinProperty, DataPinValue);
	}
	// Rotator (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Rotator::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Rotator, FFlowDataPinValue_Rotator, FRotator>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Rotator::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Rotator, FFlowDataPinValue_Rotator, FRotator>(DataPinProperty, DataPinValue);
	}
	// Transform (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Transform::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Transform, FFlowDataPinValue_Transform, FTransform>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Transform::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Transform, FFlowDataPinValue_Transform, FTransform>(DataPinProperty, DataPinValue);
	}
	// GameplayTag (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_GameplayTag::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_GameplayTag, FFlowDataPinValue_GameplayTag, FGameplayTag>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_GameplayTag::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_GameplayTag, FFlowDataPinValue_GameplayTag, FGameplayTag>(DataPinProperty, DataPinValue);
	}
	// GameplayTagContainer (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_GameplayTagContainer::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_GameplayTagContainer, FFlowDataPinValue_GameplayTagContainer, FGameplayTagContainer>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_GameplayTagContainer::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_GameplayTagContainer, FFlowDataPinValue_GameplayTagContainer, FGameplayTagContainer>(DataPinProperty, DataPinValue);
	}
	// InstancedStruct (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_InstancedStruct::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_InstancedStruct, FFlowDataPinValue_InstancedStruct, FInstancedStruct>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_InstancedStruct::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_InstancedStruct, FFlowDataPinValue_InstancedStruct, FInstancedStruct>(DataPinProperty, DataPinValue);
	}
	// Object (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Object::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Object, FFlowDataPinValue_Object, UObject*>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Object::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Object, FFlowDataPinValue_Object, UObject*>(DataPinProperty, DataPinValue);
	}
	// Class (Output and Input)
	else if (SourceStruct == FFlowDataPinOutputProperty_Class::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinOutputProperty_Class, FFlowDataPinValue_Class, FSoftClassPath>(DataPinProperty, DataPinValue);
	}
	else if (SourceStruct == FFlowDataPinInputProperty_Class::StaticStruct())
	{
		bSuccess = MigrateSimpleType<FFlowDataPinInputProperty_Class, FFlowDataPinValue_Class, FSoftClassPath>(DataPinProperty, DataPinValue);
	}

	// Clear the deprecated property
	DataPinProperty.Reset();

	return bSuccess;
}
// --
