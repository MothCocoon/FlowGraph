// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowDebuggerTypes.generated.h"

// It can represent any trait added on the specific node instance, i.e. breakpoint
USTRUCT()
struct FLOWDEBUGGER_API FFlowBreakpoint
{
	GENERATED_USTRUCT_BODY()

protected:
	/** Pin that the trait is placed on. Zero filled if this trait is for a node, not a pin */
	UPROPERTY()
	FGuid PinId;

	UPROPERTY()
	uint8 bEnabled : 1;
	
	uint8 bHit : 1;

public:
	FFlowBreakpoint()
		: bEnabled(false)
		, bHit(false)
	{
	};

	explicit FFlowBreakpoint(const bool bInitialState)
		: bEnabled(bInitialState)
		, bHit(false)
	{
	};

	explicit FFlowBreakpoint(const FGuid& InPinId, const bool bInitialState)
		: PinId(InPinId)
		, bEnabled(bInitialState)
		, bHit(false)
	{
	};

	void SetEnabled(const bool bNowEnabled) { bEnabled = bNowEnabled; }
	void MarkAsHit(const bool bNowHit) { bHit = bNowHit; }

	FGuid GetPinId() const { return PinId; }
	bool MatchesGuid(const FGuid& OtherGuid) const { return PinId == OtherGuid; }
	
	bool IsEnabled() const { return bEnabled; }
	bool IsHit() const { return bHit; }

	bool operator==(const FFlowBreakpoint& Other) const
	{
		return PinId == Other.PinId;
	}
};
