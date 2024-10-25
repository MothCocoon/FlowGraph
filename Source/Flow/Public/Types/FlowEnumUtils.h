// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Misc/EnumRange.h"

#include <type_traits>

// Extensions to EnumRange.h

namespace FlowEnum
{
	template<class T> constexpr auto MinOf() { return 0; }
	template<class T> constexpr auto MaxOf() { return 0; }

	// NOTE (gtaylor) In this context, a "Valid" enum value is one that is within ::Min to ::Max - 1
	//  Invalid values (like ::Invalid) should fall outside of this range
	template<class TEnum> constexpr bool IsValidEnumValue(const TEnum EnumValue) { return false; }

	// NOTE (gtaylor) In this context, a subrange is First..Last (where last is an inclusive bound)
	template<class TEnum> constexpr bool IsEnumValueInSubrange(const TEnum EnumValue, const TEnum SubrangeFirst, const TEnum SubrangeLast) { return false; }

	// Utility templates for Enums
	template <typename T, typename = typename std::is_enum<T>::type>
	struct safe_underlying_type {
		using type = void;
	};

	template <typename T>
	struct safe_underlying_type<T, std::true_type> {
		using type = std::underlying_type_t<T>;
	};

	template <typename T>
	struct safe_underlying_type<T, std::false_type> {
		using type = T;
	};

	template<typename EnumType>
	constexpr std::underlying_type_t<EnumType> ToUnderlyingType(const EnumType Value)
	{
		return static_cast<std::underlying_type_t<EnumType>>(Value);
	}
}
#define FLOW_ENUM_STATIC_CAST_TO_INT(EnumType) \
	namespace FlowEnum { constexpr auto ToInt(EnumType EnumValue) { return static_cast<safe_underlying_type<EnumType>::type>(EnumValue); } }

#define FLOW_ENUM_STATIC_CAST_MIN_AND_MAX(EnumType, MinValue, MaxValue) \
	namespace FlowEnum { \
		template<> constexpr auto MinOf<EnumType>() { return static_cast<safe_underlying_type<EnumType>::type>(MinValue); } \
		template<> constexpr auto MaxOf<EnumType>() { return static_cast<safe_underlying_type<EnumType>::type>(MaxValue); } \
	}
#define FLOW_ENUM_RANGE_UTILITY_FUNCTIONS(EnumType) \
	namespace FlowEnum { \
		template<> constexpr bool IsEnumValueInSubrange(const EnumType EnumValue, const EnumType SubrangeFirst, const EnumType SubrangeLast) { return ToInt(EnumValue) >= ToInt(SubrangeFirst) && ToInt(EnumValue) <= ToInt(SubrangeLast); } \
		template<> constexpr bool IsValidEnumValue(const EnumType EnumValue) { return ToInt(EnumValue) >= MinOf<EnumType>() && ToInt(EnumValue) < MaxOf<EnumType>(); } \
	}
#define FLOW_IS_ENUM_IN_SUBRANGE(EnumValue, SubrangeTag) FlowEnum::IsEnumValueInSubrange(EnumValue, SubrangeTag##First, SubrangeTag##Last)

// Macros to static-assert the max or a particular value of an enum is the expected integral value
#define FLOW_ASSERT_ENUM_MAX(EnumType, IntMaxValue) static_assert(FlowEnum::MaxOf<EnumType>() == IntMaxValue, "Ensure this code is correct after making changes to this enum.")
#define FLOW_ASSERT_ENUM_VALUE(EnumValue, IntValue) static_assert(FlowEnum::ToInt(EnumValue) == IntValue, "Ensure this code is correct after making changes to this enum.")

/**
* Version of ENUM_RANGE_VALUES for 'C-style' enums
*
* Defines a contiguous enum range from MyEnum_Min to (MyEnum_Max - 1)
*
* Example:
*
* enum EMyEnum
* {
*     MyEnum_First,
*     MyEnum_Second,
*     MyEnum_Third,
*
*     MyEnum_Max,
*     MyEnum_Invalid = -1,
*	  MyEnum_Min = 0,
* };
*
* // Defines iteration over EMyEnum to be: First, Second, Third
* ENUM_RANGE_VALUES_WITH_MIN_AND_MAX(EMyEnum, MyEnum_Min, MyEnum_Max)
*/
#define FLOW_ENUM_RANGE_VALUES_WITH_MIN_AND_MAX(EnumType, EnumMin, EnumMax) \
	ENUM_RANGE_BY_FIRST_AND_LAST(EnumType, static_cast<int64>(EnumMin), static_cast<int64>(EnumMax) - 1) \
	FLOW_ENUM_STATIC_CAST_MIN_AND_MAX(EnumType, EnumMin, EnumMax) \
	FLOW_ENUM_STATIC_CAST_TO_INT(EnumType) \
	FLOW_ENUM_RANGE_UTILITY_FUNCTIONS(EnumType)

/**
* Defines a contiguous enum range from Min to (Max - 1)
*
* Examples:
*
* for unsigned int:
*
* enum class EMyEnum : uint32
* {
*     First,
*     Second,
*     Third,
*
*     Max,
*     Invalid,
*     Min = 0,
* };
*
* or with signed int:
*
* enum class EMyEnum : int32
* {
*     First,
*     Second,
*     Third,
*
*     Max,
*     Invalid = -1,
*     Min = 0,
* };
*
* // Defines iteration over EMyEnum to be: First, Second, Third
* ENUM_RANGE_VALUES(EMyEnum)
*/
#define FLOW_ENUM_RANGE_VALUES(EnumType) \
	ENUM_RANGE_BY_FIRST_AND_LAST(EnumType, static_cast<int64>(EnumType::Min), static_cast<int64>(EnumType::Max) - 1) \
	FLOW_ENUM_STATIC_CAST_MIN_AND_MAX(EnumType, EnumType::Min, EnumType::Max) \
	FLOW_ENUM_STATIC_CAST_TO_INT(EnumType) \
	FLOW_ENUM_RANGE_UTILITY_FUNCTIONS(EnumType)
