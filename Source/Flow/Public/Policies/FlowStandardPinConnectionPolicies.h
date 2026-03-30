// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Policies/FlowPinConnectionPolicy.h"

#include "FlowStandardPinConnectionPolicies.generated.h"

/* A very relaxed policy that allows maximum data-pin connectivity lenience */
USTRUCT()
struct FFlowPinConnectionPolicy_VeryRelaxed : public FFlowPinConnectionPolicy
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	FFlowPinConnectionPolicy_VeryRelaxed()
	{
		// Cross-conversion rules:
		// - Most* types → String (one-way)  (*except InstancedStruct)
		// - Numeric: full bidirectional conversion
		// - Name/String/Text: full bidirectional
		// - GameplayTag ↔ Container: bidirectional
		constexpr bool bAllowAllTypesConvertibleToString = true;
		constexpr bool bAllowAllNumericsConvertible = true;
		constexpr bool bAllowAllTypeFamiliesConvertible = true;

		ConfigurePolicy(
			bAllowAllTypesConvertibleToString,
			bAllowAllNumericsConvertible,
			bAllowAllTypeFamiliesConvertible);
	}
#endif
};

/* A moderately relaxed policy that allows reasonable data-pin connectivity lenience */
USTRUCT()
struct FFlowPinConnectionPolicy_Relaxed : public FFlowPinConnectionPolicy
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	FFlowPinConnectionPolicy_Relaxed()
	{
		// Cross-conversion rules:
		// - Most* types → String (one-way)  (*except InstancedStruct)
		// - Int/Float/Name/String/Text: full bidirectional
		// - GameplayTag ↔ Container: bidirectional
		constexpr bool bAllowAllTypesConvertibleToString = true;
		constexpr bool bAllowAllNumericsConvertible = false;
		constexpr bool bAllowAllTypeFamiliesConvertible = true;

		ConfigurePolicy(
			bAllowAllTypesConvertibleToString,
			bAllowAllNumericsConvertible,
			bAllowAllTypeFamiliesConvertible);
	}
#endif
};

/* A strict policy that allows no cross-type connectivity (except to string, for dev purposes) */
USTRUCT()
struct FFlowPinConnectionPolicy_Strict : public FFlowPinConnectionPolicy
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	FFlowPinConnectionPolicy_Strict()
	{
		// Cross-conversion rules:
		// - Most* types → String (one-way)  (*except InstancedStruct)
		constexpr bool bAllowAllTypesConvertibleToString = true;
		constexpr bool bAllowAllNumericsConvertible = false;
		constexpr bool bAllowAllTypeFamiliesConvertible = false;

		ConfigurePolicy(
			bAllowAllTypesConvertibleToString,
			bAllowAllNumericsConvertible,
			bAllowAllTypeFamiliesConvertible);
	}
#endif
};

/* A strict policy that allows no cross-type connectivity at all */
USTRUCT()
struct FFlowPinConnectionPolicy_VeryStrict : public FFlowPinConnectionPolicy
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	FFlowPinConnectionPolicy_VeryStrict()
	{
		constexpr bool bAllowAllTypesConvertibleToString = false;
		constexpr bool bAllowAllNumericsConvertible = false;
		constexpr bool bAllowAllTypeFamiliesConvertible = false;

		ConfigurePolicy(
			bAllowAllTypesConvertibleToString,
			bAllowAllNumericsConvertible,
			bAllowAllTypeFamiliesConvertible);
	}
#endif
};
