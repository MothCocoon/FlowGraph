// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Interfaces/FlowDataPinValueOwnerInterface.h"

/*
* Unified source (ClassFilter / Enum source) policy describing visibility & editability.
* Extended to separate source editability from value editability so that
* locking the source (filter) does NOT lock the value pickers.
*/
struct FFlowValueSourcePolicy
{
	// Show the source row at all (ClassFilter / EnumClass / EnumName)
	bool bShowSourceRow = true;

	// Show the header "Lock" control
	bool bShowLockToggle = true;

	// Per-value lock flag (NOT including metadata forcing)
	bool bLocked = false;

	// Metadata (MetaClass) forces the filter (Class/Object/InstancedObject)
	bool bMetaForced = false; 

	// Editability prior to per-value lock (applies to both source & values)
	bool bBaseEditable = true;

	// Final editability of the SOURCE (filter / enum class+name) after applying lock
	bool bFinalEditableSource = true;

	// Final editability of the VALUES (class picks, object refs, enumerators)
	// Does NOT consider the per-value lock (lock only affects source).
	bool bFinalEditableValues = true;
};

/*
* Computes a unified policy.
* Lock affects only the source editability; value editability ignores the lock.
*/
inline FFlowValueSourcePolicy ComputeFlowValueSourcePolicy(
	IFlowDataPinValueOwnerInterface* Owner,
	const FFlowDataPinValue* Value,
	bool bHasMetaClass,
	bool bPerValueLock,
	bool bLockFieldExists)
{
	FFlowValueSourcePolicy P;

	P.bMetaForced = bHasMetaClass;
	P.bShowSourceRow = Owner ? Owner->ShowFlowDataPinValueClassFilter(Value) : true;

	P.bShowLockToggle =
		P.bShowSourceRow &&
		bLockFieldExists &&
		(Owner ? Owner->ShowFlowDataPinValueClassFilterLockToggle(Value) : true) &&
		!P.bMetaForced;

	P.bBaseEditable =
		!P.bMetaForced &&
		(Owner ? Owner->CanEditFlowDataPinValueClassFilter(Value) : true);

	// Lock only impacts source, not values
	P.bLocked = bPerValueLock;

	P.bFinalEditableSource = P.bBaseEditable && !P.bLocked;

	// Ignore lock for values
	P.bFinalEditableValues = P.bBaseEditable; 

	return P;
}