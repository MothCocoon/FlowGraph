// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/NameTypes.h"
#include "Types/FlowPinEnums.h"
#include "Types/FlowDataPinValue.h"
#include "Types/FlowDataPinResults.h"

#include <functional>

struct FFlowDataPinResult;

struct FFlowDataPinValueTemplates
{
public:
// TODO (gtaylor) adapt these for the data pins refactor
// 
// 	// Generic template for single value extraction using access lambda
// 	template <typename TFlowDataPinValueSubclass, typename TInner, typename TAccessFunc>
// 	static bool TryGetFirstValue(const FFlowDataPinResult& Result, const FName& ValueTypeName, TInner& OutFirstValue, TAccessFunc AccessFunc)
// 	{
// 		if (Result.Result != EFlowDataPinResolveResult::Success ||
// 			Result.ValueArray.IsEmpty() ||
// 			ValueTypeName != Result.ValueArray[0].Get().GetPinTypeName())
// 		{
// 			return false;
// 		}
// 
// 		const TFlowDataPinValueSubclass* TypedSubclass = Result.ValueArray[0].GetPtr<TFlowDataPinValueSubclass>();
// 		AccessFunc(TypedSubclass, OutFirstValue);
// 		return true;
// 	}
// 
// 	// Generic template for array value extraction using access lambda
// 	template <typename TFlowDataPinValueSubclass, typename TInner, typename TAccessFunc>
// 	static bool TryGetArrayValues(const FFlowDataPinResult& Result, const FName& ValueTypeName, TArray<TInner>& OutArrayValues, TAccessFunc AccessFunc)
// 	{
// 		check(OutArrayValues.IsEmpty());
// 
// 		if (Result.Result != EFlowDataPinResolveResult::Success)
// 		{
// 			return false;
// 		}
// 
// 		if (Result.ValueArray.IsEmpty())
// 		{
// 			return true;
// 		}
// 
// 		if (ValueTypeName != Result.ValueArray[0].Get().GetPinTypeName())
// 		{
// 			return false;
// 		}
// 
// 		OutArrayValues.Reserve(Result.ValueArray.Num());
// 
// 		for (const TInstancedStruct<FFlowDataPinValue>& DataPinValue : Result.ValueArray)
// 		{
// 			checkf(DataPinValue.Get().GetPinTypeName() == ValueTypeName, "These arrays must be homogenous");
// 
// 			const TFlowDataPinValueSubclass* TypedSubclass = DataPinValue.GetPtr<TFlowDataPinValueSubclass>();
// 			TInner Value;
// 			AccessFunc(TypedSubclass, Value);
// 			OutArrayValues.Add(Value);
// 		}
// 
// 		return true;
// 	}
// 
// 	// Template for single enum value extraction
// 	template <typename TAccessFunc>
// 	static bool TryGetFirstEnumValue(const FFlowDataPinResult& Result, const FName& ValueTypeName, FName& OutFirstEnumValueName, UEnum*& OutEnumClass, TAccessFunc AccessFunc)
// 	{
// 		if (Result.Result != EFlowDataPinResolveResult::Success ||
// 			Result.ValueArray.IsEmpty() ||
// 			ValueTypeName != Result.ValueArray[0].Get().GetPinTypeName())
// 		{
// 			return false;
// 		}
// 
// 		const FFlowDataPinValue_Enum* TypedSubclass = Result.ValueArray[0].GetPtr<FFlowDataPinValue_Enum>();
// 		AccessFunc(TypedSubclass, OutFirstEnumValueName, OutEnumClass);
// 		return true;
// 	}
// 
// 	// Template for enum array value extraction
// 	template <typename TAccessFunc>
// 	static bool TryGetEnumArrayValues(const FFlowDataPinResult& Result, const FName& ValueTypeName, TArray<FName>& OutEnumValueNames, UEnum*& OutEnumClass, TAccessFunc AccessFunc)
// 	{
// 		check(OutEnumValueNames.IsEmpty());
// 		check(OutEnumClass = nullptr);
// 
// 		if (Result.Result != EFlowDataPinResolveResult::Success)
// 		{
// 			return false;
// 		}
// 
// 		if (Result.ValueArray.IsEmpty())
// 		{
// 			return true;
// 		}
// 
// 		if (ValueTypeName != Result.ValueArray[0].Get().GetPinTypeName())
// 		{
// 			return false;
// 		}
// 
// 		const FFlowDataPinValue_Enum* FirstTypedSubclass = Result.ValueArray[0].GetPtr<FFlowDataPinValue_Enum>();
// 		OutEnumClass = FirstTypedSubclass->EnumClass;
// 
// 		OutEnumValueNames.Reserve(Result.ValueArray.Num());
// 
// 		for (const TInstancedStruct<FFlowDataPinValue>& DataPinValue : Result.ValueArray)
// 		{
// 			checkf(DataPinValue.Get().GetPinTypeName() == ValueTypeName, "These arrays must be homogenous");
// 
// 			const FFlowDataPinValue_Enum* TypedSubclass = DataPinValue.GetPtr<FFlowDataPinValue_Enum>();
// 			checkf(TypedSubclass->EnumClass == OutEnumClass, "Enum arrays must use the same EnumClass");
// 
// 			FName Value;
// 			AccessFunc(TypedSubclass, Value, OutEnumClass);
// 			OutEnumValueNames.Add(Value);
// 		}
// 
// 		return true;
// 	}
};