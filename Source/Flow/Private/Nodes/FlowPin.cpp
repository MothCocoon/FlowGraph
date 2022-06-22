// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/FlowPin.h"

#if !UE_BUILD_SHIPPING

FString FPinRecord::NoActivations = TEXT("No activations");
FString FPinRecord::PinActivations = TEXT("Pin activations");
FString FPinRecord::ForcedActivation = TEXT(" (forced activation)");

FPinRecord::FPinRecord()
	: Time(0.0f)
	, HumanReadableTime(FString())
	, bForcedActivation(false)
{
}

FPinRecord::FPinRecord(const double InTime, const bool bInForcedActivation)
	: Time(InTime)
	, bForcedActivation(bInForcedActivation)
{
	const FDateTime SystemTime(FDateTime::Now());
	HumanReadableTime = DoubleDigit(SystemTime.GetHour()) + TEXT(".")
		+ DoubleDigit(SystemTime.GetMinute()) + TEXT(".")
		+ DoubleDigit(SystemTime.GetSecond()) + TEXT(":")
		+ DoubleDigit(SystemTime.GetMillisecond()).Left(3);
}

FORCEINLINE FString FPinRecord::DoubleDigit(const int32 Number)
{
	return Number > 9 ? FString::FromInt(Number) : TEXT("0") + FString::FromInt(Number);
}

#endif
