// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/FlowPin.h"
#include "FlowLogChannels.h"

#include "GameplayTagContainer.h"
#include "Misc/DateTime.h"
#include "Misc/MessageDialog.h"
#include "StructUtils/InstancedStruct.h"
#include "Types/FlowPinType.h"
#include "Types/FlowPinTypesStandard.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowPin)

#define LOCTEXT_NAMESPACE "FlowPin"

//////////////////////////////////////////////////////////////////////////
// Pin Record

#if !UE_BUILD_SHIPPING
FString FPinRecord::NoActivations = TEXT("No activations");
FString FPinRecord::PinActivations = TEXT("Pin activations");
FString FPinRecord::ForcedActivation = TEXT(" (forced activation)");
FString FPinRecord::PassThroughActivation = TEXT(" (pass-through activation)");

FPinRecord::FPinRecord()
	: Time(0.0f)
	, HumanReadableTime(FString())
	, ActivationType(EFlowPinActivationType::Default)
{
}

FPinRecord::FPinRecord(const double InTime, const EFlowPinActivationType InActivationType)
	: Time(InTime)
	, ActivationType(InActivationType)
{
	const FDateTime SystemTime(FDateTime::Now());
	HumanReadableTime = DoubleDigit(SystemTime.GetHour()) + TEXT(".")
		+ DoubleDigit(SystemTime.GetMinute()) + TEXT(".")
		+ DoubleDigit(SystemTime.GetSecond()) + TEXT(":")
		+ DoubleDigit(SystemTime.GetMillisecond()).Left(3);
}

FORCEINLINE FString FPinRecord::DoubleDigit(const int32 Number)
{
	return Number > 9 ? FString::FromInt(Number) : TEXT("0") + FString::FromInt(Number);
}
#endif

//////////////////////////////////////////////////////////////////////////
// Flow Pin

bool FFlowPin::IsExecPin() const
{
	return PinTypeName == FFlowPinType_Exec::GetPinTypeNameStatic();
}

bool FFlowPin::IsExecPinCategory(const FName& PC)
{
	return PC == FFlowPinType_Exec::GetPinTypeNameStatic().Name;
}

const FName FFlowPin::MetadataKey_SourceForOutputFlowPin = "SourceForOutputFlowPin";
const FName FFlowPin::MetadataKey_DefaultForInputFlowPin = "DefaultForInputFlowPin";
const FName FFlowPin::MetadataKey_FlowPinType = "FlowPinType";

void FFlowPin::SetPinTypeName(const FFlowPinTypeName& InTypeName)
{
	if (PinTypeName == InTypeName)
	{
		return;
	}

	PinTypeName = InTypeName;
}

void FFlowPin::TrySetStructSubCategoryObjectFromPinType()
{
	if (PinTypeName == FFlowPinType_Vector::GetPinTypeNameStatic())
	{
		PinSubCategoryObject = TBaseStructure<FVector>::Get();
	}
	else if (PinTypeName == FFlowPinType_Rotator::GetPinTypeNameStatic())
	{
		PinSubCategoryObject = TBaseStructure<FRotator>::Get();
	}
	else if (PinTypeName == FFlowPinType_Transform::GetPinTypeNameStatic())
	{
		PinSubCategoryObject = TBaseStructure<FTransform>::Get();
	}
	else if (PinTypeName == FFlowPinType_GameplayTag::GetPinTypeNameStatic())
	{
		PinSubCategoryObject = TBaseStructure<FGameplayTag>::Get();
	}
	else if (PinTypeName == FFlowPinType_GameplayTagContainer::GetPinTypeNameStatic())
	{
		PinSubCategoryObject = TBaseStructure<FGameplayTagContainer>::Get();
	}
	else if (PinTypeName == FFlowPinType_InstancedStruct::GetPinTypeNameStatic())
	{
		PinSubCategoryObject = TBaseStructure<FInstancedStruct>::Get();
	}
	else if (PinTypeName == FFlowPinType_Enum::GetPinTypeNameStatic())
	{
		// Clear the PinSubCategoryObject if it is not an Enum
		const UObject* PinSubCategoryObjectPtr = PinSubCategoryObject.Get();
		if (PinSubCategoryObjectPtr && !PinSubCategoryObjectPtr->IsA<UEnum>())
		{
			PinSubCategoryObject = nullptr;
		}
	}
	else if (PinTypeName == FFlowPinType_Object::GetPinTypeNameStatic())
	{
		// Clear the PinSubCategoryObject if it is not an Object
		const UObject* PinSubCategoryObjectPtr = PinSubCategoryObject.Get();
		if (PinSubCategoryObjectPtr && !PinSubCategoryObjectPtr->IsA<UObject>())
		{
			PinSubCategoryObject = nullptr;
		}
	}
	else if (PinTypeName == FFlowPinType_Class::GetPinTypeNameStatic())
	{
		// Clear the PinSubCategoryObject if it is not a Class
		const UObject* PinSubCategoryObjectPtr = PinSubCategoryObject.Get();
		if (PinSubCategoryObjectPtr && !PinSubCategoryObjectPtr->IsA<UClass>())
		{
			PinSubCategoryObject = nullptr;
		}
	}
	else
	{
		// Clear the PinSubCategoryObject for all PinTypes that do not use it.
		PinSubCategoryObject = nullptr;
	}
}

#if WITH_EDITOR
FEdGraphPinType FFlowPin::BuildEdGraphPinType() const
{
	check(!PinTypeName.Name.IsNone());

	FEdGraphPinType EdGraphPinType;
	EdGraphPinType.PinCategory = PinTypeName.Name;

	// TODO (gtaylor) possible future extension for types, to allow sub categories
	EdGraphPinType.PinSubCategory = NAME_None;

	EdGraphPinType.PinSubCategoryObject = PinSubCategoryObject;
	EdGraphPinType.ContainerType = ContainerType;

	return EdGraphPinType;
}
#endif

const FFlowPinType* FFlowPin::ResolveFlowPinType() const
{
	// TODO (gtaylor) consider caching this in a mutable?
	return FFlowPinType::LookupPinType(PinTypeName);
}

// #FlowDataPinLegacy
FFlowPinTypeName FFlowPin::GetPinTypeNameForLegacyPinType(EFlowPinType PinType)
{
	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);
	switch (PinType)
	{
	case EFlowPinType::Exec:
		return FFlowPinType_Exec::GetPinTypeNameStatic();
	case EFlowPinType::Bool:
		return FFlowPinType_Bool::GetPinTypeNameStatic();
	case EFlowPinType::Int:
		return FFlowPinType_Int::GetPinTypeNameStatic();
	case EFlowPinType::Float:
		return FFlowPinType_Float::GetPinTypeNameStatic();
	case EFlowPinType::Name:
		return FFlowPinType_Name::GetPinTypeNameStatic();
	case EFlowPinType::String:
		return FFlowPinType_String::GetPinTypeNameStatic();
	case EFlowPinType::Text:
		return FFlowPinType_Text::GetPinTypeNameStatic();
	case EFlowPinType::Enum:
		return FFlowPinType_Enum::GetPinTypeNameStatic();
	case EFlowPinType::Vector:
		return FFlowPinType_Vector::GetPinTypeNameStatic();
	case EFlowPinType::Rotator:
		return FFlowPinType_Rotator::GetPinTypeNameStatic();
	case EFlowPinType::Transform:
		return FFlowPinType_Transform::GetPinTypeNameStatic();
	case EFlowPinType::GameplayTag:
		return FFlowPinType_GameplayTag::GetPinTypeNameStatic();
	case EFlowPinType::GameplayTagContainer:
		return FFlowPinType_GameplayTagContainer::GetPinTypeNameStatic();
	case EFlowPinType::InstancedStruct:
		return FFlowPinType_InstancedStruct::GetPinTypeNameStatic();
	case EFlowPinType::Object:
		return FFlowPinType_Object::GetPinTypeNameStatic();
	case EFlowPinType::Class:
		return FFlowPinType_Class::GetPinTypeNameStatic();
	default:
		return FFlowPinTypeName();
	}
}

#if WITH_EDITOR
void FFlowPin::PostEditChangedPinTypeOrSubCategorySource()
{
	// PinTypes with PinSubCategoryObjects will need to update this function

	// Must be called from PostEditChangeProperty() by an owning UObject <sigh>

	if (PinTypeName == FFlowPinType_Class::GetPinTypeNameStatic())
	{
		PinSubCategoryObject = SubCategoryClassFilter;
	}
	else if (PinTypeName == FFlowPinType_Object::GetPinTypeNameStatic())
	{
		PinSubCategoryObject = SubCategoryObjectFilter;
	}
	else if (PinTypeName == FFlowPinType_Enum::GetPinTypeNameStatic())
	{
		if (!SubCategoryEnumName.IsEmpty())
		{
			SubCategoryEnumClass = UClass::TryFindTypeSlow<UEnum>(SubCategoryEnumName, EFindFirstObjectOptions::ExactClass);
			if (SubCategoryEnumClass != nullptr && !FFlowPin::ValidateEnum(*SubCategoryEnumClass))
			{
				SubCategoryEnumClass = nullptr;
			}
		}

		PinSubCategoryObject = SubCategoryEnumClass;
	}
	else
	{
		TrySetStructSubCategoryObjectFromPinType();
	}
}

// --

FText FFlowPin::BuildHeaderText() const
{
	const FText PinNameToUse = !PinFriendlyName.IsEmpty() ? PinFriendlyName : FText::FromName(PinName);

	if (IsExecPin())
	{
		return PinNameToUse;
	}
	else
	{
		return FText::Format(LOCTEXT("FlowPinNameAndType", "{0} ({1})"), {PinNameToUse, FText::FromString(PinTypeName.ToString())});
	}
}

bool FFlowPin::ValidateEnum(const UEnum& EnumType)
{
	// This function copied and adapted from UBlackboardKeyType_Enum::ValidateEnum(),
	// because it is inaccessible w/o AIModule and private access

	bool bAllValid = true;

	// Do not test the max value (if present) since it is an internal value and users don't have access to it
	const int32 NumEnums = EnumType.ContainsExistingMax() ? EnumType.NumEnums() - 1 : EnumType.NumEnums();
	for (int32 i = 0; i < NumEnums; i++)
	{
		// Enum data type is uint8 (based on UBlackboardKeyType_Enum::ValidateEnum())
		typedef uint8 FDataType;

		const int64 Value = EnumType.GetValueByIndex(i);
		if (Value < std::numeric_limits<FDataType>::min() || Value > std::numeric_limits<FDataType>::max())
		{
			UE_LOG(LogFlow, Error, TEXT("'%s' value %lld is outside the range of supported key values for enum [%d, %d].")
			       , *EnumType.GenerateFullEnumName(*EnumType.GetDisplayNameTextByIndex(i).ToString())
			       , Value, std::numeric_limits<FDataType>::min(), std::numeric_limits<FDataType>::max());

			bAllValid = false;
		}
	}

	if (!bAllValid)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Unsupported enumeration"
		                                              , "Specified enumeration contains one or more values outside supported value range for enum keys and can not be used for Flow Data Pins. See log for details."));
	}

	return bAllValid;
}
#endif //WITH_EDITOR

#undef LOCTEXT_NAMESPACE
