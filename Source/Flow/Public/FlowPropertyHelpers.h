#pragma once

#include "PropertyPathHelpers.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowPin.h"
#include "FlowAsset.h"

#define FLOW_PROPERTY_IS(PropertyRef, Type)							PropertyRef->IsA(F##Type##Property::StaticClass())

namespace FlowPropertyHelpers
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

	static FORCEINLINE_DEBUGGABLE bool SetPropertyValue(UFlowNode* TargetNode, const FConnectedPin& ConnectedPin, const FProperty* Property)
	{
		if (Property->GetFName().IsEqual(ConnectedPin.PinProperty.InputProperty->GetFName()))
		{
			const UFlowNode* InputNode = Cast<UFlowNode>(TargetNode->GetFlowAsset()->GetNodes().FindRef(ConnectedPin.PinProperty.OutputNodeGuid));
			const UFlowNode* ConnectedParentNode = TargetNode->GetFlowAsset()->GetNode(InputNode->GetGuid());
			const FProperty* OutputProperty = ConnectedParentNode->FindOutputPropertyByPinName(ConnectedPin.PinProperty.OutputPinName);

#define SET_PROPERTY_VALUE(Type)	return PropertyPathHelpers::SetPropertyValue(TargetNode, ConnectedPin.PinProperty.InputProperty->GetNameCPP(), *OutputProperty->ContainerPtrToValuePtr<Type>(ConnectedParentNode));
			if (FLOW_PROPERTY_IS(Property, Struct))
			{
				const FStructProperty* StructProperty = CastField<FStructProperty>(Property);

				UScriptStruct* GameplayTagContainerStruct = TBaseStructure<FGameplayTagContainer>::Get();
				UScriptStruct* GameplayTagStruct = TBaseStructure<FGameplayTag>::Get();
				UScriptStruct* VectorStruct = TBaseStructure<FVector>::Get();
				UScriptStruct* RotatorStruct = TBaseStructure<FRotator>::Get();
				UScriptStruct* TransformStruct = TBaseStructure<FTransform>::Get();

				if (StructProperty->Struct == GameplayTagContainerStruct)
				{
					SET_PROPERTY_VALUE(FGameplayTagContainer);
				}
				else if (StructProperty->Struct == GameplayTagStruct)
				{
					SET_PROPERTY_VALUE(FGameplayTag);
				}
				else if (StructProperty->Struct == VectorStruct)
				{
					SET_PROPERTY_VALUE(FVector);
				}
				else if (StructProperty->Struct == RotatorStruct)
				{
					SET_PROPERTY_VALUE(FRotator);
				}
				else if (StructProperty->Struct == TransformStruct)
				{
					SET_PROPERTY_VALUE(FTransform);
				}
			}
			else if (FLOW_PROPERTY_IS(Property, Object))
			{
				SET_PROPERTY_VALUE(UObject*);
			}
			else if (FLOW_PROPERTY_IS(Property, Float))
			{
				SET_PROPERTY_VALUE(float);
			}
			else if (FLOW_PROPERTY_IS(Property, Bool))
			{
				SET_PROPERTY_VALUE(bool);
			}
			else if (FLOW_PROPERTY_IS(Property, Byte))
			{
				SET_PROPERTY_VALUE(uint8);
			}
			else if (FLOW_PROPERTY_IS(Property, Int))
			{
				SET_PROPERTY_VALUE(int32);
			}
			else if (FLOW_PROPERTY_IS(Property, Int64))
			{
				SET_PROPERTY_VALUE(int64);
			}
			else if (FLOW_PROPERTY_IS(Property, Str))
			{
				SET_PROPERTY_VALUE(FString);
			}
			else if (FLOW_PROPERTY_IS(Property, Text))
			{
				SET_PROPERTY_VALUE(FText);
			}
			else if (FLOW_PROPERTY_IS(Property, Name))
			{
				SET_PROPERTY_VALUE(FName);
			}
#undef SET_PROPERTY_VALUE
		}

		return false;
	}
}
