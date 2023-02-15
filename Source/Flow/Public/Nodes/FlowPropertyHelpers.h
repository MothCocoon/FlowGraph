// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "FlowVariable.h"

#define IS_FLOW_STRUCT_PROPERTY(Property)	const FStructProperty* StructProperty = CastField<FStructProperty>(Property); \
if (StructProperty && StructProperty->Struct == TBaseStructure<FFlowPropertyBag>::Get())

namespace FlowPropertyHelpers
{
	/**
	* public
	* static FlowPropertyHelpers::IsPropertyExposedAsInput
	* Checks if the given property is exposed as input.
	* @param Property [const FProperty*] Property to check.
	**/
	static FORCEINLINE_DEBUGGABLE bool IsPropertyExposedAsInput(const FProperty* Property)
	{
		IS_FLOW_STRUCT_PROPERTY(Property)
		{
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
		IS_FLOW_STRUCT_PROPERTY(Property)
		{
			return !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance) && Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly | CPF_BlueprintVisible);
		}

		return false;
	}

	static FORCEINLINE_DEBUGGABLE FProperty* FindInputPropertyByPinName(const UFlowNode* TargetNode, const FName& InPinName)
	{
		if (!IsValid(TargetNode) || InPinName.IsNone())
		{
			return nullptr;
		}

		for (TFieldIterator<FProperty> PropIt(TargetNode->GetClass()); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;
			if (FlowPropertyHelpers::IsPropertyExposedAsInput(Property))
			{
				return Property;
			}
		}

		return nullptr;
	}

	static FORCEINLINE_DEBUGGABLE FProperty* FindOutputPropertyByPinName(const UFlowNode* TargetNode, const FName& InPinName)
	{
		if (!IsValid(TargetNode) || InPinName.IsNone())
		{
			return nullptr;
		}
		
		for (TFieldIterator<FProperty> PropIt(TargetNode->GetClass()); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;
			if (FlowPropertyHelpers::IsPropertyExposedAsOutput(Property))
			{
				return Property;
			}
		}

		return nullptr;
	}

	static FORCEINLINE_DEBUGGABLE FFlowPropertyBag GetInputFlowPropertyBag(const UFlowNode* TargetNode)
	{
		if (IsValid(TargetNode))
		{
			for (TFieldIterator<FProperty> PropertyIterator(TargetNode->GetClass()); PropertyIterator; ++PropertyIterator)
			{
				const FProperty* Property = *PropertyIterator;
				if (IsPropertyExposedAsInput(Property))
				{
					return FFlowPropertyBag(*Property);
				}
			}
		}

		return FFlowPropertyBag();
	}

	static FORCEINLINE_DEBUGGABLE FFlowPropertyBag GetOutputFlowPropertyBag(const UFlowNode* TargetNode)
	{
		for (TFieldIterator<FProperty> PropertyIterator(TargetNode->GetClass()); PropertyIterator; ++PropertyIterator)
		{
			const FProperty* Property = *PropertyIterator;
			if (IsPropertyExposedAsOutput(Property))
			{
				return FFlowPropertyBag(*Property);
			}
		}

		return FFlowPropertyBag();
	}
	
	static FORCEINLINE_DEBUGGABLE bool SetPropertyValue(UFlowNode* TargetNode, const FConnectedPin& ConnectedPin)
	{
		if (!ConnectedPin.VariablePin.IsPinValid())
		{
			return false;
		}
		
		for (TFieldIterator<FStructProperty> PropertyIterator(TargetNode->GetClass()); PropertyIterator; ++PropertyIterator)
		{
			const FStructProperty* StructProperty = *PropertyIterator;
			if (FlowPropertyHelpers::IsPropertyExposedAsInput(StructProperty))
			{
				const auto& VariablePin = ConnectedPin.VariablePin;
				const auto ValueStruct = VariablePin.PropertyBag.GetValueStruct<FFlowPropertyBag>(ConnectedPin.VariablePin.InputPinName);
				check(ValueStruct.IsValid());
				const auto ValuePtr = ValueStruct.GetValue();

				auto NodeBag = StructProperty->ContainerPtrToValuePtr<FFlowPropertyBag>(TargetNode);
				NodeBag->PropertyName = VariablePin.InputPinName;
				NodeBag->RuntimeBag = ValuePtr->RuntimeBag.IsValid() ? ValuePtr->RuntimeBag : ValuePtr->Properties;
				
				return true;
			}
		}

		return false;
	}

	static FORCEINLINE_DEBUGGABLE void SetVariablePin(const UFlowNode* TargetNode, FConnectedPin& TargetPin)
	{
		if (!TargetNode->IsSignalModeEnabled())
		{
			return;
		}
		
		const FFlowPropertyBag OutputPropertyPin = FlowPropertyHelpers::GetOutputFlowPropertyBag(TargetNode);
		if (OutputPropertyPin.IsValid())
		{
			const FConnectedPin FlowOutputPin = TargetNode->GetConnection(OutputPropertyPin.PropertyName);
			if (FlowOutputPin.IsValidPin())
			{
				FFlowInputOutputPin InputPin;
				const UFlowNode* LinkedNode = TargetNode->GetFlowAsset()->GetNode(FlowOutputPin.VariablePin.InputNodeGuid);
				const UFlowNode* OutputNode = TargetNode->GetFlowAsset()->GetNode(FlowOutputPin.VariablePin.OutputNodeGuid);
				const auto InputProperty = FlowPropertyHelpers::FindInputPropertyByPinName(LinkedNode, FlowOutputPin.VariablePin.InputPinName);
				const auto OutputProperty = FlowPropertyHelpers::FindOutputPropertyByPinName(OutputNode, FlowOutputPin.VariablePin.OutputPinName);
				InputPin = FlowOutputPin.VariablePin;
				InputPin.SetInputValue(OutputNode, InputProperty, OutputProperty);
				TargetPin.VariablePin = InputPin;
			}
		}
	}
}

#undef IS_FLOW_STRUCT_PROPERTY
