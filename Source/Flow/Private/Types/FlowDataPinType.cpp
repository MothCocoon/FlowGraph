// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowDataPinType.h"
#include "Nodes/FlowNodeBase.h"
#include "FlowDataPinSubsystem.h"
#include "FlowLogChannels.h"
#if WITH_EDITOR
#include "PropertyHandle.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDataPinType)

const FFlowDataPinType* FFlowDataPinType::LookupDataPinType(const FFlowPinTypeName& DataPinTypeName)
{
	const FFlowDataPinType* DataPinType = UFlowDataPinSubsystem::Get()->FindDataPinType(DataPinTypeName);

	if (!DataPinType)
	{
		UE_LOG(LogFlow, Error, TEXT("Could not find data pin type %s in FlowDataPinSubsystem"), *DataPinTypeName.ToString());
		return nullptr;
	}

	return DataPinType;
}

bool FFlowDataPinType::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	Node.LogWarning(FString::Printf(TEXT("Unsupported pin type %s for pin %s"), *GetPinTypeName().ToString(), *PinName.ToString()));

	return false;
}

bool FFlowDataPinType::PopulateResult(const UFlowNodeBase& Node, const FName& PinName, FFlowDataPinResult& OutResult) const
{
	Node.LogWarning(FString::Printf(TEXT("Unsupported pin type %s for pin %s"), *GetPinTypeName().ToString(), *PinName.ToString()));

	OutResult.Result = EFlowDataPinResolveResult::FailedMissingPin;

	return false;
}

#if WITH_EDITOR
TSharedPtr<IPropertyHandle> FFlowDataPinType::GetValuesHandle(const TSharedRef<IPropertyHandle>& FlowDataPinValuePropertyHandle) const
{
	return FlowDataPinValuePropertyHandle.Get().GetChildHandle(TEXT("Values"));
}
#endif