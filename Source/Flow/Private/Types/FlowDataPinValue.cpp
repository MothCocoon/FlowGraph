// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowDataPinValue.h"
#include "Types/FlowDataPinResults.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "Types/FlowPinType.h"
#include "Nodes/FlowPin.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDataPinValue)

const FString FFlowDataPinValue::StringArraySeparator = TEXT(",");

const FFlowPinType* FFlowDataPinValue::LookupPinType() const
{
	return FFlowPinType::LookupPinType(GetPinTypeName());
}
