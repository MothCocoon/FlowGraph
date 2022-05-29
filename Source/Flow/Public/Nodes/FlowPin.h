// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowPin.generated.h"

USTRUCT()
struct FLOW_API FFlowPin
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "FlowPin")
	FName PinName;

	UPROPERTY(EditDefaultsOnly, Category = "FlowPin")
	FString PinToolTip;

	FFlowPin()
		: PinName(NAME_None)
	{
	}

	FFlowPin(const FName& InPinName)
		: PinName(InPinName)
	{
	}

	FFlowPin(const FString& InPinName)
		: PinName(*InPinName)
	{
	}

	FFlowPin(const FText& InPinName)
        : PinName(*InPinName.ToString())
	{
	}

	FFlowPin(const TCHAR* InPinName)
		: PinName(FName(InPinName))
	{
	}

	FFlowPin(const uint8& InPinName)
		: PinName(FName(*FString::FromInt(InPinName)))
	{
	}

	FFlowPin(const int32& InPinName)
		: PinName(FName(*FString::FromInt(InPinName)))
	{
	}

	FFlowPin(const FName& InPinName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinToolTip(InPinTooltip)
	{
	}

	FFlowPin(const FString& InPinName, const FString& InPinTooltip)
		: PinName(*InPinName)
		, PinToolTip(InPinTooltip)
	{
	}

	FFlowPin(const FText& InPinName, const FString& InPinTooltip)
        : PinName(*InPinName.ToString())
        , PinToolTip(InPinTooltip)
	{
	}

	FFlowPin(const TCHAR* InPinName, const FString& InPinTooltip)
		: PinName(FName(InPinName))
		, PinToolTip(InPinTooltip)
	{
	}
	
	FORCEINLINE bool IsValid() const
	{
		return !PinName.IsNone();
	}

	FORCEINLINE bool operator==(const FFlowPin& Other) const
	{
		return PinName == Other.PinName;
	}

	FORCEINLINE bool operator!=(const FFlowPin& Other) const
	{
		return PinName != Other.PinName;
	}

	FORCEINLINE bool operator==(const FName& Other) const
	{
		return PinName == Other;
	}

	FORCEINLINE bool operator!=(const FName& Other) const
	{
		return PinName != Other;
	}

	friend uint32 GetTypeHash(const FFlowPin& FlowPin)
	{
		return GetTypeHash(FlowPin.PinName);
	}
};

// Processing Flow Nodes creates map of connected pins
USTRUCT()
struct FLOW_API FConnectedPin
{
	GENERATED_USTRUCT_BODY()

    UPROPERTY()
	FGuid NodeGuid;

	UPROPERTY()
	FName PinName;

	FConnectedPin()
        : NodeGuid(FGuid())
        , PinName(NAME_None)
	{
	}

	FConnectedPin(const FGuid InNodeId, const FName& InPinName)
        : NodeGuid(InNodeId)
        , PinName(InPinName)
	{
	}

	FORCEINLINE bool operator==(const FConnectedPin& Other) const
	{
		return NodeGuid == Other.NodeGuid && PinName == Other.PinName;
	}

	FORCEINLINE bool operator!=(const FConnectedPin& Other) const
	{
		return NodeGuid != Other.NodeGuid || PinName != Other.PinName;
	}

	friend uint32 GetTypeHash(const FConnectedPin& ConnectedPin)
	{
		return GetTypeHash(ConnectedPin.NodeGuid) + GetTypeHash(ConnectedPin.PinName);
	}
};

// Every time pin is activated, we record it and display this data while user hovers mouse over pin
#if !UE_BUILD_SHIPPING
struct FLOW_API FPinRecord
{	
	double Time;
	FString HumanReadableTime;
	bool bForcedActivation;

	static FString NoActivations;
	static FString PinActivations;
	static FString ForcedActivation;

	FPinRecord();
	FPinRecord(const double InTime, const bool bInForcedActivation);

	private:
	FORCEINLINE static FString DoubleDigit(const int32 Number);
};
#endif
