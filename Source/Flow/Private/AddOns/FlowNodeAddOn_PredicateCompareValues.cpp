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

bool UFlowNodeAddOn_PredicateCompareValues::IsBoolTypeName(const FName& TypeName)
{
	return TypeName == FFlowPinTypeNamesStandard::PinTypeNameBool;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsVectorTypeName(const FName& TypeName)
{
	return TypeName == FFlowPinTypeNamesStandard::PinTypeNameVector;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsRotatorTypeName(const FName& TypeName)
{
	return TypeName == FFlowPinTypeNamesStandard::PinTypeNameRotator;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsTransformTypeName(const FName& TypeName)
{
	return TypeName == FFlowPinTypeNamesStandard::PinTypeNameTransform;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsObjectTypeName(const FName& TypeName)
{
	return TypeName == FFlowPinTypeNamesStandard::PinTypeNameObject;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsClassTypeName(const FName& TypeName)
{
	return TypeName == FFlowPinTypeNamesStandard::PinTypeNameClass;
}

bool UFlowNodeAddOn_PredicateCompareValues::IsInstancedStructTypeName(const FName& TypeName)
{
	return TypeName == FFlowPinTypeNamesStandard::PinTypeNameInstancedStruct;
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

EDataValidationResult UFlowNodeAddOn_PredicateCompareValues::ValidateNode()
{
	EDataValidationResult Result = Super::ValidateNode();

	// Validate that both values are configured
	if (!LeftValue.IsValid())
	{
		LogValidationError(TEXT("LeftValue is not configured (missing name or pin type)."));
		Result = EDataValidationResult::Invalid;
	}

	if (!RightValue.IsValid())
	{
		LogValidationError(TEXT("RightValue is not configured (missing name or pin type)."));
		Result = EDataValidationResult::Invalid;
	}

	// Remaining checks require both values to be valid
	if (!LeftValue.IsValid() || !RightValue.IsValid())
	{
		return Result;
	}

	const FFlowPinTypeName LeftPinTypeName = LeftValue.DataPinValue.Get().GetPinTypeName();
	const FFlowPinTypeName RightPinTypeName = RightValue.DataPinValue.Get().GetPinTypeName();

	// Validate pin type names are set
	if (LeftPinTypeName.IsNone())
	{
		LogValidationError(TEXT("LeftValue has an unknown or unset pin type."));
		Result = EDataValidationResult::Invalid;
	}

	if (RightPinTypeName.IsNone())
	{
		LogValidationError(TEXT("RightValue has an unknown or unset pin type."));
		Result = EDataValidationResult::Invalid;
	}

	if (LeftPinTypeName.IsNone() || RightPinTypeName.IsNone())
	{
		return Result;
	}

	// Check type compatibility
	const FName LeftTypeName = LeftPinTypeName.Name;
	const FName RightTypeName = RightPinTypeName.Name;

	const bool bSameType = (LeftTypeName == RightTypeName);

	if (!bSameType && !AreComparableStandardPinTypes(LeftTypeName, RightTypeName))
	{
		LogValidationError(FString::Printf(
			TEXT("Pin types are not comparable: '%s' vs '%s'."),
			*LeftTypeName.ToString(),
			*RightTypeName.ToString()));
		Result = EDataValidationResult::Invalid;
	}

	// Validate arithmetic operators are only used with numeric types
	if (IsArithmeticOp() && !(IsNumericTypeName(LeftTypeName) && IsNumericTypeName(RightTypeName)))
	{
		LogValidationError(FString::Printf(
			TEXT("Arithmetic operator '%s' is only supported for numeric pin types (Int/Int64/Float/Double). Current types: '%s' vs '%s'."),
			*EFlowPredicateCompareOperatorType_Classifiers::GetOperatorSymbolString(OperatorType),
			*LeftTypeName.ToString(),
			*RightTypeName.ToString()));
		Result = EDataValidationResult::Invalid;
	}

	// Warn if both sides have the same authored name (potential user confusion)
	if (GetAuthoredValueName(LeftValue) == GetAuthoredValueName(RightValue))
	{
		LogValidationWarning(FString::Printf(
			TEXT("LeftValue and RightValue have the same name '%s'. This may cause confusion with pin disambiguation."),
			*GetAuthoredValueName(LeftValue).ToString()));
	}

	if (Result == EDataValidationResult::NotValidated)
	{
		Result = EDataValidationResult::Valid;
	}

	return Result;
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

	// Note: Bool, Vector, Rotator, Transform, Object, Class, InstancedStruct are all
	// only comparable with themselves (handled by the LeftPinTypeName == RightPinTypeName check above).
	// Unknown/user types also fall into same-type comparison via the fallback path in EvaluatePredicate.

	return false;
}

bool UFlowNodeAddOn_PredicateCompareValues::CacheTypeNames(FCachedTypeNames& OutCache) const
{
	OutCache.Reset();

	if (!LeftValue.IsValid() || !RightValue.IsValid())
	{
		LogError(TEXT("Compare Values requires both LeftValue and RightValue to be configured."));
		return false;
	}

	OutCache.LeftTypeName = LeftValue.DataPinValue.Get().GetPinTypeName().Name;
	OutCache.RightTypeName = RightValue.DataPinValue.Get().GetPinTypeName().Name;
	OutCache.bIsValid = true;

	return true;
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

bool UFlowNodeAddOn_PredicateCompareValues::TryCheckFallbackStringEqual(bool& bOutIsEqual) const
{
	// Fallback path: try to convert both sides to string via their FFlowDataPinValue::TryConvertValuesToString.
	// This enables user-added pin types (from other plugins) to participate in equality comparisons
	// as long as they implement TryConvertValuesToString on their FFlowDataPinValue subclass.

	const FFlowDataPinValue* LeftDataPinValue = LeftValue.DataPinValue.GetPtr<FFlowDataPinValue>();
	const FFlowDataPinValue* RightDataPinValue = RightValue.DataPinValue.GetPtr<FFlowDataPinValue>();

	if (!LeftDataPinValue || !RightDataPinValue)
	{
		return false;
	}

	FString LeftString;
	if (!LeftDataPinValue->TryConvertValuesToString(LeftString))
	{
		LogError(TEXT("Failed to convert LeftValue to String for fallback comparison."));
		return false;
	}

	FString RightString;
	if (!RightDataPinValue->TryConvertValuesToString(RightString))
	{
		LogError(TEXT("Failed to convert RightValue to String for fallback comparison."));
		return false;
	}

	bOutIsEqual = (LeftString == RightString);
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

bool UFlowNodeAddOn_PredicateCompareValues::EvaluateEqualityBlock(const TCHAR* TypeLabel, const TFunctionRef<bool(bool& /*bOutIsEqual*/)> CompareFunc) const
{
	if (!IsEqualityOp())
	{
		LogError(FString::Printf(TEXT("Arithmetic operators are not supported for %s comparisons."), TypeLabel));
		return false;
	}

	bool bIsEqual = false;
	if (!CompareFunc(bIsEqual))
	{
		return false;
	}

	return (OperatorType == EFlowPredicateCompareOperatorType::Equal) == bIsEqual;
}

bool UFlowNodeAddOn_PredicateCompareValues::EvaluatePredicate_Implementation() const
{
	// Cache type names once to avoid repeated TInstancedStruct::Get() virtual dispatch.
	FCachedTypeNames Cache;
	if (!CacheTypeNames(Cache))
	{
		return false;
	}

	const FName& LeftTypeName = Cache.LeftTypeName;
	const FName& RightTypeName = Cache.RightTypeName;

	const bool bSameType = (LeftTypeName == RightTypeName);

	// Type compatibility gate.
	// Same-type unknowns are allowed through for the fallback path at the bottom.
	if (!bSameType && !AreComparableStandardPinTypes(LeftTypeName, RightTypeName))
	{
		LogError(FString::Printf(
			TEXT("Compare Values pin types are not comparable: '%s' vs '%s'."),
			*LeftTypeName.ToString(),
			*RightTypeName.ToString()));

		return false;
	}

	// Arithmetic operators: numeric only (fast reject before the cascade)
	if (IsArithmeticOp() && !(IsNumericTypeName(LeftTypeName) && IsNumericTypeName(RightTypeName)))
	{
		LogError(TEXT("Arithmetic operators are only supported for numeric pin types (Int/Int64/Float/Double)."));
		return false;
	}

	// Numeric (full operator set)
	if (IsNumericTypeName(LeftTypeName) && IsNumericTypeName(RightTypeName))
	{
		if (IsFloatingPointType(LeftTypeName) || IsFloatingPointType(RightTypeName))
		{
			return TryCompareAsDouble();
		}

		return TryCompareAsInt64();
	}

	// Gameplay tags: compare as container (superset). Equality ops only.
	if (IsGameplayTagLikeTypeName(LeftTypeName) || IsGameplayTagLikeTypeName(RightTypeName))
	{
		return EvaluateEqualityBlock(TEXT("Gameplay Tag"),
			[this](bool& bIsEqual) { return TryCheckGameplayTagsEqual(bIsEqual); });
	}

	// String-like (including enums-as-names). Equality ops only.
	if (IsAnyStringLikeTypeName(LeftTypeName) || IsAnyStringLikeTypeName(RightTypeName))
	{
		// Dispatch order is significant:
		// 1) Name-like (Name OR Enum) => case-insensitive compare via FString
		// 2) Text => FText::EqualTo (culture-aware)
		// 3) String => exact FString equality
		if (IsNameLikeType(LeftTypeName) || IsNameLikeType(RightTypeName))
		{
			return EvaluateEqualityBlock(TEXT("Name/Enum"),
				[this](bool& bIsEqual)
				{
					return TryCheckResolvedValuesEqual<FFlowPinType_String>(bIsEqual, TEXT("String (Name-like)"),
						[](const FString& L, const FString& R) { return L.Equals(R, ESearchCase::IgnoreCase); });
				});
		}

		if (IsTextType(LeftTypeName) || IsTextType(RightTypeName))
		{
			return EvaluateEqualityBlock(TEXT("Text"),
				[this](bool& bIsEqual)
				{
					return TryCheckResolvedValuesEqual<FFlowPinType_Text>(bIsEqual, TEXT("Text"),
						[](const FText& L, const FText& R) { return L.EqualTo(R); });
				});
		}

		return EvaluateEqualityBlock(TEXT("String"),
			[this](bool& bIsEqual)
			{
				return TryCheckResolvedValuesEqual<FFlowPinType_String>(bIsEqual, TEXT("String"));
			});
	}

	// Bool. Equality ops only.
	if (IsBoolTypeName(LeftTypeName) && IsBoolTypeName(RightTypeName))
	{
		return EvaluateEqualityBlock(TEXT("Bool"),
			[this](bool& bIsEqual) { return TryCheckResolvedValuesEqual<FFlowPinType_Bool>(bIsEqual, TEXT("Bool")); });
	}

	// Vector. Equality ops only, strict comparison (no tolerance).
	if (IsVectorTypeName(LeftTypeName) && IsVectorTypeName(RightTypeName))
	{
		return EvaluateEqualityBlock(TEXT("Vector"),
			[this](bool& bIsEqual) { return TryCheckResolvedValuesEqual<FFlowPinType_Vector>(bIsEqual, TEXT("Vector")); });
	}

	// Rotator. Equality ops only, strict comparison (no tolerance).
	if (IsRotatorTypeName(LeftTypeName) && IsRotatorTypeName(RightTypeName))
	{
		return EvaluateEqualityBlock(TEXT("Rotator"),
			[this](bool& bIsEqual) { return TryCheckResolvedValuesEqual<FFlowPinType_Rotator>(bIsEqual, TEXT("Rotator")); });
	}

	// Transform. Equality ops only, strict comparison (zero tolerance).
	if (IsTransformTypeName(LeftTypeName) && IsTransformTypeName(RightTypeName))
	{
		return EvaluateEqualityBlock(TEXT("Transform"),
			[this](bool& bIsEqual)
			{
				return TryCheckResolvedValuesEqual<FFlowPinType_Transform>(bIsEqual, TEXT("Transform"),
					[](const FTransform& L, const FTransform& R) { return L.Equals(R, 0.0); });
			});
	}

	// Object. Equality ops only, pointer identity.
	if (IsObjectTypeName(LeftTypeName) && IsObjectTypeName(RightTypeName))
	{
		return EvaluateEqualityBlock(TEXT("Object"),
			[this](bool& bIsEqual) { return TryCheckResolvedValuesEqual<FFlowPinType_Object>(bIsEqual, TEXT("Object")); });
	}

	// Class. Equality ops only, strict class identity (not "is derived from").
	if (IsClassTypeName(LeftTypeName) && IsClassTypeName(RightTypeName))
	{
		return EvaluateEqualityBlock(TEXT("Class"),
			[this](bool& bIsEqual) { return TryCheckResolvedValuesEqual<FFlowPinType_Class>(bIsEqual, TEXT("Class")); });
	}

	// InstancedStruct. Equality ops only, struct type + data equality.
	if (IsInstancedStructTypeName(LeftTypeName) && IsInstancedStructTypeName(RightTypeName))
	{
		return EvaluateEqualityBlock(TEXT("InstancedStruct"),
			[this](bool& bIsEqual) { return TryCheckResolvedValuesEqual<FFlowPinType_InstancedStruct>(bIsEqual, TEXT("InstancedStruct")); });
	}

	// Fallback: same-type comparison via string conversion.
	// This supports user-added types from other plugins as long as they
	// implement TryConvertValuesToString on their FFlowDataPinValue subclass.
	if (bSameType)
	{
		return EvaluateEqualityBlock(*LeftTypeName.ToString(),
			[this](bool& bIsEqual) { return TryCheckFallbackStringEqual(bIsEqual); });
	}

	LogError(FString::Printf(
		TEXT("Compare Values does not support comparing pin types '%s' and '%s'."),
		*LeftTypeName.ToString(),
		*RightTypeName.ToString()));

	return false;
}

#undef LOCTEXT_NAMESPACE
