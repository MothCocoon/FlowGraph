// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowDataPinResults.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "Types/FlowPinTypeTemplates.h"
#include "Types/FlowArray.h"
#include "Nodes/FlowNode.h"

// Additional FlowPinType templates that require FlowNode.h include
namespace FlowPinType
{
	template <typename TPinType>
	static bool PopulateResultTemplate(const UObject& PropertyOwnerObject, const UFlowNode& FlowNode, const FFlowPin& Pin, FFlowDataPinResult& OutResult)
	{
		using TValue = typename TPinType::ValueType;
		using TWrapper = typename TPinType::WrapperType;
		using Traits = FlowPinType::FFlowDataPinValueTraits<TPinType>;

		TInstancedStruct<FFlowDataPinValue> ValueStruct;
		const FProperty* FoundProperty = nullptr;

		if (!FlowNode.TryFindPropertyByPinName(PropertyOwnerObject, Pin.PinName, FoundProperty, ValueStruct))
		{
			OutResult.Result = EFlowDataPinResolveResult::FailedUnknownPin;
			return false;
		}

		if (ValueStruct.IsValid() && ValueStruct.Get<FFlowDataPinValue>().GetPinTypeName() == TPinType::GetPinTypeNameStatic())
		{
			OutResult.ResultValue = ValueStruct;
			OutResult.Result = EFlowDataPinResolveResult::Success;
			return true;
		}

		TArray<TValue> Values;
		if (FlowPinType::IsSuccess(Traits::ExtractFromProperty(FoundProperty, &PropertyOwnerObject, Values)))
		{
			OutResult.ResultValue = TInstancedStruct<TWrapper>::Make(Values);
			OutResult.Result = EFlowDataPinResolveResult::Success;
			return true;
		}

		OutResult.Result = EFlowDataPinResolveResult::FailedMismatchedType;
		return false;
	}

	// ResolveAndFormatArray
	template <typename TPinType>
	bool ResolveAndFormatArray(
		const UFlowNodeBase& Node,
		const FName& PinName,
		FFormatArgumentValue& OutValue,
		TFunctionRef<FString(const typename TPinType::ValueType&)> Formatter)
	{
		using TValue = typename TPinType::ValueType;

		TArray<TValue> Values;
		const EFlowDataPinResolveResult ResolveResult = Node.TryResolveDataPinValues<TPinType>(PinName, Values);
		if (FlowPinType::IsSuccess(ResolveResult))
		{
			const FString ValueString = FlowArray::FormatArrayString<TValue>(Values, Formatter);
			OutValue = FFormatArgumentValue(FText::FromString(ValueString));
			return true;
		}

		return false;
	}
};