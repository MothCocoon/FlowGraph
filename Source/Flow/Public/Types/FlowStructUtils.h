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

	template<typename TStruct>
	TStruct* GetTypedStructValue(FProperty& Prop, void* Container)
	{
		static_assert(TIsDerivedFrom<TStruct, TStruct>::IsDerived, "Must be a USTRUCT type");
		if (auto* StructProp = CastField<FStructProperty>(&Prop))
		{
			if (StructProp->Struct->IsChildOf(TStruct::StaticStruct()))
			{
				return reinterpret_cast<TStruct*>(StructProp
					->ContainerPtrToValuePtr<void>(Container));
			}
		}
		return nullptr;
	}

	// Internal SFINAE probe: will fail to compile if TStruct has no StaticStruct().
	template<typename T>
	struct THasStaticStruct
	{
	private:
		template<typename U>
		static auto Test(int) -> decltype(U::StaticStruct(), std::true_type{});
		template<typename>
		static std::false_type Test(...);
	public:
		static constexpr bool Value = decltype(Test<T>(0))::value;
	};

	template<typename TStruct>
	FORCEINLINE TStruct* CastStructValue(FProperty* Prop, void* Container)
	{
		static_assert(THasStaticStruct<TStruct>::Value,
			"TStruct must be a USTRUCT type providing StaticStruct().");

		if (!Prop || !Container)
			return nullptr;

		FStructProperty* StructProp = CastField<FStructProperty>(Prop);
		if (!StructProp)
			return nullptr;

		// Check exact or derived type.
		if (!StructProp->Struct->IsChildOf(TStruct::StaticStruct()))
			return nullptr;

		// Retrieve the memory for this property within the container and cast.
		void* ValueMem = StructProp->ContainerPtrToValuePtr<void>(Container);
		return static_cast<TStruct*>(ValueMem);
	}

	// Pointer overload (const)
	template<typename TStruct>
	FORCEINLINE const TStruct* CastStructValue(const FProperty* Prop, const void* Container)
	{
		return CastStructValue<TStruct>(
			const_cast<FProperty*>(Prop),
			const_cast<void*>(Container));
	}

	// Reference overloads for convenience
	template<typename TStruct>
	FORCEINLINE TStruct* CastStructValue(FProperty& Prop, void* Container)
	{
		return CastStructValue<TStruct>(&Prop, Container);
	}

	template<typename TStruct>
	FORCEINLINE const TStruct* CastStructValue(const FProperty& Prop, const void* Container)
	{
		return CastStructValue<TStruct>(&Prop, Container);
	}
}
#endif