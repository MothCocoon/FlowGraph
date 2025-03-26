// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowDataPinProperties.h"
#include "Types/FlowClassUtils.h"
#include "FlowLogChannels.h"
#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"
#if WITH_EDITOR
#include "EditorClassUtils.h"
#endif

#define LOCTEXT_NAMESPACE "FlowDataPinProperties"

#if WITH_EDITOR

FFlowPin FFlowDataPinProperty::CreateFlowPin(const FName& PinName, const TInstancedStruct<FFlowDataPinProperty>& DataPinProperty)
{
	FFlowPin FlowPin;

	const FFlowDataPinProperty* Property = DataPinProperty.GetPtr<FFlowDataPinProperty>();
	if (!Property)
	{
		return FlowPin;
	}

	FlowPin.PinName = PinName;

	const EFlowPinType FlowPinType = Property->GetFlowPinType();

	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	switch (FlowPinType)
	{
	case EFlowPinType::Enum:
		{
			const FFlowDataPinOutputProperty_Enum& EnumDataPinProperty = DataPinProperty.Get<FFlowDataPinOutputProperty_Enum>();
			UEnum* EnumClass = EnumDataPinProperty.EnumClass;

			FlowPin.SetPinType(FlowPinType, EnumClass);
		}
		break;

	case EFlowPinType::Class:
		{
			const FFlowDataPinOutputProperty_Class& ClassDataPinProperty = DataPinProperty.Get<FFlowDataPinOutputProperty_Class>();

			FlowPin.SetPinType(FlowPinType, ClassDataPinProperty.ClassFilter);
		}
		break;

	case EFlowPinType::Object:
		{
			const FFlowDataPinOutputProperty_Object& ObjectDataPinProperty = DataPinProperty.Get<FFlowDataPinOutputProperty_Object>();

			FlowPin.SetPinType(FlowPinType, ObjectDataPinProperty.ClassFilter);
		}
		break;

	default:
		{
			FlowPin.SetPinType(FlowPinType);
		}
		break;
	}

	return FlowPin;
}

void FFlowDataPinOutputProperty_Enum::OnEnumNameChanged()
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

FText FFlowNamedDataPinProperty::BuildHeaderText() const
{
	EFlowPinType PinType = EFlowPinType::Invalid;

	if (const FFlowDataPinProperty* DataPinPropertyPtr = DataPinProperty.GetPtr<FFlowDataPinProperty>())
	{
		PinType = DataPinPropertyPtr->GetFlowPinType();
	}

	return FText::Format(LOCTEXT("FlowNamedDataPinPropertyHeader", "{0} ({1})"), { FText::FromName(Name), UEnum::GetDisplayValueAsText(PinType) });
}

UClass* FFlowDataPinOutputProperty_Class::DeriveMetaClass(const FProperty& MetaDataProperty) const
{
	if (UClass* MetaClass = TryGetMetaClassFromProperty(MetaDataProperty))
	{
		return MetaClass;
	}

	return ClassFilter;
}

UClass* FFlowDataPinOutputProperty_Class::TryGetMetaClassFromProperty(const FProperty& MetaDataProperty)
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

UClass* FFlowDataPinOutputProperty_Object::DeriveObjectClass(const FProperty& MetaDataProperty) const
{
	if (UClass* MetaClass = FFlowDataPinOutputProperty_Object::TryGetObjectClassFromProperty(MetaDataProperty))
	{
		return MetaClass;
	}

	return ClassFilter;
}

UClass* FFlowDataPinOutputProperty_Object::TryGetObjectClassFromProperty(const FProperty& MetaDataProperty)
{
	if (UClass* MetaClass = FFlowDataPinOutputProperty_Class::TryGetMetaClassFromProperty(MetaDataProperty))
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
#endif

bool FFlowNamedDataPinProperty::IsInputProperty() const
{
	if (const FFlowDataPinProperty* DataPinPropertyPtr = DataPinProperty.GetPtr())
	{
		return DataPinPropertyPtr->IsInputProperty();
	}

	return false;
}

bool FFlowNamedDataPinProperty::IsOutputProperty() const
{
	if (const FFlowDataPinProperty* DataPinPropertyPtr = DataPinProperty.GetPtr())
	{
		return !DataPinPropertyPtr->IsInputProperty();
	}

	return false;
}

FFlowDataPinOutputProperty_Object::FFlowDataPinOutputProperty_Object(UObject* InValue, UClass* InClassFilter)
	: Super()
#if WITH_EDITOR
	, ClassFilter(InClassFilter)
#endif
{
	SetObjectValue(InValue);
}

void FFlowDataPinOutputProperty_Object::SetObjectValue(UObject* InValue)
{
	UClass* ObjectClass = IsValid(InValue) ? InValue->GetClass() : nullptr;
	if (IsValid(ObjectClass))
	{
		const bool bIsInstanced = (ObjectClass->ClassFlags & CLASS_EditInlineNew) != 0;

		if (bIsInstanced)
		{
			InlineValue = InValue;
			ReferenceValue = nullptr;
		}
		else
		{
			InlineValue = nullptr;
			ReferenceValue = InValue;
		}
	}
	else
	{
		InlineValue = nullptr;
		ReferenceValue = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
