// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Policies/FlowPolicy.h"
#include "FlowPinTypeMatchPolicy.h"

#include "FlowPinConnectionPolicy.generated.h"

// Policy for Flow Pin type relationships.
//
// This struct serves as the domain's type system definition, consumed by:
// 1. The FlowGraphSchema — for pin connection compatibility in the editor
// 2. Runtime predicates (e.g., CompareValues) — for type classification and comparison dispatch
//
// Both consumers access the policy through UFlowAsset::GetFlowPinConnectionPolicy(),
// which allows per-asset-subclass customization of the type system.
USTRUCT()
struct FFlowPinConnectionPolicy : public FFlowPolicy
{
	GENERATED_BODY()

protected:
	/* These are the policies for matching data pin types. */
	UPROPERTY(EditAnywhere, Category = PinConnection, meta = (ShowOnlyInnerProperties))
	TMap<FName, FFlowPinTypeMatchPolicy> PinTypeMatchPolicies;

public:
	FFlowPinConnectionPolicy();

//////////////////////////////////////////////////////////////////////////
// Runtime-available queries (used by CompareValues predicate and others)

	FLOW_API const FFlowPinTypeMatchPolicy* TryFindPinTypeMatchPolicy(const FName& PinTypeName) const;

	// Simple connection test using only pin type names 
	// (more checks will be needed for actual pin connection testing in the Schema)
	FLOW_API bool CanConnectPinTypeNames(const FName& FromOutputPinTypeName, const FName& ToInputPinTypeName) const;

	FLOW_API virtual const TSet<FName>& GetAllSupportedTypes() const;
	FLOW_API virtual const TSet<FName>& GetAllSupportedIntegerTypes() const;
	FLOW_API virtual const TSet<FName>& GetAllSupportedFloatTypes() const;
	FLOW_API virtual const TSet<FName>& GetAllSupportedGameplayTagTypes() const;
	FLOW_API virtual const TSet<FName>& GetAllSupportedStringLikeTypes() const;
	FLOW_API virtual const TSet<FName>& GetAllSupportedSubCategoryObjectTypes() const;
	FLOW_API virtual const TSet<FName>& GetAllSupportedConvertibleToStringTypes() const;
	FLOW_API virtual const TSet<FName>& GetAllSupportedReceivingConvertToStringTypes() const;
	FLOW_API virtual EFlowPinTypeMatchRules GetPinTypeMatchRulesForType(const FName& PinTypeName) const;

//////////////////////////////////////////////////////////////////////////
// Policy configuration (editor-only, used to build PinTypeMatchPolicies)

#if WITH_EDITOR
	FLOW_API void ConfigurePolicy(
		bool bAllowAllTypesConvertibleToString,
		bool bAllowAllNumericsConvertible,
		bool bAllowAllTypeFamiliesConvertible);

	FLOW_API FORCEINLINE static void AddConnectablePinTypes(const TSet<FName>& PinTypeNames, const FName& PinTypeName, TSet<FName>& ConnectablePinCategories);
	FLOW_API FORCEINLINE static void AddConnectablePinTypesIfContains(const TSet<FName>& PinTypeNames, const FName& PinTypeName, TSet<FName>& ConnectablePinCategories);
	FLOW_API FORCEINLINE static TSet<FName> BuildSetExcludingName(const TSet<FName>& NamesSet, const FName& NameToExclude);
#endif
};

#if WITH_EDITOR
// Inline implementations
void FFlowPinConnectionPolicy::AddConnectablePinTypes(const TSet<FName>& PinTypeNames, const FName& PinTypeName, TSet<FName>& ConnectablePinCategories)
{
	ConnectablePinCategories.Append(BuildSetExcludingName(PinTypeNames, PinTypeName));
}

void FFlowPinConnectionPolicy::AddConnectablePinTypesIfContains(const TSet<FName>& PinTypeNames, const FName& PinTypeName, TSet<FName>& ConnectablePinCategories)
{
	if (PinTypeNames.Contains(PinTypeName))
	{
		AddConnectablePinTypes(PinTypeNames, PinTypeName, ConnectablePinCategories);
	}
}

TSet<FName> FFlowPinConnectionPolicy::BuildSetExcludingName(const TSet<FName>& NamesSet, const FName& NameToExclude)
{
	TSet<FName> NewSet = NamesSet;
	NewSet.Remove(NameToExclude);
	return MoveTemp(NewSet);
}
#endif