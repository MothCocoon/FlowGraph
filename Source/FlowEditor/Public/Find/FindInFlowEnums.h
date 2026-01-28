// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowEnumUtils.h"

#include "FindInFlowEnums.generated.h"

/** Bitflags controlling what parts of a Flow node are included in search */
UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EFlowSearchFlags : uint32
{
	None = 0 UMETA(Hidden),

	Titles = 1 << 0 UMETA(DisplayName = "Titles"),
	Tooltips = 1 << 1 UMETA(DisplayName = "Tooltips"),
	Classes = 1 << 2 UMETA(DisplayName = "Classes"),
	Comments = 1 << 3 UMETA(DisplayName = "Comments"),
	Descriptions = 1 << 4 UMETA(DisplayName = "Descriptions"),
	ConfigText = 1 << 5 UMETA(DisplayName = "Config Text"),
	PropertyNames = 1 << 6 UMETA(DisplayName = "Property Names"),
	PropertyValues = 1 << 7 UMETA(DisplayName = "Property Values"),
	AddOns = 1 << 8 UMETA(DisplayName = "Add-Ons"),
	Subgraphs = 1 << 9 UMETA(DisplayName = "Subgraphs"),

	All = 
		Titles | Tooltips | Classes | Comments | Descriptions | ConfigText |
		PropertyNames | PropertyValues | AddOns | Subgraphs UMETA(Hidden),

	// Default mask — used at startup and for "reset"
	DefaultSearchFlags = All UMETA(Hidden),
	PropertiesFlags = PropertyNames | PropertyValues | Tooltips UMETA(Hidden),
};
ENUM_CLASS_FLAGS(EFlowSearchFlags);

/** Search scope — intentionally minimal */
UENUM()
enum class EFlowSearchScope : uint8
{
	ThisAssetOnly UMETA(DisplayName = "This Asset", ToolTip = "Search only the currently open Flow Asset"),
	AllOfThisType UMETA(DisplayName = "All Flow Assets of This Type",ToolTip = "Search all Flow Assets of this type (or subclasses)"),
	AllFlowAssets UMETA(DisplayName = "All Flow Assets", ToolTip = "Search every Flow Asset in the project"),

	Max UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowSearchScope);