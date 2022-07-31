#pragma once

#include "PropertyPathHelpers.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowPin.h"
#include "FlowAsset.h"
#include "FlowSettings.h"
#include "PropertyPath.h"

#define FLOW_PROPERTY_IS(PropertyRef, Type)							PropertyRef->IsA(F##Type##Property::StaticClass())

namespace FlowPropertyHelpers
{
	static FName GetPinNameFromProperty(const FProperty* Property)
	{
		if (FLOW_PROPERTY_IS(Property, Object))
		{
			return UEdGraphSchema_K2::PC_Object;
		}

		if (FLOW_PROPERTY_IS(Property, Float))
		{
			return UEdGraphSchema_K2::PC_Float;
		}

		if (FLOW_PROPERTY_IS(Property, Bool))
		{
			return UEdGraphSchema_K2::PC_Boolean;
		}

		if (FLOW_PROPERTY_IS(Property, Byte))
		{
			return UEdGraphSchema_K2::PC_Byte;
		}

		if (FLOW_PROPERTY_IS(Property, Int))
		{
			return UEdGraphSchema_K2::PC_Int;
		}

		if (FLOW_PROPERTY_IS(Property, Int64))
		{
			return UEdGraphSchema_K2::PC_Int64;
		}

		if (FLOW_PROPERTY_IS(Property, Struct))
		{
			return UEdGraphSchema_K2::PC_Struct;
		}

		if (FLOW_PROPERTY_IS(Property, Str))
		{
			return UEdGraphSchema_K2::PC_String;
		}

		if (FLOW_PROPERTY_IS(Property, Text))
		{
			return UEdGraphSchema_K2::PC_Text;
		}

		if (FLOW_PROPERTY_IS(Property, Name))
		{
			return UEdGraphSchema_K2::PC_Name;
		}

		return NAME_None;
	}

	/**
	* public
	* static FlowPropertyHelpers::IsPropertyExposedAsInput
	* Checks if the given property is exposed as input.
	* @param Property [const FProperty*] Property to check.
	**/
	static FORCEINLINE_DEBUGGABLE bool IsPropertyExposedAsInput(const FProperty* Property)
	{
		if (Property)
		{
			if (UFlowSettings::Get()->bUseCustomMetaTagsForInputOutputs)
			{
				return Property->HasMetaData(UFlowSettings::Get()->PropertyInputMetaTag);
			}

			return !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance | CPF_BlueprintReadOnly) && Property->HasAllPropertyFlags(CPF_ExposeOnSpawn);
		}

		return false;
	}

	/**
	* public
	* static FlowPropertyHelpers::IsPropertyExposedAsOutput
	* Checks if the given property is exposed as output.
	* @param Property [const FProperty*] Property to check.
	**/
	static FORCEINLINE_DEBUGGABLE bool IsPropertyExposedAsOutput(const FProperty* Property)
	{
		if (Property)
		{
			if (UFlowSettings::Get()->bUseCustomMetaTagsForInputOutputs)
			{
				return Property->HasMetaData(UFlowSettings::Get()->PropertyOutputMetaTag);
			}

			return !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance) && Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly | CPF_BlueprintVisible);
		}

		return false;
	}

	static bool SetPropertyValue(UFlowNode* TargetNode, const FFlowInputOutputPin& ConnectedPin, const FName PinName)
	{
		if (ConnectedPin.InputPinName.IsEqual(PinName))
		{
			UFlowNode* ConnectedParentNode = TargetNode->GetFlowAsset()->GetNode(ConnectedPin.OutputNodeGuid);
			uint8* OutputVariableHolder = ConnectedParentNode->GetVariableContainer();

			const void* OutputValuePtr = ConnectedPin.OutputProperty->ContainerPtrToValuePtr<const void>(OutputVariableHolder);
			void* InputValuePtr = ConnectedPin.InputProperty->ContainerPtrToValuePtr<void>(TargetNode->GetVariableContainer());

			FString Value;
			ConnectedPin.OutputProperty->ExportText_Direct(Value, OutputValuePtr, OutputValuePtr, ConnectedParentNode->GetVariableHolder(), PPF_None);
			ConnectedPin.InputProperty->ImportText(*Value, InputValuePtr, PPF_None, TargetNode->GetVariableHolder());

			return true;
		}

		return false;
	}

	static FORCEINLINE_DEBUGGABLE TMultiMap<FString, FProperty*> GatherProperties(const UScriptStruct* ScriptStruct, bool (&Predicate)(const FProperty*))
	{
		TMultiMap<FString, FProperty*> Properties;

		if (ScriptStruct == nullptr)
		{
			return Properties;
		}

		const FField* ChildProperties = ScriptStruct->ChildProperties;

		while (ChildProperties != nullptr)
		{
			FProperty* Property = ScriptStruct->FindPropertyByName(FName(ChildProperties->GetName()));

			if (Predicate(Property))
			{
				Properties.Add(Property->GetPathName(ScriptStruct), Property);
			}

			ChildProperties = ChildProperties->Next;
		}

		return Properties;
	}

	static FORCEINLINE_DEBUGGABLE TMultiMap<FString, FProperty*> GatherProperties(UObject* Object, bool (&Predicate)(const FProperty*))
	{
		if (Cast<UScriptStruct>(Object))
		{
			return GatherProperties(Cast<UScriptStruct>(Object), Predicate);
		}

		TMultiMap<FString, FProperty*> Properties;

		if (Object == nullptr)
		{
			return Properties;
		}

		for (TFieldIterator<FProperty> PropertyIterator(Object->GetClass()); PropertyIterator; ++PropertyIterator)
		{
			FProperty* Property = *PropertyIterator;
			if (Predicate(Property))
			{
				Properties.Add(Property->GetPathName(Object), Property);
			}
		}

		return Properties;
	}
}
