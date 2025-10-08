// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "DetailCustomizations/FlowDataPinValueCustomization_ObjectBase.h"

/*
* Concrete customization for FFlowDataPinValue_Object.
* Relies entirely on ObjectBase shared behavior (no inline creation).
*/
class FLOWEDITOR_API FFlowDataPinValueCustomization_Object : public FFlowDataPinValueCustomization_ObjectBase
{
	using Super = FFlowDataPinValueCustomization_ObjectBase;

public:
	FFlowDataPinValueCustomization_Object() = default;

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FFlowDataPinValueCustomization_Object());
	}

	// Non-copyable / non-movable
	FFlowDataPinValueCustomization_Object(const FFlowDataPinValueCustomization_Object&) = delete;
	FFlowDataPinValueCustomization_Object& operator=(const FFlowDataPinValueCustomization_Object&) = delete;
	FFlowDataPinValueCustomization_Object(FFlowDataPinValueCustomization_Object&&) = delete;
	FFlowDataPinValueCustomization_Object& operator=(FFlowDataPinValueCustomization_Object&&) = delete;
};