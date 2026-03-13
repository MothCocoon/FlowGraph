// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn_PredicateCompareValues.h"

#include "FlowSettings.h"
#include "Types/FlowPinTypeNamesStandard.h"
#include "Types/FlowPinTypesStandard.h"
#include "Types/FlowDataPinValuesStandard.h"

#define LOCTEXT_NAMESPACE "FlowNodeAddOn_PredicateCompareValues"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn_PredicateCompareValues)

namespace
{
#if WITH_EDITOR
	static void ForceNamedPropertyPinDirection(FFlowNamedDataPinProperty& NamedProperty, const bool bIsInput)
	{
		const UScriptStruct* ScriptStruct = NamedProperty.DataPinValue.GetScriptStruct();
		if (IsValid(ScriptStruct) && ScriptStruct->IsChildOf<FFlowDataPinValue>())
		{
			FFlowDataPinValue& WrapperValue = NamedProperty.DataPinValue.GetMutable<FFlowDataPinValue>();
			WrapperValue.bIsInputPin = bIsInput;
		}
	}
#endif // WITH_EDITOR
}

UFlowNodeAddOn_PredicateCompareValues::UFlowNodeAddOn_PredicateCompareValues()
	: Super()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::AddOn_Predicate;
	Category = TEXT("DataPins");
#endif

	if (GetAuthoredValueName(LeftValue).IsNone())
	{
		LeftValue.Name = TEXT("Compare Left Value");
	}
	if (GetAuthoredValueName(RightValue).IsNone())
	{
		RightValue.Name = TEXT("Compare Right Value");
	}

#if WITH_EDITORONLY_DATA
	// Encourage input pins by default; PostEditChangeChainProperty enforces it.
	if (FFlowDataPinValue* LeftWrapper = LeftValue.DataPinValue.GetMutablePtr<FFlowDataPinValue>())
	{
		LeftWrapper->bIsInputPin = true;
	}
	if (FFlowDataPinValue* RightWrapper = RightValue.DataPinValue.GetMutablePtr<FFlowDataPinValue>())
	{
		RightWrapper->bIsInputPin = true;
	}
#endif
}

bool UFlowNodeAddOn_PredicateCompareValues::TryFindPropertyByPinName(const FName& PinName, const FProperty*& OutFoundProperty, TInstancedStruct<FFlowDataPinValue>& OutFoundInstancedStruct) const
{
	// TODO (gtaylor) It would be nicer if the base IFlowDataPinValueOwnerInterface::TryFindPropertyByPinName implementation
	// could find member FFlowNamedDataPinProperty's by their Name field, but that would require a property search,
	// so we don't need to special-case these.  Maybe we can think of a more clever version at some point.
	if (GetAuthoredValueName(LeftValue) == PinName)
	{
		OutFoundInstancedStruct = LeftValue.DataPinValue;

		return true;
	}

	if (GetAuthoredValueName(RightValue) == PinName)
	{
		OutFoundInstancedStruct = RightValue.DataPinValue;

		return true;
	}

	if (Super::TryFindPropertyByPinName(PinName, OutFoundProperty, OutFoundInstancedStruct))
	{
		return true;
	}

	return false;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsEqualityOp() const
{
	return EFlowPredicateCompareOperatorType_Classifiers::IsEqualityOperation(OperatorType);
}

bool UFlowNodeAddOn_PredicateCompareValues::IsArithmeticOp() const
{
	return EFlowPredicateCompareOperatorType_Classifiers::IsArithmeticOperation(OperatorType);
}

bool UFlowNodeAddOn_PredicateCompareValues::IsNumericTypeName(const FName& TypeName)
{
	return
		IsFloatingPointType(TypeName) ||
		IsIntegerType(TypeName);
}

bool UFlowNodeAddOn_PredicateCompareValues::IsFloatingPointType(const FName& TypeName)
{
	return
		TypeName == FFlowPinTypeNamesStandard::PinTypeNameFloat ||
		TypeName == FFlowPinTypeNamesStandard::PinTypeNameDouble;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsIntegerType(const FName& TypeName)
{
	return
		TypeName == FFlowPinTypeNamesStandard::PinTypeNameInt ||
		TypeName == FFlowPinTypeNamesStandard::PinTypeNameInt64;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsTextType(const FName& TypeName)
{
	return TypeName == FFlowPinTypeNamesStandard::PinTypeNameText;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsStringType(const FName& TypeName)
{
	return TypeName == FFlowPinTypeNamesStandard::PinTypeNameString;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsNameLikeType(const FName& TypeName)
{
	// Treat Enum as "Name-like" for comparisons (case-insensitive)
	return
		TypeName == FFlowPinTypeNamesStandard::PinTypeNameName ||
		TypeName == FFlowPinTypeNamesStandard::PinTypeNameEnum;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsEnumTypeName(const FName& TypeName)
{
	return TypeName == FFlowPinTypeNamesStandard::PinTypeNameEnum;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsAnyStringLikeTypeName(const FName& TypeName)
{
	return
		IsNameLikeType(TypeName) ||
		IsTextType(TypeName) ||
		IsStringType(TypeName);
}

bool UFlowNodeAddOn_PredicateCompareValues::IsGameplayTagLikeTypeName(const FName& TypeName)
{
	return
		TypeName == FFlowPinTypeNamesStandard::PinTypeNameGameplayTag ||
		TypeName == FFlowPinTypeNamesStandard::PinTypeNameGameplayTagContainer;
}

#if WITH_EDITOR

void UFlowNodeAddOn_PredicateCompareValues::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	// Force both FFlowNamedDataPinProperty values to always be input pins.
	const auto& ChangedProperty = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue();
	constexpr bool bIsInput = true;
	OnPostEditEnsureAllNamedPropertiesPinDirection(*ChangedProperty, bIsInput);

	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	OnReconstructionRequested.ExecuteIfBound();
}

void UFlowNodeAddOn_PredicateCompareValues::OnPostEditEnsureAllNamedPropertiesPinDirection(const FProperty& Property, bool bIsInput)
{
	if (Property.GetFName() == GetLeftValuePropertyName())
	{
		ForceNamedPropertyPinDirection(LeftValue, bIsInput);
	}
	else if (Property.GetFName() == GetRightValuePropertyName())
	{
		ForceNamedPropertyPinDirection(RightValue, bIsInput);
	}
}

FText UFlowNodeAddOn_PredicateCompareValues::K2_GetNodeTitle_Implementation() const
{
	using namespace EFlowPredicateCompareOperatorType_Classifiers;

	const bool bIsClassDefault = HasAnyFlags(RF_ArchetypeObject | RF_ClassDefaultObject);

	if (!bIsClassDefault && 
		GetDefault<UFlowSettings>()->bUseAdaptiveNodeTitles)
	{
		const FText LeftDisplayName = FText::FromName(GetAuthoredValueName(LeftValue));
		const FText RightDisplayName = FText::FromName(GetAuthoredValueName(RightValue));

		const FString OperatorString = GetOperatorSymbolString(OperatorType);
		const FText OperatorText = FText::FromString(OperatorString);

		return FText::Format(LOCTEXT("CompareValuesTitle", "{0} {1} {2}"), { LeftDisplayName, OperatorText, RightDisplayName });
	}

	return Super::K2_GetNodeTitle_Implementation();
}

#endif // WITH_EDITOR

bool UFlowNodeAddOn_PredicateCompareValues::AreComparableStandardPinTypes(const FName& LeftPinTypeName, const FName& RightPinTypeName)
{
	// TODO (gtaylor) We should update this function to respect the authored pin type compatibility settings.
	// We can't at this time, because they are known only to the editor flow code (UFlowGraphSchema::ArePinTypesCompatible),
	// but we can conceivably move that information to UFlowAsset (or similar) for runtime and editor-time code to use.

	if (LeftPinTypeName == RightPinTypeName)
	{
		return true;
	}

	// Numeric: allow int/int64/float/double interchange
	if (IsNumericTypeName(LeftPinTypeName) && IsNumericTypeName(RightPinTypeName))
	{
		return true;
	}

	// String-like: allow Name/String/Text/Enum interchange
	// (we include Enums as they have FName values for the purposes of comparison)
	if (IsAnyStringLikeTypeName(LeftPinTypeName) && IsAnyStringLikeTypeName(RightPinTypeName))
	{
		return true;
	}

	// GameplayTag / Container: allow interchange (type templates can upscale tag -> container)
	if (IsGameplayTagLikeTypeName(LeftPinTypeName) && IsGameplayTagLikeTypeName(RightPinTypeName))
	{
		return true;
	}

	return false;
}

bool UFlowNodeAddOn_PredicateCompareValues::TryCheckGameplayTagsEqual(bool& bOutIsEqual) const
{
	// Compare both sides as containers; pin type templates should allow tag->container conversion.
	FGameplayTagContainer LeftContainer;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_GameplayTagContainer>(GetDisambiguatedValueName(LeftValue), LeftContainer, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve LeftValue as GameplayTagContainer."));
			return false;
		}
	}

	FGameplayTagContainer RightContainer;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_GameplayTagContainer>(GetDisambiguatedValueName(RightValue), RightContainer, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve RightValue as GameplayTagContainer."));
			return false;
		}
	}

	bOutIsEqual = (LeftContainer == RightContainer);
	return true;
}

bool UFlowNodeAddOn_PredicateCompareValues::TryCheckTextEqual(bool& bOutIsEqual) const
{
	// Compare both sides as Text; pin type templates should allow Name/String/Enum -> Text conversion.
	FText LeftText;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_Text>(GetDisambiguatedValueName(LeftValue), LeftText, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve LeftValue as Text."));
			return false;
		}
	}

	FText RightText;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_Text>(GetDisambiguatedValueName(RightValue), RightText, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve RightValue as Text."));
			return false;
		}
	}

	bOutIsEqual = LeftText.EqualTo(RightText);
	return true;
}

bool UFlowNodeAddOn_PredicateCompareValues::TryCheckStringEqual(bool& bOutIsEqual) const
{
	// Compare both sides as String; templates can handle Name/Text/Enum -> String if allowed.
	FString LeftString;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_String>(GetDisambiguatedValueName(LeftValue), LeftString, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve LeftValue as String."));
			return false;
		}
	}

	FString RightString;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_String>(GetDisambiguatedValueName(RightValue), RightString, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve RightValue as String."));
			return false;
		}
	}

	bOutIsEqual = (LeftString == RightString);
	return true;
}

bool UFlowNodeAddOn_PredicateCompareValues::TryCheckNameEqual(bool& bOutIsEqual) const
{
	// Compare case-insensitively if either side is Name-like.
	// We resolve both sides as String and compare IgnoreCase, because:
	// - FName itself is case-sensitive in operator==, but your requirement is case-insensitive for Name-like.
	// - FlowPinType templates can source Enum values (FName) into string as needed.
	FString LeftString;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_String>(GetDisambiguatedValueName(LeftValue), LeftString, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve LeftValue for Name-like comparison."));
			return false;
		}
	}

	FString RightString;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_String>(GetDisambiguatedValueName(RightValue), RightString, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve RightValue for Name-like comparison."));
			return false;
		}
	}

	bOutIsEqual = LeftString.Equals(RightString, ESearchCase::IgnoreCase);
	return true;
}

bool UFlowNodeAddOn_PredicateCompareValues::CompareDoubleUsingOperator(double LeftValueAsDouble, double RightValueAsDouble) const
{
	FLOW_ASSERT_ENUM_MAX(EFlowPredicateCompareOperatorType, 6);
	switch (OperatorType)
	{
	case EFlowPredicateCompareOperatorType::Equal:
		return FMath::IsNearlyEqual(LeftValueAsDouble, RightValueAsDouble, static_cast<double>(SMALL_NUMBER));

	case EFlowPredicateCompareOperatorType::NotEqual:
		return !FMath::IsNearlyEqual(LeftValueAsDouble, RightValueAsDouble, static_cast<double>(SMALL_NUMBER));

	case EFlowPredicateCompareOperatorType::Less:
		return (LeftValueAsDouble < RightValueAsDouble);

	case EFlowPredicateCompareOperatorType::LessOrEqual:
		return (LeftValueAsDouble <= RightValueAsDouble + static_cast<double>(SMALL_NUMBER));

	case EFlowPredicateCompareOperatorType::Greater:
		return (LeftValueAsDouble > RightValueAsDouble);

	case EFlowPredicateCompareOperatorType::GreaterOrEqual:
		return (LeftValueAsDouble >= RightValueAsDouble - static_cast<double>(SMALL_NUMBER));

	default:
		break;
	}

	return false;
}

bool UFlowNodeAddOn_PredicateCompareValues::CompareInt64UsingOperator(int64 LeftValueAsInt64, int64 RightValueAsInt64) const
{
	FLOW_ASSERT_ENUM_MAX(EFlowPredicateCompareOperatorType, 6);
	switch (OperatorType)
	{
	case EFlowPredicateCompareOperatorType::Equal:
		return (LeftValueAsInt64 == RightValueAsInt64);

	case EFlowPredicateCompareOperatorType::NotEqual:
		return (LeftValueAsInt64 != RightValueAsInt64);

	case EFlowPredicateCompareOperatorType::Less:
		return (LeftValueAsInt64 < RightValueAsInt64);

	case EFlowPredicateCompareOperatorType::LessOrEqual:
		return (LeftValueAsInt64 <= RightValueAsInt64);

	case EFlowPredicateCompareOperatorType::Greater:
		return (LeftValueAsInt64 > RightValueAsInt64);

	case EFlowPredicateCompareOperatorType::GreaterOrEqual:
		return (LeftValueAsInt64 >= RightValueAsInt64);

	default:
		break;
	}

	return false;
}

const FName& UFlowNodeAddOn_PredicateCompareValues::GetLeftValuePropertyName() const
{
	static const FName LeftValueName = GET_MEMBER_NAME_CHECKED(ThisClass, LeftValue);
	return LeftValueName;
}

const FName& UFlowNodeAddOn_PredicateCompareValues::GetRightValuePropertyName() const
{
	static const FName RightValueName = GET_MEMBER_NAME_CHECKED(ThisClass, RightValue);
	return RightValueName;
}

bool UFlowNodeAddOn_PredicateCompareValues::TryCompareAsDouble() const
{
	double LeftDouble = 0.0;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_Double>(GetDisambiguatedValueName(LeftValue), LeftDouble, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve LeftValue as Double."));
			return false;
		}
	}

	double RightDouble = 0.0;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_Double>(GetDisambiguatedValueName(RightValue), RightDouble, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve RightValue as Double."));
			return false;
		}
	}

	return CompareDoubleUsingOperator(LeftDouble, RightDouble);
}

bool UFlowNodeAddOn_PredicateCompareValues::TryCompareAsInt64() const
{
	int64 LeftInt64 = 0;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_Int64>(GetDisambiguatedValueName(LeftValue), LeftInt64, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve LeftValue as Int64."));
			return false;
		}
	}

	int64 RightInt64 = 0;
	{
		const EFlowDataPinResolveResult ResolveResult =
			TryResolveDataPinValue<FFlowPinType_Int64>(GetDisambiguatedValueName(RightValue), RightInt64, SingleFromArray);

		if (!FlowPinType::IsSuccess(ResolveResult))
		{
			LogError(TEXT("Failed to resolve RightValue as Int64."));
			return false;
		}
	}

	return CompareInt64UsingOperator(LeftInt64, RightInt64);
}

bool UFlowNodeAddOn_PredicateCompareValues::EvaluatePredicate_Implementation() const
{
	// All failures are errors and return false.
	if (!LeftValue.IsValid() || !RightValue.IsValid())
	{
		LogError(TEXT("Compare Values requires both LeftValue and RightValue to be configured."));
		return false;
	}

	const FName LeftTypeName = LeftValue.DataPinValue.Get().GetPinTypeName().Name;
	const FName RightTypeName = RightValue.DataPinValue.Get().GetPinTypeName().Name;

	if (!AreComparableStandardPinTypes(LeftTypeName, RightTypeName))
	{
		LogError(FString::Printf(
			TEXT("Compare Values pin types are not comparable: '%s' vs '%s'."),
			*LeftTypeName.ToString(),
			*RightTypeName.ToString()));

		return false;
	}

	// Arithmetic operators: numeric only
	if (IsArithmeticOp())
	{
		if (!(IsNumericTypeName(LeftTypeName) && IsNumericTypeName(RightTypeName)))
		{
			LogError(TEXT("Arithmetic operators are only supported for numeric pin types (Int/Int64/Float/Double)."));
			return false;
		}
	}

	// Numeric
	if (IsNumericTypeName(LeftTypeName) && IsNumericTypeName(RightTypeName))
	{
		// Prefer Int64 if both are integer types (or can be upscaled to int64 precisely).
		// Use Double if either side is floating point.
		if (IsFloatingPointType(LeftTypeName) || IsFloatingPointType(RightTypeName))
		{
			return TryCompareAsDouble();
		}

		return TryCompareAsInt64();
	}

	// Gameplay tags: compare as container (superset). Equality ops only (as per enum).
	if (IsGameplayTagLikeTypeName(LeftTypeName) || IsGameplayTagLikeTypeName(RightTypeName))
	{
		if (!IsEqualityOp())
		{
			LogError(TEXT("Arithmetic operators are not supported for Gameplay Tags."));
			return false;
		}

		bool bIsEqual = false;
		if (!TryCheckGameplayTagsEqual(bIsEqual))
		{
			return false;
		}

		return (OperatorType == EFlowPredicateCompareOperatorType::Equal) == bIsEqual;
	}

	// String-like (including enums-as-names). Equality ops only.
	if (IsAnyStringLikeTypeName(LeftTypeName) || IsAnyStringLikeTypeName(RightTypeName))
	{
		if (!IsEqualityOp())
		{
			LogError(TEXT("Arithmetic operators are not supported for Name/Text/String/Enum comparisons."));
			return false;
		}

		// Order is significant:
		// 1) Name-like (Name OR Enum) => case-insensitive compare
		// 2) Text => FText equality (localized)
		// 3) String => FString equality
		bool bIsEqual = false;

		if (IsNameLikeType(LeftTypeName) || IsNameLikeType(RightTypeName))
		{
			if (!TryCheckNameEqual(bIsEqual))
			{
				return false;
			}
		}
		else if (IsTextType(LeftTypeName) || IsTextType(RightTypeName))
		{
			if (!TryCheckTextEqual(bIsEqual))
			{
				return false;
			}
		}
		else
		{
			if (!TryCheckStringEqual(bIsEqual))
			{
				return false;
			}
		}

		return (OperatorType == EFlowPredicateCompareOperatorType::Equal) == bIsEqual;
	}

	// TODO (gtaylor) Add Object, Class, InstancedStruct, Vector... etc. support
	LogError(FString::Printf(
		TEXT("Compare Values does not support comparing pin types '%s' and '%s' yet."),
		*LeftTypeName.ToString(),
		*RightTypeName.ToString()));

	return false;
}

#undef LOCTEXT_NAMESPACE
