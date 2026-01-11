// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/NameTypes.h"
#include "UObject/TextProperty.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "Types/FlowDataPinValuesStandard.h"
#include "Types/FlowDataPinResults.h"
#include "FlowLogChannels.h"
#include <limits>
#include <type_traits>

// #FlowDataPinLegacy
#include "Types/FlowDataPinProperties.h"
// --

namespace FlowPinType
{
	// Success check helper
	FORCEINLINE bool IsSuccess(EFlowDataPinResolveResult ResultEnum)
	{
		return EFlowDataPinResolveResult_Classifiers::IsSuccess(ResultEnum);
	}

	FORCEINLINE EFlowDataPinResolveSimpleResult ConvertToSimpleResult(EFlowDataPinResolveResult ResultEnum)
	{
		return EFlowDataPinResolveResult_Classifiers::ConvertToSimpleResult(ResultEnum);
	}

	// -----------------------------------------------------------------------
	// Value Conversion System
	// -----------------------------------------------------------------------

	// Numeric conversion dispatcher
	template <typename TTarget, typename TSource>
	struct TValueConverter;

	// int32
	template <> struct TValueConverter<int32, int64> { static int32 Convert(int64  Val); };
	template <> struct TValueConverter<int32, float> { static int32 Convert(float  Val); };
	template <> struct TValueConverter<int32, double> { static int32 Convert(double Val); };

	// int64
	template <> struct TValueConverter<int64, int32> { static int64 Convert(int32  Val); };
	template <> struct TValueConverter<int64, float> { static int64 Convert(float  Val); };
	template <> struct TValueConverter<int64, double> { static int64 Convert(double Val); };

	// float
	template <> struct TValueConverter<float, int32> { static float Convert(int32  Val); };
	template <> struct TValueConverter<float, int64> { static float Convert(int64  Val); };
	template <> struct TValueConverter<float, double> { static float Convert(double Val); };

	// double
	template <> struct TValueConverter<double, int32> { static double Convert(int32  Val); };
	template <> struct TValueConverter<double, int64> { static double Convert(int64  Val); };
	template <> struct TValueConverter<double, float> { static double Convert(float  Val); };

	// String types
	template <> struct TValueConverter<FName, FString> { static FName   Convert(const FString& Val); };
	template <> struct TValueConverter<FName, FText> { static FName   Convert(const FText& Val); };
	template <> struct TValueConverter<FString, FName> { static FString Convert(const FName& Val); };
	template <> struct TValueConverter<FString, FText> { static FString Convert(const FText& Val); };
	template <> struct TValueConverter<FText, FName> { static FText   Convert(const FName& Val); };
	template <> struct TValueConverter<FText, FString> { static FText   Convert(const FString& Val); };

	// To string for logging
	template <> struct TValueConverter<FString, int32> { static FString Convert(int32   Val); };
	template <> struct TValueConverter<FString, int64> { static FString Convert(int64   Val); };
	template <> struct TValueConverter<FString, float> { static FString Convert(float   Val); };
	template <> struct TValueConverter<FString, double> { static FString Convert(double  Val); };
	template <> struct TValueConverter<FString, bool> { static FString Convert(bool    Val); };

	// GameplayTag
	template <> struct TValueConverter<FGameplayTag, FGameplayTagContainer>
	{
		static FGameplayTag Convert(const FGameplayTagContainer& Container);
	};

	template <> struct TValueConverter<FGameplayTagContainer, FGameplayTag>
	{
		static FGameplayTagContainer Convert(const FGameplayTag& Tag);
	};

	// -----------------------------------------------------------------------
	// Array Conversion Helper
	// -----------------------------------------------------------------------

	// Converts array with logging and clamping
	template <typename TValue, typename TSource, typename TConverter>
	void ConvertArray(const TArray<TSource>& Source, TArray<TValue>& OutValues, TConverter Converter)
	{
		OutValues.Reserve(Source.Num());
		for (const TSource& Val : Source)
		{
#if !UE_BUILD_SHIPPING
			// Lossy conversion warnings
			if constexpr (std::is_integral_v<TValue> && std::is_floating_point_v<TSource>)
			{
				int64 iv = FMath::FloorToInt64(Val);
				if (iv < std::numeric_limits<TValue>::min() || iv > std::numeric_limits<TValue>::max())
				{
					UE_LOG(LogFlow, Warning, TEXT("Converting %s to %s (out of range, clamping)"),
						*TValueConverter<FString, TSource>::Convert(Val), TEXT("int"));
				}
			}
			else if constexpr (std::is_same_v<TValue, float> && std::is_same_v<TSource, double>)
			{
				if (Val < std::numeric_limits<float>::lowest() || Val > std::numeric_limits<float>::max())
				{
					UE_LOG(LogFlow, Warning, TEXT("Converting %s to float (out of range, clamping)"),
						*TValueConverter<FString, TSource>::Convert(Val));
				}
			}
			else if constexpr (std::is_same_v<TValue, FName> && (std::is_same_v<TSource, FString> || std::is_same_v<TSource, FText>))
			{
				FString SourceStr;
				if constexpr (std::is_same_v<TSource, FString>)
				{
					SourceStr = Val;
				}
				else
				{
					SourceStr = TValueConverter<FString, TSource>::Convert(Val);
				}
				if (SourceStr.Len() > NAME_SIZE)
				{
					UE_LOG(LogFlow, Warning, TEXT("Converting '%s' to FName (possible truncation)"), *SourceStr);
				}
			}
#endif
			OutValues.Add(Converter(Val));
		}
	}

	// -----------------------------------------------------------------------
	// Internal helper – applies the single-from-array policy after extraction
	// -----------------------------------------------------------------------
	template<typename T>
	FORCEINLINE EFlowDataPinResolveResult ApplySinglePolicy(
		const TArray<T>& Source,
		TArray<T>& OutValues,
		EFlowSingleFromArray Policy)
	{
		if (Policy == EFlowSingleFromArray::EntireArray)
		{
			OutValues = Source;
			return EFlowDataPinResolveResult::Success;
		}

		const int32 Num = Source.Num();
		const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(Policy, Num);

		if (!Source.IsValidIndex(Index))
		{
			return EFlowDataPinResolveResult::FailedInsufficientValues;
		}

		OutValues.Add(Source[Index]);
		return EFlowDataPinResolveResult::Success;
	}

	template<typename TSource, typename TValue, typename TConverter>
	FORCEINLINE EFlowDataPinResolveResult ConvertWithPolicy(
		const TArray<TSource>& Source,
		TArray<TValue>& OutValues,
		TConverter Converter,
		EFlowSingleFromArray Policy)
	{
		if (Policy == EFlowSingleFromArray::EntireArray)
		{
			ConvertArray<TValue, TSource>(Source, OutValues, Converter);
			return EFlowDataPinResolveResult::Success;
		}

		const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(Policy, Source.Num());
		if (!Source.IsValidIndex(Index))
		{
			return EFlowDataPinResolveResult::FailedInsufficientValues;
		}

		OutValues.Add(Converter(Source[Index]));
		return EFlowDataPinResolveResult::Success;
	}

	// -----------------------------------------------------------------------
	// Numeric Validation & Clamping
	// -----------------------------------------------------------------------

	template <typename TValue, typename TInput>
	TValue ValidateAndClampNumericValue(TInput Val, TValue MinValue, TValue MaxValue)
	{
		if constexpr (std::is_floating_point<TInput>::value)
		{
#if !UE_BUILD_SHIPPING
			if (!FMath::IsFinite(Val))
			{
				UE_LOG(LogFlow, Warning, TEXT("Non-finite value %s encountered during conversion to %s"),
					*TValueConverter<FString, TInput>::Convert(Val), TEXT("numeric"));
				return TValue(0);
			}
#endif
		}

		if constexpr (std::is_floating_point<TValue>::value)
		{
			if constexpr (std::is_same<TValue, float>::value && std::is_same<TInput, double>::value)
			{
#if !UE_BUILD_SHIPPING
				if (Val < MinValue || Val > MaxValue)
				{
					UE_LOG(LogFlow, Warning, TEXT("Double value %s out of range for float, clamping"),
						*TValueConverter<FString, TInput>::Convert(Val));
				}
#endif
			}
			return FMath::Clamp(static_cast<TValue>(Val), MinValue, MaxValue);
		}
		else
		{
			int64 iv = std::is_floating_point<TInput>::value ? FMath::FloorToInt64(Val) : static_cast<int64>(Val);
#if !UE_BUILD_SHIPPING
			if (iv < MinValue || iv > MaxValue)
			{
				UE_LOG(LogFlow, Warning, TEXT("Value %lld out of range for %s, clamping"), iv, TEXT("int"));
			}
#endif
			return static_cast<TValue>(FMath::Clamp(iv, static_cast<int64>(MinValue), static_cast<int64>(MaxValue)));
		}
	}

	// -----------------------------------------------------------------------
	// ValueConverter Implementations
	// -----------------------------------------------------------------------

	// int32
	inline int32 TValueConverter<int32, int64>::Convert(int64 Val) { return ValidateAndClampNumericValue<int32>(Val, std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max()); }
	inline int32 TValueConverter<int32, float>::Convert(float Val) { return ValidateAndClampNumericValue<int32>(Val, std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max()); }
	inline int32 TValueConverter<int32, double>::Convert(double Val) { return ValidateAndClampNumericValue<int32>(Val, std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max()); }

	// int64
	inline int64 TValueConverter<int64, int32>::Convert(int32 Val) { return static_cast<int64>(Val); }
	inline int64 TValueConverter<int64, float>::Convert(float Val) { return ValidateAndClampNumericValue<int64>(Val, MIN_int64, MAX_int64); }
	inline int64 TValueConverter<int64, double>::Convert(double Val) { return ValidateAndClampNumericValue<int64>(Val, MIN_int64, MAX_int64); }

	// float
	inline float TValueConverter<float, int32>::Convert(int32 Val) { return static_cast<float>(Val); }
	inline float TValueConverter<float, int64>::Convert(int64 Val) { return ValidateAndClampNumericValue<float>(Val, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()); }
	inline float TValueConverter<float, double>::Convert(double Val) { return ValidateAndClampNumericValue<float>(Val, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()); }

	// double
	inline double TValueConverter<double, int32>::Convert(int32 Val) { return static_cast<double>(Val); }
	inline double TValueConverter<double, int64>::Convert(int64 Val) { return static_cast<double>(Val); }
	inline double TValueConverter<double, float>::Convert(float Val) { return static_cast<double>(Val); }

	// String types
	inline FName   TValueConverter<FName, FString>::Convert(const FString& Val) { return FName(*Val); }
	inline FName   TValueConverter<FName, FText>::Convert(const FText& Val) { return FName(*Val.ToString()); }
	inline FString TValueConverter<FString, FName>::Convert(const FName& Val) { return Val.ToString(); }
	inline FString TValueConverter<FString, FText>::Convert(const FText& Val) { return Val.ToString(); }
	inline FText   TValueConverter<FText, FName>::Convert(const FName& Val) { return FText::FromName(Val); }
	inline FText   TValueConverter<FText, FString>::Convert(const FString& Val) { return FText::FromString(Val); }

	// String converters for other types
	inline FString TValueConverter<FString, int32>::Convert(int32 Val) { return FString::Printf(TEXT("%d"), Val); }
	inline FString TValueConverter<FString, int64>::Convert(int64 Val) { return FString::Printf(TEXT("%lld"), Val); }
	inline FString TValueConverter<FString, float>::Convert(float Val) { return FString::Printf(TEXT("%f"), Val); }
	inline FString TValueConverter<FString, double>::Convert(double Val) { return FString::Printf(TEXT("%f"), Val); }
	inline FString TValueConverter<FString, bool>::Convert(bool Val) { return Val ? TEXT("true") : TEXT("false"); }

	// GameplayTag
	inline FGameplayTag TValueConverter<FGameplayTag, FGameplayTagContainer>::Convert(const FGameplayTagContainer& Container)
	{
#if !UE_BUILD_SHIPPING
		if (Container.Num() > 1)
		{
			UE_LOG(LogFlow, Warning, TEXT("Multiple tags in container; using first: %s"), *Container.ToStringSimple());
		}
#endif
		return Container.Num() > 0 ? Container.GetByIndex(0) : FGameplayTag();
	}

	inline FGameplayTagContainer TValueConverter<FGameplayTagContainer, FGameplayTag>::Convert(const FGameplayTag& Tag)
	{
		return FGameplayTagContainer(Tag);
	}

	// -----------------------------------------------------------------------
	// Property Traits
	// -----------------------------------------------------------------------

	// Base for simple scalar types
	template <typename TPinType>
	struct FFlowSimplePropertyTraitsBase
	{
		using ValueType = TPinType::ValueType;
		using WrapperType = TPinType::WrapperType;
		using PropertyType = TPinType::MainPropertyType;
		using LegacyWrapperType = TPinType::LegacyWrapperType;

		static EFlowDataPinResolveResult ExtractFromProperty(const FProperty* Property, const void* Container, TArray<ValueType>& OutValues)
		{
			// 1. Wrapper struct
			if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				if (StructProp->Struct == WrapperType::StaticStruct())
				{
					const WrapperType* Wrapper = StructProp->ContainerPtrToValuePtr<WrapperType>(Container);
					OutValues = Wrapper->Values;
					return EFlowDataPinResolveResult::Success;
				}

				// #FlowDataPinLegacy - support sourcing from old property wrappers For Now(tm)
				static const UScriptStruct* OldPropStruct = LegacyWrapperType::StaticStruct();
				if (StructProp->Struct->IsChildOf(OldPropStruct))
				{
					const LegacyWrapperType* Wrapper = StructProp->ContainerPtrToValuePtr<LegacyWrapperType>(Container);
					OutValues = { Wrapper->Value };
					return EFlowDataPinResolveResult::Success;
				}
				// --
			}

			// 2. Direct property
			if (const PropertyType* Prop = CastField<PropertyType>(Property))
			{
				OutValues = { *Prop->template ContainerPtrToValuePtr<ValueType>(Container) };
				return EFlowDataPinResolveResult::Success;
			}

			// 3. Array of property
			if (const FArrayProperty* ArrProp = CastField<FArrayProperty>(Property))
			{
				if (const PropertyType* Inner = CastField<PropertyType>(ArrProp->Inner))
				{
					FScriptArrayHelper ArrHelper(ArrProp, ArrProp->ContainerPtrToValuePtr<void>(Container));
					OutValues.Reserve(ArrHelper.Num());
					for (int32 i = 0; i < ArrHelper.Num(); ++i)
					{
						OutValues.Add(*Inner->template ContainerPtrToValuePtr<ValueType>(ArrHelper.GetRawPtr(i)));
					}
					return EFlowDataPinResolveResult::Success;
				}
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}

		static EFlowDataPinResolveResult ExtractValues(const FFlowDataPinResult& DataPinResult, TArray<ValueType>& OutValues, EFlowSingleFromArray SingleFromArray)
		{
			if (!IsSuccess(DataPinResult.Result))
			{
				return DataPinResult.Result;
			}

			if (DataPinResult.ResultValue.GetScriptStruct() == WrapperType::StaticStruct())
			{
				const WrapperType& Wrapper = DataPinResult.ResultValue.Get<WrapperType>();
				return ApplySinglePolicy(Wrapper.Values, OutValues, SingleFromArray);
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}
	};

	// Numeric cross-conversion
	template <typename TPinType>
	struct FFlowNumericTraitsBase : public FFlowSimplePropertyTraitsBase<TPinType>
	{
		using Super = FFlowSimplePropertyTraitsBase<TPinType>;
		using ValueType = TPinType::ValueType;
		using WrapperType = TPinType::WrapperType;

		static EFlowDataPinResolveResult ExtractValues(const FFlowDataPinResult& DataPinResult, TArray<ValueType>& OutValues, EFlowSingleFromArray SingleFromArray)
		{
			if (!IsSuccess(DataPinResult.Result))
			{
				return DataPinResult.Result;
			}

			const UScriptStruct* ScriptStruct = DataPinResult.ResultValue.GetScriptStruct();

			if (ScriptStruct == WrapperType::StaticStruct())
			{
				const WrapperType& Wrapper = DataPinResult.ResultValue.Get<WrapperType>();
				return ApplySinglePolicy(Wrapper.Values, OutValues, SingleFromArray);
			}

			// Cross-convert from other numeric types
			if constexpr (!std::is_same_v<WrapperType, FFlowDataPinValue_Int>)
			{
				if (ScriptStruct == FFlowDataPinValue_Int::StaticStruct())
				{
					const FFlowDataPinValue_Int& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_Int>();
					return ConvertWithPolicy(Wrapper.Values, OutValues, TValueConverter<ValueType, int32>::Convert, SingleFromArray);
				}
			}

			if constexpr (!std::is_same_v<WrapperType, FFlowDataPinValue_Int64>)
			{
				if (ScriptStruct == FFlowDataPinValue_Int64::StaticStruct())
				{
					const FFlowDataPinValue_Int64& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_Int64>();
					return ConvertWithPolicy(Wrapper.Values, OutValues, TValueConverter<ValueType, int64>::Convert, SingleFromArray);
				}
			}

			if constexpr (!std::is_same_v<WrapperType, FFlowDataPinValue_Float>)
			{
				if (ScriptStruct == FFlowDataPinValue_Float::StaticStruct())
				{
					const FFlowDataPinValue_Float& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_Float>();
					return ConvertWithPolicy(Wrapper.Values, OutValues, TValueConverter<ValueType, float>::Convert, SingleFromArray);
				}
			}

			if constexpr (!std::is_same_v<WrapperType, FFlowDataPinValue_Double>)
			{
				if (ScriptStruct == FFlowDataPinValue_Double::StaticStruct())
				{
					const FFlowDataPinValue_Double& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_Double>();
					return ConvertWithPolicy(Wrapper.Values, OutValues, TValueConverter<ValueType, double>::Convert, SingleFromArray);
				}
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}
	};

	// String cross-conversion
	template <typename TPinType>
	struct FFlowStringTraitsBase : public FFlowSimplePropertyTraitsBase<TPinType>
	{
		using Super = FFlowSimplePropertyTraitsBase<TPinType>;
		using ValueType = TPinType::ValueType;
		using WrapperType = TPinType::WrapperType;

		static EFlowDataPinResolveResult ExtractValues(const FFlowDataPinResult& DataPinResult, TArray<ValueType>& OutValues, EFlowSingleFromArray SingleFromArray)
		{
			if (!IsSuccess(DataPinResult.Result))
			{
				return DataPinResult.Result;
			}

			const UScriptStruct* ScriptStruct = DataPinResult.ResultValue.GetScriptStruct();

			if (ScriptStruct == WrapperType::StaticStruct())
			{
				const WrapperType& Wrapper = DataPinResult.ResultValue.Get<WrapperType>();
				return ApplySinglePolicy(Wrapper.Values, OutValues, SingleFromArray);
			}

			// Cross-convert from other string types
			if constexpr (!std::is_same_v<WrapperType, FFlowDataPinValue_Name>)
			{
				if (ScriptStruct == FFlowDataPinValue_Name::StaticStruct())
				{
					const FFlowDataPinValue_Name& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_Name>();
					return ConvertWithPolicy(Wrapper.Values, OutValues, TValueConverter<ValueType, FName>::Convert, SingleFromArray);
				}
			}

			if constexpr (!std::is_same_v<WrapperType, FFlowDataPinValue_String>)
			{
				if (ScriptStruct == FFlowDataPinValue_String::StaticStruct())
				{
					const FFlowDataPinValue_String& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_String>();
					return ConvertWithPolicy(Wrapper.Values, OutValues, TValueConverter<ValueType, FString>::Convert, SingleFromArray);
				}
			}

			if constexpr (!std::is_same_v<WrapperType, FFlowDataPinValue_Text>)
			{
				if (ScriptStruct == FFlowDataPinValue_Text::StaticStruct())
				{
					const FFlowDataPinValue_Text& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_Text>();
					return ConvertWithPolicy(Wrapper.Values, OutValues, TValueConverter<ValueType, FText>::Convert, SingleFromArray);
				}
			}

			// Fallback to string conversion from any pin value
			if (ScriptStruct->IsChildOf(FFlowDataPinValue::StaticStruct()))
			{
				const FFlowDataPinValue& BaseWrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue>();
				FString StrValue;
				if (BaseWrapper.TryConvertValuesToString(StrValue))
				{
					if constexpr (std::is_same_v<WrapperType, FFlowDataPinValue_String>)
					{
						OutValues = { StrValue };
					}
					else
					{
						OutValues = { TValueConverter<ValueType, FString>::Convert(StrValue) };
					}
					return EFlowDataPinResolveResult::Success;
				}
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}
	};

	// Struct types (Vector, Rotator, etc.)
	template <typename TPinType>
	struct FFlowStructTraitsBase : public FFlowSimplePropertyTraitsBase<TPinType>
	{
		using ValueType = TPinType::ValueType;
		using WrapperType = TPinType::WrapperType;
		using LegacyWrapperType = TPinType::LegacyWrapperType;

		static EFlowDataPinResolveResult ExtractFromProperty(const FProperty* Property, const void* Container, TArray<ValueType>& OutValues)
		{
			static const UScriptStruct* ValueStruct = TBaseStructure<ValueType>::Get();

			if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				static const UScriptStruct* WrapperStruct = TBaseStructure<WrapperType>::Get();
				if (StructProp->Struct == WrapperStruct)
				{
					const WrapperType* Wrapper = StructProp->ContainerPtrToValuePtr<WrapperType>(Container);
					OutValues = Wrapper->Values;
					return EFlowDataPinResolveResult::Success;
				}

				if (StructProp->Struct == ValueStruct)
				{
					OutValues = { *StructProp->ContainerPtrToValuePtr<ValueType>(Container) };
					return EFlowDataPinResolveResult::Success;
				}

				// #FlowDataPinLegacy - support sourcing from old property wrappers For Now(tm)
				static const UScriptStruct* OldPropStruct = LegacyWrapperType::StaticStruct();
				if (StructProp->Struct->IsChildOf(OldPropStruct))
				{
					const LegacyWrapperType* Wrapper = StructProp->ContainerPtrToValuePtr<LegacyWrapperType>(Container);
					OutValues = { Wrapper->Value };
					return EFlowDataPinResolveResult::Success;
				}
				// --
			}
			else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
			{
				const FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProp->Inner);
				if (InnerStruct && InnerStruct->Struct == ValueStruct)
				{
					FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Container));
					OutValues.Reserve(Helper.Num());
					for (int32 i = 0; i < Helper.Num(); ++i)
					{
						OutValues.Add(*reinterpret_cast<const ValueType*>(Helper.GetRawPtr(i)));
					}
					return EFlowDataPinResolveResult::Success;
				}
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}
	};

	// -----------------------------------------------------------------------
	// Pin Type Traits
	// -----------------------------------------------------------------------

	template <typename TPinType> struct FFlowDataPinValueTraits;

	// Scalars
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Bool> : public FFlowSimplePropertyTraitsBase<FFlowPinType_Bool> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Int> : public FFlowNumericTraitsBase<FFlowPinType_Int> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Int64> : public FFlowNumericTraitsBase<FFlowPinType_Int64> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Float> : public FFlowNumericTraitsBase<FFlowPinType_Float> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Double> : public FFlowNumericTraitsBase<FFlowPinType_Double> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Name> : public FFlowStringTraitsBase<FFlowPinType_Name> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_String> : public FFlowStringTraitsBase<FFlowPinType_String> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Text> : public FFlowStringTraitsBase<FFlowPinType_Text> {};

	// Structs
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Vector> : public FFlowStructTraitsBase<FFlowPinType_Vector> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Rotator> : public FFlowStructTraitsBase<FFlowPinType_Rotator> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Transform> : public FFlowStructTraitsBase<FFlowPinType_Transform> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_InstancedStruct> : public FFlowStructTraitsBase<FFlowPinType_InstancedStruct> {};

	// Enum
	template <>
	struct FFlowDataPinValueTraits<FFlowPinType_Enum> : public FFlowSimplePropertyTraitsBase<FFlowPinType_Enum>
	{
		using TPinType = FFlowPinType_Enum;
		using WrapperType = TPinType::WrapperType;
		using ValueType = TPinType::ValueType;
		using LegacyWrapperType = TPinType::LegacyWrapperType;

		static EFlowDataPinResolveResult ExtractFromProperty(const FProperty* Property, const void* Container, TArray<FName>& OutValues, TSoftObjectPtr<UEnum>& OutEnumClass)
		{
			const FStructProperty* StructProp = CastField<FStructProperty>(Property);
			if (StructProp && StructProp->Struct == WrapperType::StaticStruct())
			{
				const WrapperType* Wrapper = StructProp->ContainerPtrToValuePtr<WrapperType>(Container);
				OutValues = Wrapper->Values;
				OutEnumClass = Wrapper->EnumClass;
				return EFlowDataPinResolveResult::Success;
			}

			// #FlowDataPinLegacy - support sourcing from old property wrappers For Now(tm)
			static const UScriptStruct* OldPropStruct = LegacyWrapperType::StaticStruct();
			if (StructProp && StructProp->Struct->IsChildOf(OldPropStruct))
			{
				const LegacyWrapperType* Wrapper = StructProp->ContainerPtrToValuePtr<LegacyWrapperType>(Container);
				OutValues = { Wrapper->Value };
				OutEnumClass = Wrapper->EnumClass;
				return EFlowDataPinResolveResult::Success;
			}
			// --

			if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
			{
				const void* ContainerPtr = EnumProp->ContainerPtrToValuePtr<uint8>(Container);
				UEnum* EnumClass = EnumProp->GetEnum();
				const FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty();
				int64 RawValue = Underlying->GetSignedIntPropertyValue_InContainer(ContainerPtr);
				FString AuthoredName = EnumClass->GetAuthoredNameStringByValue(RawValue);

				OutValues = { FName(AuthoredName) };
				OutEnumClass = EnumClass;
				return EFlowDataPinResolveResult::Success;
			}

			if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
			{
				if (const FEnumProperty* Inner = CastField<FEnumProperty>(ArrayProp->Inner))
				{
					FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Container));
					UEnum* EnumClass = Inner->GetEnum();
					const FNumericProperty* Underlying = Inner->GetUnderlyingProperty();
					OutValues.Reserve(Helper.Num());
					for (int32 i = 0; i < Helper.Num(); ++i)
					{
						int64 RawValue = Underlying->GetSignedIntPropertyValue(Helper.GetRawPtr(i));
						FString Name = EnumClass->GetAuthoredNameStringByValue(RawValue);
						OutValues.Add(FName(Name));
					}
					OutEnumClass = EnumClass;
					return EFlowDataPinResolveResult::Success;
				}
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}
	};

	// GameplayTag
	template <>
	struct FFlowDataPinValueTraits<FFlowPinType_GameplayTag> : public FFlowStructTraitsBase<FFlowPinType_GameplayTag>
	{
		using PinType = FFlowPinType_GameplayTag;
		using ValueType = PinType::ValueType;
		using WrapperType = FFlowDataPinValue_GameplayTag;
		using ContainerWrapper = FFlowDataPinValue_GameplayTagContainer;

		static EFlowDataPinResolveResult ExtractValues(const FFlowDataPinResult& DataPinResult, TArray<ValueType>& OutValues, EFlowSingleFromArray SingleFromArray)
		{
			if (!IsSuccess(DataPinResult.Result))
			{
				return DataPinResult.Result;
			}

			const UScriptStruct* ScriptStruct = DataPinResult.ResultValue.GetScriptStruct();

			if (ScriptStruct == WrapperType::StaticStruct())
			{
				const WrapperType& Wrapper = DataPinResult.ResultValue.Get<WrapperType>();
				return ApplySinglePolicy(Wrapper.Values, OutValues, SingleFromArray);
			}

			if (ScriptStruct == ContainerWrapper::StaticStruct())
			{
				const ContainerWrapper& Wrapper = DataPinResult.ResultValue.Get<ContainerWrapper>();
				TArray<FGameplayTag> Temp = Wrapper.Values.GetGameplayTagArray();
				return ApplySinglePolicy(Temp, OutValues, SingleFromArray);
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}
	};

	// GameplayTagContainer
	template <>
	struct FFlowDataPinValueTraits<FFlowPinType_GameplayTagContainer> : public FFlowStructTraitsBase<FFlowPinType_GameplayTagContainer>
	{
		using PinType = FFlowPinType_GameplayTagContainer;
		using ValueType = PinType::ValueType;
		using WrapperType = FFlowDataPinValue_GameplayTagContainer;

		static EFlowDataPinResolveResult ExtractFromProperty(const FProperty* Property, const void* Container, TArray<ValueType>& OutValues)
		{
			static const UScriptStruct* ValueStruct = TBaseStructure<ValueType>::Get();

			if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				static const UScriptStruct* WrapperStruct = TBaseStructure<WrapperType>::Get();
				if (StructProp->Struct == WrapperStruct)
				{
					const WrapperType* Wrapper = StructProp->ContainerPtrToValuePtr<WrapperType>(Container);
					OutValues = { Wrapper->Values };
					return EFlowDataPinResolveResult::Success;
				}

				if (StructProp->Struct == ValueStruct)
				{
					OutValues = { *StructProp->ContainerPtrToValuePtr<ValueType>(Container) };
					return EFlowDataPinResolveResult::Success;
				}

				// #FlowDataPinLegacy - support sourcing from old property wrappers For Now(tm)
				static const UScriptStruct* OldPropStruct = LegacyWrapperType::StaticStruct();
				if (StructProp->Struct->IsChildOf(OldPropStruct))
				{
					const LegacyWrapperType* Wrapper = StructProp->ContainerPtrToValuePtr<LegacyWrapperType>(Container);
					OutValues = { Wrapper->Value };
					return EFlowDataPinResolveResult::Success;
				}
				// --
			}
			else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
			{
				const FStructProperty* Inner = CastField<FStructProperty>(ArrayProp->Inner);
				if (Inner && Inner->Struct == ValueStruct)
				{
					FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Container));
					ValueType Consolidated;
					for (int32 i = 0; i < Helper.Num(); ++i)
					{
						Consolidated.AppendTags(*reinterpret_cast<const ValueType*>(Helper.GetRawPtr(i)));
					}
					OutValues = { Consolidated };
					return EFlowDataPinResolveResult::Success;
				}
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}

		static EFlowDataPinResolveResult ExtractValues(const FFlowDataPinResult& DataPinResult, TArray<ValueType>& OutValues, EFlowSingleFromArray SingleFromArray)
		{
			if (!IsSuccess(DataPinResult.Result))
			{
				return DataPinResult.Result;
			}

			const UScriptStruct* ScriptStruct = DataPinResult.ResultValue.GetScriptStruct();

			if (ScriptStruct == WrapperType::StaticStruct())
			{
				const WrapperType& Wrapper = DataPinResult.ResultValue.Get<WrapperType>();
				OutValues = { Wrapper.Values };
				return EFlowDataPinResolveResult::Success;
			}

			if (ScriptStruct == FFlowDataPinValue_GameplayTag::StaticStruct())
			{
				const FFlowDataPinValue_GameplayTag& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_GameplayTag>();
				OutValues = { FGameplayTagContainer::CreateFromArray(Wrapper.Values) };
				return EFlowDataPinResolveResult::Success;
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}
	};

	// Base for Object, Class
	template <typename TPinType, typename TProperty, typename TSoftProperty, typename TValueObjectType>
	struct FFlowObjectTraitsBase
	{
		using ValueType = TPinType::ValueType;
		using WrapperType = TPinType::WrapperType;
		using LegacyWrapperType = TPinType::LegacyWrapperType;

		static EFlowDataPinResolveResult ExtractFromProperty(const FProperty* Property, const void* Container, TArray<ValueType>& OutValues)
		{
			if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				if (StructProp->Struct == WrapperType::StaticStruct())
				{
					const WrapperType* Wrapper = StructProp->ContainerPtrToValuePtr<WrapperType>(Container);
					for (const auto& Path : Wrapper->Values)
					{
						if constexpr (std::is_same_v<std::decay_t<decltype(Path)>, FSoftObjectPath> ||
							std::is_same_v<std::decay_t<decltype(Path)>, FSoftClassPath>)
						{
							OutValues.Add(Cast<TValueObjectType>(Path.ResolveObject()));
						}
						else
						{
							OutValues.Add(Cast<TValueObjectType>(Path));
						}
					}
					return EFlowDataPinResolveResult::Success;
				}

				// #FlowDataPinLegacy - support sourcing from old property wrappers For Now(tm)
				static const UScriptStruct* OldPropStruct = LegacyWrapperType::StaticStruct();
				if (StructProp->Struct->IsChildOf(OldPropStruct))
				{
					const LegacyWrapperType* Wrapper = StructProp->ContainerPtrToValuePtr<LegacyWrapperType>(Container);
					OutValues = { Cast<TValueObjectType>(Wrapper->GetObjectValue()) };
					return EFlowDataPinResolveResult::Success;
				}
				// --
			}

			if (const FArrayProperty* ArrProp = CastField<FArrayProperty>(Property))
			{
				if (const TProperty* InnerObjProp = CastField<TProperty>(ArrProp->Inner))
				{
					FScriptArrayHelper ArrHelper(ArrProp, ArrProp->ContainerPtrToValuePtr<void>(Container));
					const int32 Num = ArrHelper.Num();
					OutValues.Reserve(Num);
					for (int32 i = 0; i < Num; ++i)
					{
						OutValues.Add(Cast<TValueObjectType>(InnerObjProp->GetObjectPropertyValue(ArrHelper.GetRawPtr(i))));
					}
					return EFlowDataPinResolveResult::Success;
				}
				else if (const TSoftProperty* InnerSoftProp = CastField<TSoftProperty>(ArrProp->Inner))
				{
					FScriptArrayHelper ArrHelper(ArrProp, ArrProp->ContainerPtrToValuePtr<void>(Container));
					const int32 Num = ArrHelper.Num();
					OutValues.Reserve(Num);
					for (int32 i = 0; i < Num; ++i)
					{
						const FSoftObjectPath Path = InnerSoftProp->GetPropertyValue(ArrHelper.GetRawPtr(i)).ToSoftObjectPath();
						OutValues.Add(Cast<TValueObjectType>(Path.ResolveObject()));
					}
					return EFlowDataPinResolveResult::Success;
				}
				else if (const FWeakObjectProperty* InnerWeakProp = CastField<FWeakObjectProperty>(ArrProp->Inner))
				{
					FScriptArrayHelper ArrHelper(ArrProp, ArrProp->ContainerPtrToValuePtr<void>(Container));
					const int32 Num = ArrHelper.Num();
					OutValues.Reserve(Num);
					for (int32 i = 0; i < Num; ++i)
					{
						OutValues.Add(Cast<TValueObjectType>(InnerWeakProp->GetPropertyValue_InContainer(Container).Get()));
					}
					return EFlowDataPinResolveResult::Success;
				}
			}

			if (const TProperty* ObjProp = CastField<TProperty>(Property))
			{
				OutValues = { Cast<TValueObjectType>(ObjProp->GetObjectPropertyValue_InContainer(Container)) };
				return EFlowDataPinResolveResult::Success;
			}
			else if (const TSoftProperty* SoftObjProp = CastField<TSoftProperty>(Property))
			{
				const FSoftObjectPath Path = SoftObjProp->GetPropertyValue_InContainer(Container).ToSoftObjectPath();
				OutValues = { Cast<TValueObjectType>(Path.ResolveObject()) };
				return EFlowDataPinResolveResult::Success;
			}
			else if (const FWeakObjectProperty* WeakProp = CastField<FWeakObjectProperty>(Property))
			{
				OutValues = { Cast<TValueObjectType>(WeakProp->GetPropertyValue_InContainer(Container).Get()) };
				return EFlowDataPinResolveResult::Success;
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}

		static EFlowDataPinResolveResult ExtractValues(const FFlowDataPinResult& DataPinResult, TArray<ValueType>& OutValues, EFlowSingleFromArray SingleFromArray)
		{
			if (!IsSuccess(DataPinResult.Result))
			{
				return DataPinResult.Result;
			}

			if (DataPinResult.ResultValue.GetScriptStruct() == WrapperType::StaticStruct())
			{
				const WrapperType& Wrapper = DataPinResult.ResultValue.Get<WrapperType>();
				const auto& Source = Wrapper.Values; // this is TArray<FSoftObjectPath> or TArray<FSoftClassPath>

				if (SingleFromArray == EFlowSingleFromArray::EntireArray)
				{
					OutValues.Reserve(Source.Num());
					for (const auto& Path : Source)
					{
						if constexpr (std::is_same_v<std::decay_t<decltype(Path)>, FSoftObjectPath> ||
							std::is_same_v<std::decay_t<decltype(Path)>, FSoftClassPath>)
						{
							OutValues.Add(Cast<TValueObjectType>(Path.ResolveObject()));
						}
						else
						{
							OutValues.Add(Cast<TValueObjectType>(Path));
						}
					}
				}
				else
				{
					const int32 Index = EFlowSingleFromArray_Classifiers::ConvertToIndex(SingleFromArray, Source.Num());
					if (!Source.IsValidIndex(Index))
					{
						return EFlowDataPinResolveResult::FailedInsufficientValues;
					}

					const auto& Path = Source[Index];
					if constexpr (std::is_same_v<std::decay_t<decltype(Path)>, FSoftObjectPath> ||
						std::is_same_v<std::decay_t<decltype(Path)>, FSoftClassPath>)
					{
						OutValues.Add(Cast<TValueObjectType>(Path.ResolveObject()));
					}
					else
					{
						OutValues.Add(Cast<TValueObjectType>(Path));
					}
				}

				return EFlowDataPinResolveResult::Success;
			}

			return EFlowDataPinResolveResult::FailedMismatchedType;
		}
	};

	template <> struct FFlowDataPinValueTraits<FFlowPinType_Object> : public FFlowObjectTraitsBase<FFlowPinType_Object, FObjectProperty, FSoftObjectProperty, UObject> {};
	template <> struct FFlowDataPinValueTraits<FFlowPinType_Class> : public FFlowObjectTraitsBase<FFlowPinType_Class, FClassProperty, FSoftClassProperty, UClass> {};

	// -----------------------------------------------------------------------
	// Value Extractors
	// -----------------------------------------------------------------------

	template <typename TPinType>
	static EFlowDataPinResolveResult TryExtractValue(const FFlowDataPinResult& DataPinResult, typename TPinType::ValueType& OutValue, EFlowSingleFromArray SingleFromArray)
	{
		if (!IsSuccess(DataPinResult.Result))
		{
			return DataPinResult.Result;
		}

		TArray<typename TPinType::ValueType> Values;
		const EFlowDataPinResolveResult Result = FFlowDataPinValueTraits<TPinType>::ExtractValues(DataPinResult, Values, SingleFromArray);

		if (!IsSuccess(Result))
		{
			return Result;
		}

		if (Values.IsEmpty())
		{
			return EFlowDataPinResolveResult::FailedInsufficientValues;
		}

		OutValue = Values[0];
		return EFlowDataPinResolveResult::Success;
	}

	template <typename TPinType>
	static EFlowDataPinResolveResult TryExtractValues(const FFlowDataPinResult& DataPinResult, TArray<typename TPinType::ValueType>& OutValues)
	{
		if (!IsSuccess(DataPinResult.Result))
		{
			return DataPinResult.Result;
		}

		return FFlowDataPinValueTraits<TPinType>::ExtractValues(DataPinResult, OutValues, EFlowSingleFromArray::EntireArray);
	}

	// Special-case single-value extractor for enums (FName + EnumClass)
	template <typename TPinType = FFlowPinType_Enum>
	static EFlowDataPinResolveResult TryExtractValue(const FFlowDataPinResult& DataPinResult, typename TPinType::ValueType& OutValue, typename TPinType::FieldType*& OutField, EFlowSingleFromArray SingleFromArray)
	{
		if (!IsSuccess(DataPinResult.Result))
		{
			return DataPinResult.Result;
		}

		const typename TPinType::WrapperType& Wrapper = DataPinResult.ResultValue.Get<typename TPinType::WrapperType>();
		OutField = Cast<typename TPinType::FieldType>(Wrapper.GetFieldType());
		return TryExtractValue<TPinType>(DataPinResult, OutValue, SingleFromArray);
	}

	// Special-case array-value extractor for enums (TArray<FName> + EnumClass)
	template <typename TPinType = FFlowPinType_Enum>
	static EFlowDataPinResolveResult TryExtractValues(const FFlowDataPinResult& DataPinResult, TArray<typename TPinType::ValueType>& OutValues, typename TPinType::FieldType*& OutField)
	{
		if (!IsSuccess(DataPinResult.Result))
		{
			return DataPinResult.Result;
		}

		const typename TPinType::WrapperType& Wrapper = DataPinResult.ResultValue.Get<typename TPinType::WrapperType>();
		OutField = Cast<typename TPinType::FieldType>(Wrapper.GetFieldType());
		return TryExtractValues<TPinType>(DataPinResult, OutValues);
	}

	// Special-case single-value extractor for enums (Native enum value)
	template <typename TEnumType> requires std::is_enum_v<TEnumType>
	static EFlowDataPinResolveResult TryExtractValue(const FFlowDataPinResult& DataPinResult, TEnumType& OutValue, EFlowSingleFromArray SingleFromArray)
	{
		if (!IsSuccess(DataPinResult.Result))
		{
			return DataPinResult.Result;
		}

		const FFlowDataPinValue_Enum& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_Enum>();
		return Wrapper.TryGetSingleEnumValue(OutValue, SingleFromArray);
	}

	// Special-case array-value extractor for enums (Native enum values)
	template <typename TEnumType> requires std::is_enum_v<TEnumType>
	static EFlowDataPinResolveResult TryExtractValues(const FFlowDataPinResult& DataPinResult, TArray<TEnumType>& OutValues)
	{
		if (!IsSuccess(DataPinResult.Result))
		{
			return DataPinResult.Result;
		}

		const FFlowDataPinValue_Enum& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_Enum>();
		return Wrapper.TryGetAllNativeEnumValues(OutValues);
	}
}