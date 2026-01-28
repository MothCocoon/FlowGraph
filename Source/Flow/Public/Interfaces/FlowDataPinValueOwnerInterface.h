// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Interface.h"
#include "Delegates/Delegate.h"

#include "FlowDataPinValueOwnerInterface.generated.h"

struct FFlowDataPinValue;

UINTERFACE(NotBlueprintable)
class FLOW_API UFlowDataPinValueOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowDataPinValueOwnerInterface
{
	GENERATED_BODY()

public:
#if WITH_EDITOR

	// Determines if the pin's type properties (bIsInputPin, MultiType) can be modified
	virtual bool CanModifyFlowDataPinType() const { return true; }

	// Determines if the bIsInputPin checkbox should be visible in the Details panel
	virtual bool ShowFlowDataPinValueInputPinCheckbox() const { return true; }

	// Should the ClassFilter or EnumClass row be visible?
	virtual bool ShowFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const { return true; }

	// Base policy for whether the ClassFilter / Enum source can be edited
	virtual bool CanEditFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const { return true; }

	// Set the delegate that forces a layout rebuild (provided by owner detail customization).
	virtual void SetFlowDataPinValuesRebuildDelegate(FSimpleDelegate InDelegate) {}

	// Request a details rebuild (executes delegate if bound).
	virtual void RequestFlowDataPinValuesDetailsRebuild() {}
#endif
};
