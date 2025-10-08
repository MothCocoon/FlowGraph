// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Interface.h"

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
	virtual bool CanModifyFlowDataPinValueType() const { return true; }

	// Determines if the bIsInputPin checkbox should be visible in the Details panel
	virtual bool ShowFlowDataPinValueInputPinCheckbox() const { return true; }

	// --------------------------------------------------------------------
	// Class / Enum source visibility & edit policies
	//
	// These are value-level UI policy hooks (mirroring ShowFlowDataPinValueInputPinCheckbox / CanModifyFlowDataPinValueType)
	// to allow higher-level owners (e.g., nodes, assets) to centrally control:
	//  * Whether the ClassFilter (or Enum source) row is shown at all
	//  * Whether the "Lock" toggle is shown
	//  * Whether the ClassFilter / Enum source itself is editable
	//
	// NOTE:
	//  - 'Value' may be nullptr if not derivable in a specific context; implementers should null-guard if they inspect it.
	//  - The per-value struct's own bLockClassFilter (editor-only) is applied AFTER these policies
	//    and AFTER metadata-based forcing (e.g., MetaClass), to produce final editability.
	//  - These methods intentionally share naming so the same logic can gate both ClassFilter and Enum source (EnumClass / EnumName).
	// --------------------------------------------------------------------

	// Should the ClassFilter (or analogous Enum source) row be visible?
	virtual bool ShowFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const { return true; }

	// Should the "Lock Class Filter" toggle (bLockClassFilter) be visible?
	// (If false, the checkbox is hidden; its stored value may still disable editing if already true.)
	virtual bool ShowFlowDataPinValueClassFilterLockToggle(const FFlowDataPinValue* Value) const { return true; }

	// Base policy for whether the ClassFilter / Enum source can be edited (before per-value lock flag).
	virtual bool CanEditFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const { return true; }
#endif
};