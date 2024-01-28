// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/FlowPin.h"

#include "Misc/DateTime.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowPin)

//////////////////////////////////////////////////////////////////////////
// Pin Record

#if !UE_BUILD_SHIPPING
FString FPinRecord::NoActivations = TEXT("No activations");
FString FPinRecord::PinActivations = TEXT("Pin activations");
FString FPinRecord::ForcedActivation = TEXT(" (forced activation)");
FString FPinRecord::PassThroughActivation = TEXT(" (pass-through activation)");

FPinRecord::FPinRecord()
	: Time(0.0f)
	, HumanReadableTime(FString())
	, ActivationType(EFlowPinActivationType::Default)
{
}

FPinRecord::FPinRecord(const double InTime, const EFlowPinActivationType InActivationType)
	: Time(InTime)
	, ActivationType(InActivationType)
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

//////////////////////////////////////////////////////////////////////////
// Pin Trait

void FFlowPinTrait::AllowTrait()
{
	if (!bTraitAllowed)
	{
		bTraitAllowed = true;
		bEnabled = true;
	}
}

void FFlowPinTrait::DisallowTrait()
{
	if (bTraitAllowed)
	{
		bTraitAllowed = false;
		bEnabled = false;
	}
}

bool FFlowPinTrait::IsAllowed() const
{
	return bTraitAllowed;
}

void FFlowPinTrait::EnableTrait()
{
	if (bTraitAllowed && !bEnabled)
	{
		bEnabled = true;
	}
}

void FFlowPinTrait::DisableTrait()
{
	if (bTraitAllowed && bEnabled)
	{
		bEnabled = false;
	}
}

void FFlowPinTrait::ToggleTrait()
{
	if (bTraitAllowed)
	{
		bTraitAllowed = false;
		bEnabled = false;
	}
	else
	{
		bTraitAllowed = true;
		bEnabled = true;
	}
}

bool FFlowPinTrait::CanEnable() const
{
	return bTraitAllowed && !bEnabled;
}

bool FFlowPinTrait::IsEnabled() const
{
	return bTraitAllowed && bEnabled;
}

void FFlowPinTrait::MarkAsHit()
{
	bHit = true;
}

void FFlowPinTrait::ResetHit()
{
	bHit = false;
}

bool FFlowPinTrait::IsHit() const
{
	return bHit;
}

