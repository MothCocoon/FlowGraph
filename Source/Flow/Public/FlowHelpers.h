#pragma once

#define FLOW_PROPERTY_IS(PropertyRef, Type)							PropertyRef->IsA(F##Type##Property::StaticClass())

namespace FlowHelpers
{
	/**
	* public
	* static FlowHelpers::IsPropertySupported
	* Checks if the given property is supported as input/output.
	* @param Property [const FProperty*] Property to check.
	**/
	static FORCEINLINE_DEBUGGABLE bool IsPropertySupported(const FProperty* Property)
	{
		return FLOW_PROPERTY_IS(Property, Object)
			|| FLOW_PROPERTY_IS(Property, Float)
			|| FLOW_PROPERTY_IS(Property, Bool)
			|| FLOW_PROPERTY_IS(Property, Byte)
			|| FLOW_PROPERTY_IS(Property, Int)
			|| FLOW_PROPERTY_IS(Property, Int64)
			|| FLOW_PROPERTY_IS(Property, Struct)
			|| FLOW_PROPERTY_IS(Property, Str)
			|| FLOW_PROPERTY_IS(Property, Text)
			|| FLOW_PROPERTY_IS(Property, Name);
	}

	/**
	* public
	* static FlowHelpers::IsPropertyExposedAsInput
	* Checks if the given property is exposed as input.
	* @param Property [const FProperty*] Property to check.
	**/
	static FORCEINLINE_DEBUGGABLE bool IsPropertyExposedAsInput(const FProperty* Property)
	{
		if (Property && IsPropertySupported(Property))
		{
			return !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance | CPF_BlueprintReadOnly) && Property->HasAllPropertyFlags(CPF_ExposeOnSpawn);
		}

		return false;
	}

	/**
	* public
	* static FlowHelpers::IsPropertyExposedAsOutput
	* Checks if the given property is exposed as output.
	* @param Property [const FProperty*] Property to check.
	**/
	static FORCEINLINE_DEBUGGABLE bool IsPropertyExposedAsOutput(const FProperty* Property)
	{
		if (Property && IsPropertySupported(Property))
		{
			return !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance) && Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly | CPF_BlueprintVisible);
		}

		return false;
	}
}
