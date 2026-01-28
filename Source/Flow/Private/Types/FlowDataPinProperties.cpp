// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

// #FlowDataPinLegacy
#include "Types/FlowDataPinProperties.h"
#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDataPinProperties)

FFlowDataPinOutputProperty_Object::FFlowDataPinOutputProperty_Object(UObject* InValue, UClass* InClassFilter)
	: Super()
#if WITH_EDITOR
	, ClassFilter(InClassFilter)
#endif
{
	const UClass* ObjectClass = IsValid(InValue) ? InValue->GetClass() : nullptr;
	if (IsValid(ObjectClass))
	{
		const bool bIsInstanced = (ObjectClass->ClassFlags & CLASS_EditInlineNew) != 0;

		if (bIsInstanced)
		{
			InlineValue = InValue;
			ReferenceValue = nullptr;
		}
		else
		{
			InlineValue = nullptr;
			ReferenceValue = InValue;
		}
	}
	else
	{
		InlineValue = nullptr;
		ReferenceValue = nullptr;
	}
}

// --
