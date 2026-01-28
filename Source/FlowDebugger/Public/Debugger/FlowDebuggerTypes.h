// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowDebuggerTypes.generated.h"

USTRUCT()
struct FLOWDEBUGGER_API FFlowBreakpoint
{
	GENERATED_BODY()

protected:
	// Applies only to node breakpoint
	// Pin breakpoints are deactivated by removing element from FNodeBreakpoint::PinBreakpoints
	UPROPERTY()
	bool bActive;

	UPROPERTY()
	uint8 bEnabled : 1;

	UPROPERTY(Transient)
	uint8 bHit : 1;

public:
	FFlowBreakpoint()
		: bActive(false)
		, bEnabled(false)
		, bHit(false)
	{
	};

	void SetActive(const bool bNowActive)
	{
		bActive = bNowActive;
		bEnabled = bNowActive;
	}

	void SetEnabled(const bool bNowEnabled)
	{
		bEnabled = bNowEnabled;
	}

	void MarkAsHit(const bool bNowHit)
	{
		bHit = bNowHit;
	}

	bool IsActive() const { return bActive; }
	bool IsEnabled() const { return bEnabled; }
	bool IsHit() const { return bHit; }
};

USTRUCT()
struct FLOWDEBUGGER_API FNodeBreakpoint
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FFlowBreakpoint Breakpoint;

	UPROPERTY()
	TMap<FName, FFlowBreakpoint> PinBreakpoints;

	FNodeBreakpoint()
	{
	};

	bool IsEmpty() const
	{
		return !Breakpoint.IsActive() && PinBreakpoints.IsEmpty();
	}
};
