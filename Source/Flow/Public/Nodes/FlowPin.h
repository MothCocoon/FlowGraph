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

USTRUCT()
struct FLOW_API FFlowInputOutputPin
{
	GENERATED_USTRUCT_BODY()

	FProperty* InputProperty;
	FProperty* OutputProperty;

	UPROPERTY()
	FName InputPinName;

	UPROPERTY()
	FName OutputPinName;

	UPROPERTY()
	FGuid InputNodeGuid;

	UPROPERTY()
	FGuid OutputNodeGuid;

	FORCEINLINE bool IsValid() const
	{
		return !OutputPinName.IsNone() && !InputPinName.IsNone() && InputNodeGuid.IsValid() && OutputNodeGuid.IsValid();
	}

	FFlowInputOutputPin()
	{
		InputPinName = OutputPinName = NAME_None;
		InputNodeGuid = OutputNodeGuid = FGuid();
		InputProperty = OutputProperty = nullptr;
	}

	FFlowInputOutputPin(const FName& InPinName, const FName& OutPinName, const FGuid& InNodeGuid, const FGuid& OutNodeGuid)
	{
		InputPinName = InPinName;
		OutputPinName = OutPinName;
		InputNodeGuid = InNodeGuid;
		OutputNodeGuid = OutNodeGuid;
		InputProperty = OutputProperty = nullptr;
	}

	FFlowInputOutputPin(FProperty* NewInputProperty, FProperty* NewOutputProperty)
	{
		InputPinName = OutputPinName = NAME_None;
		InputNodeGuid = OutputNodeGuid = FGuid();
		InputProperty = NewInputProperty;
		OutputProperty = NewOutputProperty;
	}
};

USTRUCT()
struct FLOW_API FFlowPropertyPin
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName Name;

	UPROPERTY()
	FString Tooltip;

	UPROPERTY()
	FName Category;

	UPROPERTY()
	FName SubCategory;

	UPROPERTY()
	UObject* SubCategoryObject = nullptr;

	UPROPERTY()
	bool bIsWeakPointer;
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

	UPROPERTY()
	FFlowInputOutputPin PinProperty;

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

	FConnectedPin(const FGuid InNodeId, const FName& InPinName, const FFlowInputOutputPin& InputOutputPin)
		: NodeGuid(InNodeId)
		  , PinName(InPinName)
		  , PinProperty(InputOutputPin)
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
