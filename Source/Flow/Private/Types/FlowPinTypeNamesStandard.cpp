// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowPinTypeNamesStandard.h"

#if WITH_EDITOR

// Cross-conversion rules:
// - Most* types → String (one-way)  (*except InstancedStruct)
// - Numeric: full bidirectional conversion
// - Name/String/Text: full bidirectional
// - GameplayTag ↔ Container: bidirectional

const TMap<FName, FFlowPinTypeMatchPolicy> FFlowPinTypeNamesStandard::PinTypeMatchPolicies =
{
	{ FFlowPinTypeNamesStandard::PinTypeNameBool,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{ },
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameInt,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{
				FFlowPinTypeNamesStandard::PinTypeNameInt64,
				FFlowPinTypeNamesStandard::PinTypeNameFloat,
				FFlowPinTypeNamesStandard::PinTypeNameDouble
			},
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameInt64,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{
				FFlowPinTypeNamesStandard::PinTypeNameInt,
				FFlowPinTypeNamesStandard::PinTypeNameFloat,
				FFlowPinTypeNamesStandard::PinTypeNameDouble
			},
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameFloat,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{
				FFlowPinTypeNamesStandard::PinTypeNameInt,
				FFlowPinTypeNamesStandard::PinTypeNameInt64,
				FFlowPinTypeNamesStandard::PinTypeNameDouble
			},
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameDouble,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{
				FFlowPinTypeNamesStandard::PinTypeNameInt,
				FFlowPinTypeNamesStandard::PinTypeNameInt64,
				FFlowPinTypeNamesStandard::PinTypeNameFloat
			},
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameEnum,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{ },
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameName,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{
				FFlowPinTypeNamesStandard::PinTypeNameString,
				FFlowPinTypeNamesStandard::PinTypeNameText
			},
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameString,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{
				FFlowPinTypeNamesStandard::PinTypeNameName,
				FFlowPinTypeNamesStandard::PinTypeNameText,

				// All other types (except InstancedStruct) can cross-convert to string
				FFlowPinTypeNamesStandard::PinTypeNameBool,
				FFlowPinTypeNamesStandard::PinTypeNameInt,
				FFlowPinTypeNamesStandard::PinTypeNameInt64,
				FFlowPinTypeNamesStandard::PinTypeNameFloat,
				FFlowPinTypeNamesStandard::PinTypeNameDouble,
				FFlowPinTypeNamesStandard::PinTypeNameEnum,
				FFlowPinTypeNamesStandard::PinTypeNameVector,
				FFlowPinTypeNamesStandard::PinTypeNameRotator,
				FFlowPinTypeNamesStandard::PinTypeNameTransform,
				FFlowPinTypeNamesStandard::PinTypeNameGameplayTag,
				FFlowPinTypeNamesStandard::PinTypeNameGameplayTagContainer,
				FFlowPinTypeNamesStandard::PinTypeNameObject,
				FFlowPinTypeNamesStandard::PinTypeNameClass
			},
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameText,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{
				FFlowPinTypeNamesStandard::PinTypeNameString,
				FFlowPinTypeNamesStandard::PinTypeNameName
			},
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameVector,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{ },
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameRotator,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{ },
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameTransform,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{ },
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameGameplayTag,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{
				FFlowPinTypeNamesStandard::PinTypeNameGameplayTagContainer
			},
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameGameplayTagContainer,
		{
			EFlowPinTypeMatchRules::StandardPinTypeMatchRulesMask,
			{
				FFlowPinTypeNamesStandard::PinTypeNameGameplayTag
			},
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameInstancedStruct,
		{
			EFlowPinTypeMatchRules::SubCategoryObjectPinTypeMatchRulesMask,
			{ },
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameObject,
		{
			EFlowPinTypeMatchRules::SubCategoryObjectPinTypeMatchRulesMask,
			{ },
		}
	},
	{ FFlowPinTypeNamesStandard::PinTypeNameClass,
		{
			EFlowPinTypeMatchRules::SubCategoryObjectPinTypeMatchRulesMask,
			{ },
		}
	},
};
#endif