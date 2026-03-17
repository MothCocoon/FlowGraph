// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Policies/FlowPinConnectionPolicy.h"
#include "Nodes/FlowPin.h"
#include "Types/FlowPinTypeNamesStandard.h"
#include "FlowAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowPinConnectionPolicy)

FFlowPinConnectionPolicy::FFlowPinConnectionPolicy()
{
}

const FFlowPinTypeMatchPolicy* FFlowPinConnectionPolicy::TryFindPinTypeMatchPolicy(const FName& PinTypeName) const
{
	return PinTypeMatchPolicies.Find(PinTypeName);
}

bool FFlowPinConnectionPolicy::CanConnectPinTypeNames(const FName& FromOutputPinTypeName, const FName& ToInputPinTypeName) const
{
	const bool bIsInputExecPin = FFlowPin::IsExecPinCategory(ToInputPinTypeName);
	const bool bIsOutputExecPin = FFlowPin::IsExecPinCategory(FromOutputPinTypeName);
	if (bIsInputExecPin || bIsOutputExecPin)
	{
		// Exec pins must match exactly (exec ↔ exec only).
		return (bIsInputExecPin && bIsOutputExecPin);
	}

	const FFlowPinTypeMatchPolicy* FoundPinTypeMatchPolicy = TryFindPinTypeMatchPolicy(ToInputPinTypeName);
	if (!FoundPinTypeMatchPolicy)
	{
		// Could not find PinTypeMatchPolicy for ToInputPinTypeName.
		return false;
	}

	// PinCategories must match exactly or be in the map of compatible PinCategories for the input pin type
	const bool bRequirePinCategoryMatch =
		EnumHasAnyFlags(FoundPinTypeMatchPolicy->PinTypeMatchRules, EFlowPinTypeMatchRules::RequirePinCategoryMatch);

	if (bRequirePinCategoryMatch &&
		FromOutputPinTypeName != ToInputPinTypeName &&
		!FoundPinTypeMatchPolicy->PinCategories.Contains(FromOutputPinTypeName))
	{
		// Pin type mismatch FromOutputPinTypeName != ToInputPinTypeName (and not in compatible categories list).
		return false;
	}

	return true;
}

const TSet<FName>& FFlowPinConnectionPolicy::GetAllSupportedTypes() const
{
	return FFlowPinTypeNamesStandard::AllStandardTypeNames;
}

const TSet<FName>& FFlowPinConnectionPolicy::GetAllSupportedIntegerTypes() const
{
	return FFlowPinTypeNamesStandard::AllStandardIntegerTypeNames;
}

const TSet<FName>& FFlowPinConnectionPolicy::GetAllSupportedFloatTypes() const
{
	return FFlowPinTypeNamesStandard::AllStandardFloatTypeNames;
}

const TSet<FName>& FFlowPinConnectionPolicy::GetAllSupportedGameplayTagTypes() const
{
	return FFlowPinTypeNamesStandard::AllStandardGameplayTagTypeNames;
}

const TSet<FName>& FFlowPinConnectionPolicy::GetAllSupportedStringLikeTypes() const
{
	return FFlowPinTypeNamesStandard::AllStandardStringLikeTypeNames;
}

const TSet<FName>& FFlowPinConnectionPolicy::GetAllSupportedSubCategoryObjectTypes() const
{
	return FFlowPinTypeNamesStandard::AllStandardSubCategoryObjectTypeNames;
}

const TSet<FName>& FFlowPinConnectionPolicy::GetAllSupportedConvertibleToStringTypes() const
{
	// By default, all types are convertible to string
	return GetAllSupportedTypes();
}

const TSet<FName>& FFlowPinConnectionPolicy::GetAllSupportedReceivingConvertToStringTypes() const
{
	// Only allowing to convert to String type specifically by default.
	// Subclasses could choose different or additional type(s) for the ConvertibleToString conversion
	static const TSet<FName> OnlyStringType = { FFlowPinTypeNamesStandard::PinTypeNameString };
	return OnlyStringType;
}

EFlowPinTypeMatchRules FFlowPinConnectionPolicy::GetPinTypeMatchRulesForType(const FName& PinTypeName) const
{
	const TSet<FName>& SubCategoryObjectTypes = GetAllSupportedSubCategoryObjectTypes();
	if (SubCategoryObjectTypes.Contains(PinTypeName))
	{
		return EFlowPinTypeMatchRules::SubCategoryObjectPinTypeMatchRulesMask;
	}
	else
	{
		return EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask;
	}
}

#if WITH_EDITOR

void FFlowPinConnectionPolicy::ConfigurePolicy(
	bool bAllowAllTypesConvertibleToString,
	bool bAllowAllNumericsConvertible,
	bool bAllowAllTypeFamiliesConvertible)
{
	PinTypeMatchPolicies.Reset();

	const TSet<FName>& AllSupportedTypes = GetAllSupportedTypes();
	const TSet<FName>& AllGameplayTagTypes= GetAllSupportedGameplayTagTypes();
	const TSet<FName>& AllSubCategoryObjectTypes = GetAllSupportedSubCategoryObjectTypes();
	const TSet<FName>& AllStringLikeTypes = GetAllSupportedStringLikeTypes();
	const TSet<FName>& AllConvertibleToStringTypes = GetAllSupportedConvertibleToStringTypes();
	const TSet<FName>& AllReceivingConvertToStringTypes = GetAllSupportedReceivingConvertToStringTypes();
	const TSet<FName>& AllIntegerTypes = GetAllSupportedIntegerTypes();
	const TSet<FName>& AllFloatTypes = GetAllSupportedFloatTypes();
	TSet<FName> AllNumericTypes = AllIntegerTypes;
	AllNumericTypes.Append(AllFloatTypes);

	TSet<FName> ConnectablePinCategories;

	for (const FName& PinTypeName : AllSupportedTypes)
	{
		const EFlowPinTypeMatchRules PinTypeMatchRules = GetPinTypeMatchRulesForType(PinTypeName);

		ConnectablePinCategories.Reset();

		// Add support for AllowAllTypesConvertibleToString
		if (bAllowAllTypesConvertibleToString &&
			AllReceivingConvertToStringTypes.Contains(PinTypeName))
		{
			AddConnectablePinTypes(AllConvertibleToStringTypes, PinTypeName, ConnectablePinCategories);
		}

		// Add support for numeric type conversion
		if (bAllowAllNumericsConvertible)
		{
			AddConnectablePinTypesIfContains(AllNumericTypes, PinTypeName, ConnectablePinCategories);
		}

		if (bAllowAllTypeFamiliesConvertible)
		{
			// The type families are: Integer, Float, GameplayTag and String-Like
			AddConnectablePinTypesIfContains(AllIntegerTypes, PinTypeName, ConnectablePinCategories);
			AddConnectablePinTypesIfContains(AllFloatTypes, PinTypeName, ConnectablePinCategories);
			AddConnectablePinTypesIfContains(AllGameplayTagTypes, PinTypeName, ConnectablePinCategories);
			AddConnectablePinTypesIfContains(AllStringLikeTypes, PinTypeName, ConnectablePinCategories);
		}

		// Add the entry for this PinTypeName to the match policies map
		PinTypeMatchPolicies.Add(
			PinTypeName,
			FFlowPinTypeMatchPolicy(
				PinTypeMatchRules,
				ConnectablePinCategories));
	}
}

#endif