// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowDataPinValue.h"
#include "Types/FlowDataPinResults.h"
#include "Types/FlowDataPinType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDataPinValue)

const FFlowDataPinType* FFlowDataPinValue::LookupDataPinType() const
{
	return FFlowDataPinType::LookupDataPinType(GetPinTypeName());
}

bool FFlowDataPinValue::PopulateResult(const FProperty* Property, const UObject* Container, FFlowDataPinResult& OutResult) const
{
	OutResult = FFlowDataPinResult(EFlowDataPinResolveResult::FailedMismatchedType);

	return false;
}