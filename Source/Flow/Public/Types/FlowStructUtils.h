// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Field.h"

#if WITH_EDITOR
namespace FlowStructUtils
{
	template <typename TPropertyType, typename TUnrealType>
	static UScriptStruct* FindScriptStructForProperty(const FProperty& Property)
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(&Property);
		if (!StructProperty)
		{
			return nullptr;
		}

		UScriptStruct* ScriptStruct = TPropertyType::StaticStruct();

		if (StructProperty->Struct == ScriptStruct)
		{
			static UScriptStruct* UnrealType = TBaseStructure<TUnrealType>::Get();
			return UnrealType;
		}

		return StructProperty->Struct;
	}
}
#endif