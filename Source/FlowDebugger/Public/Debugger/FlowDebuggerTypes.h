// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowDebuggerTypes.generated.h"

UENUM()
enum class EFlowTraitType
{
	Breakpoint, // default trait type

	// ^ Add new trait types above here ^
	Max
};

ENUM_RANGE_BY_COUNT(EFlowTraitType, EFlowTraitType::Max)

// It can represent any trait added on the specific node instance, i.e. breakpoint
USTRUCT()
struct FLOWDEBUGGER_API FFlowDebugTrait
{
	GENERATED_USTRUCT_BODY()

protected:
	/** Pin that the trait is placed on. Zero filled if this trait is for a node, not a pin */
	UPROPERTY()
	FGuid PinId;

	UPROPERTY()
	EFlowTraitType Type;

	UPROPERTY()
	uint8 bEnabled : 1;
	uint8 bHit : 1;

public:
	FFlowDebugTrait()
		: Type(EFlowTraitType::Breakpoint) // default trait type
		, bEnabled(false)
		, bHit(false)
	{
	};

	explicit FFlowDebugTrait(const EFlowTraitType InType, const bool bInitialState)
		: Type(InType)
		, bEnabled(bInitialState)
		, bHit(false)
	{
	};

	explicit FFlowDebugTrait(const EFlowTraitType InType, const FGuid InPinId, const bool bInitialState)
		: PinId(InPinId)
		, Type(InType)
		, bEnabled(bInitialState)
		, bHit(false)
	{
	};

	bool IsEnabled() const { return bEnabled; }
	bool IsHit() const { return bHit; }

	bool operator==(const FFlowDebugTrait& Other) const
	{
		return Type == Other.Type && PinId == Other.PinId;
	}

	friend class UFlowDebuggerSubsystem;
};

USTRUCT()
struct FLOWDEBUGGER_API FFlowTraitSettings
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FFlowDebugTrait> NodeTraits;

	UPROPERTY()
	TArray<FFlowDebugTrait> PinTraits;

	bool operator==(const FFlowTraitSettings& Other) const
	{
		return NodeTraits == Other.NodeTraits && PinTraits == Other.PinTraits;
	}
};
