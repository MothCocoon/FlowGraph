// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/FlowPin.h"
#include "FlowLogChannels.h"

#include "GameplayTagContainer.h"
#include "Misc/DateTime.h"
#include "Misc/MessageDialog.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif

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
// Pin Trait

void FFlowPinTrait::AllowTrait()
{
	if (!bTraitAllowed)
	{
		bTraitAllowed = true;
		bEnabled = true;
	}
}

void FFlowPinTrait::DisallowTrait()
{
	if (bTraitAllowed)
	{
		bTraitAllowed = false;
		bEnabled = false;
	}
}

bool FFlowPinTrait::IsAllowed() const
{
	return bTraitAllowed;
}

void FFlowPinTrait::EnableTrait()
{
	if (bTraitAllowed && !bEnabled)
	{
		bEnabled = true;
	}
}

void FFlowPinTrait::DisableTrait()
{
	if (bTraitAllowed && bEnabled)
	{
		bEnabled = false;
	}
}

void FFlowPinTrait::ToggleTrait()
{
	if (bTraitAllowed)
	{
		bTraitAllowed = false;
		bEnabled = false;
	}
	else
	{
		bTraitAllowed = true;
		bEnabled = true;
	}
}

bool FFlowPinTrait::CanEnable() const
{
	return bTraitAllowed && !bEnabled;
}

bool FFlowPinTrait::IsEnabled() const
{
	return bTraitAllowed && bEnabled;
}

void FFlowPinTrait::MarkAsHit()
{
	bHit = true;
}

void FFlowPinTrait::ResetHit()
{
	bHit = false;
}

bool FFlowPinTrait::IsHit() const
{
	return bHit;
}

//////////////////////////////////////////////////////////////////////////
// Flow Pin

TArray<FName> FFlowPin::FlowPinTypeEnumValuesWithoutSpaces;

const FName FFlowPin::MetadataKey_SourceForOutputFlowPin = "SourceForOutputFlowPin";
const FName FFlowPin::MetadataKey_DefaultForInputFlowPin = "DefaultForInputFlowPin";
const FName FFlowPin::MetadataKey_FlowPinType = "FlowPinType";

const TArray<FName>& FFlowPin::GetFlowPinTypeEnumValuesWithoutSpaces()
{
	if (FlowPinTypeEnumValuesWithoutSpaces.IsEmpty())
	{
		FlowPinTypeEnumValuesWithoutSpaces.Reserve(static_cast<int32>(EFlowPinType::Max));

		// Do a one-time caching of the string-names for this enum, 
		// since we need to de-spacify it and everything....

		for (EFlowPinType PinType : TEnumRange<EFlowPinType>())
		{
			FString StringValue = UEnum::GetDisplayValueAsText(PinType).ToString();
			StringValue.RemoveSpacesInline();

			FlowPinTypeEnumValuesWithoutSpaces.Add(FName(StringValue));
		}
	}

	return FlowPinTypeEnumValuesWithoutSpaces;
}

bool FFlowPin::ArePinArraysMatchingNamesAndTypes(const TArray<FFlowPin>& Left, const TArray<FFlowPin>& Right)
{
	if (Left.Num() != Right.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < Left.Num(); ++Index)
	{
		const FFlowPin& LeftPin = Left[Index];
		const FFlowPin& RightPin = Right[Index];

		if (!DoPinsMatchNamesAndTypes(LeftPin, RightPin))
		{
			return false;
		}
	}

	return true;
}

void FFlowPin::SetPinType(EFlowPinType InFlowPinType, UObject* SubCategoryObject)
{
	if (PinType == InFlowPinType)
	{
		return;
	}

	PinType = InFlowPinType;

	PinSubCategoryObject = SubCategoryObject;

	TrySetStructSubCategoryObjectFromPinType();
}

void FFlowPin::TrySetStructSubCategoryObjectFromPinType()
{
	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	// Set the PinSubCategoryObject based on the PinType (if appropriate)
	switch (PinType)
	{
	case EFlowPinType::Vector:
		{
			PinSubCategoryObject = TBaseStructure<FVector>::Get();
		}
		break;

	case EFlowPinType::Rotator:
		{
			PinSubCategoryObject = TBaseStructure<FRotator>::Get();
		}
		break;

	case EFlowPinType::Transform:
		{
			PinSubCategoryObject = TBaseStructure<FTransform>::Get();
		}
		break;

	case EFlowPinType::GameplayTag:
		{
			PinSubCategoryObject = TBaseStructure<FGameplayTag>::Get();
		}
		break;

	case EFlowPinType::GameplayTagContainer:
		{
			PinSubCategoryObject = TBaseStructure<FGameplayTagContainer>::Get();
		}
		break;

	case EFlowPinType::InstancedStruct:
		{
			PinSubCategoryObject = TBaseStructure<FInstancedStruct>::Get();
		}
		break;

	case EFlowPinType::Enum:
		{
			// Clear the PinSubCategoryObject if it is not an Enum
			UObject* PinSubCategoryObjectPtr = PinSubCategoryObject.Get();
			if (PinSubCategoryObjectPtr && !PinSubCategoryObjectPtr->IsA<UEnum>())
			{
				PinSubCategoryObject = nullptr;
			}
		}
		break;

	case EFlowPinType::Object:
		{
			// Clear the PinSubCategoryObject if it is not a Object
			UObject* PinSubCategoryObjectPtr = PinSubCategoryObject.Get();
			if (PinSubCategoryObjectPtr && !PinSubCategoryObjectPtr->IsA<UObject>())
			{
				PinSubCategoryObject = nullptr;
			}
		}
		break;

	case EFlowPinType::Class:
		{
			// Clear the PinSubCategoryObject if it is not a Class
			UObject* PinSubCategoryObjectPtr = PinSubCategoryObject.Get();
			if (PinSubCategoryObjectPtr && !PinSubCategoryObjectPtr->IsA<UClass>())
			{
				PinSubCategoryObject = nullptr;
			}
		}
		break;

	default:
		{
			// Clear the PinSubCategoryObject for all PinTypes that do not use it.
			PinSubCategoryObject = nullptr;
		}
		break;
	}
}

const FName& FFlowPin::GetPinCategoryFromPinType(EFlowPinType FlowPinType)
{
	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	switch (FlowPinType)
	{
	case EFlowPinType::Exec:
		return FFlowPin::PC_Exec;

	case EFlowPinType::Bool:
		return FFlowPin::PC_Boolean;

	case EFlowPinType::Int:
		return FFlowPin::PC_Int;

	case EFlowPinType::Float:
		return FFlowPin::PC_Float;

	case EFlowPinType::Name:
		return FFlowPin::PC_Name;

	case EFlowPinType::String:
		return FFlowPin::PC_String;

	case EFlowPinType::Text:
		return FFlowPin::PC_Text;

	case EFlowPinType::Enum:
		return FFlowPin::PC_Enum;

	case EFlowPinType::Vector:
	case EFlowPinType::Rotator:
	case EFlowPinType::Transform:
	case EFlowPinType::GameplayTag:
	case EFlowPinType::GameplayTagContainer:
	case EFlowPinType::InstancedStruct:
		return FFlowPin::PC_Struct;

	case EFlowPinType::Object:
		return FFlowPin::PC_Object;

	case EFlowPinType::Class:
		return FFlowPin::PC_Class;

	default:
		{
			static const FName NameNone = NAME_None;
			return NameNone;
		}
	}
}

#if WITH_EDITOR
void FFlowPin::PostEditChangedPinTypeOrSubCategorySource()
{
	// PinTypes with PinSubCategoryObjects will need to update this function
	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	// Must be called from PostEditChangeProperty() by an owning UObject <sigh>

	switch (PinType)
	{
	case EFlowPinType::Class:
		{
			PinSubCategoryObject = SubCategoryClassFilter;
		}
		break;

	case EFlowPinType::Object:
		{
			PinSubCategoryObject = SubCategoryObjectFilter;
		}
		break;

	case EFlowPinType::Enum:
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
		break;

	default:
		{
			TrySetStructSubCategoryObjectFromPinType();
		}
		break;
	}
}

FText FFlowPin::BuildHeaderText() const
{
	const FText PinNameToUse = !PinFriendlyName.IsEmpty() ? PinFriendlyName : FText::FromName(PinName);

	if (PinType == EFlowPinType::Exec)
	{
		return PinNameToUse;
	}
	else
	{
		return FText::Format(LOCTEXT("FlowPinNameAndType", "{0} ({1})"), {PinNameToUse, UEnum::GetDisplayValueAsText(PinType)});
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
