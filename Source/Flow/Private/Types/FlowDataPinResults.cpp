// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowDataPinResults.h"
#include "Types/FlowDataPinValuesStandard.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDataPinResults)

FFlowDataPinResult_Object::FFlowDataPinResult_Object(UObject* InValue)
	: Super(EFlowDataPinResolveResult::Success)
{
	SetValueFromObjectPtr(InValue);
}

// FFlowDataPinResult_Class

FFlowDataPinResult_Class::FFlowDataPinResult_Class(const FSoftClassPath& InValuePath)
	: Super(EFlowDataPinResolveResult::Success)
{
	SetValueFromSoftPath(InValuePath);
}

FFlowDataPinResult_Class::FFlowDataPinResult_Class(UClass* InValueClass)
	: Super(EFlowDataPinResolveResult::Success)
{
	SetValueFromObjectPtr(InValueClass);
}

void FFlowDataPinResult_Class::SetValueFromSoftPath(const FSoftObjectPath& SoftObjectPath)
{
	const FSoftClassPath SoftClassPath(SoftObjectPath.ToString());
	SetValueSoftClassAndClassPtr(SoftClassPath, SoftClassPath.ResolveClass());
}

void FFlowDataPinResult_Class::SetValueSoftClassAndClassPtr(const FSoftClassPath& SoftPath, UClass* ObjectPtr)
{
	ValuePath = SoftPath;
	ValueClass = ObjectPtr;
}

FSoftClassPath FFlowDataPinResult_Class::GetAsSoftClass() const
{
	if (ValuePath.IsValid())
	{
		return ValuePath;
	}
	else
	{
		return FSoftClassPath(ValueClass);
	}
}
