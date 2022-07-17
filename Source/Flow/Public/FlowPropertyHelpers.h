#pragma once

#include "PropertyPathHelpers.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowPin.h"
#include "FlowAsset.h"
#include "FlowModule.h"

#define FLOW_PROPERTY_IS(PropertyRef, Type)							PropertyRef->IsA(F##Type##Property::StaticClass())

namespace FlowPropertyHelpers
{
	struct FPropertyInfo
	{
		UPROPERTY()
		FString Name;
		UPROPERTY()
		FString Tooltip;

		FPropertyInfo(FString InName, FString InTooltip):
			Name(InName),
			Tooltip(InTooltip)
		{
		}
	};

	/**
	* public
	* static FlowPropertyHelpers::IsPropertySupported
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
	* static FlowPropertyHelpers::IsPropertyExposedAsInput
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
	* static FlowPropertyHelpers::IsPropertyExposedAsOutput
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

	static bool SetPropertyValue(UFlowNode* TargetNode, const FFlowInputOutputPin& ConnectedPin, FProperty* Property)
	{
		if (Property->GetFName().IsEqual(ConnectedPin.InputProperty->GetFName()))
		{
			const UFlowNode* InputNode = Cast<UFlowNode>(TargetNode->GetFlowAsset()->GetNodes().FindRef(ConnectedPin.OutputNodeGuid));
			UFlowNode* ConnectedParentNode = TargetNode->GetFlowAsset()->GetNode(InputNode->GetGuid());
			const FProperty* OutputProperty = ConnectedParentNode->FindOutputPropertyByPinName(ConnectedPin.OutputPinName);
			UObject* InputVariableHolder = TargetNode->GetVariableHolder();
			UObject* OutputVariableHolder = ConnectedParentNode->GetVariableHolder();

#define SET_PROPERTY_VALUE(Type)	return PropertyPathHelpers::SetPropertyValue(InputVariableHolder, ConnectedPin.InputProperty->GetNameCPP(), *OutputProperty->ContainerPtrToValuePtr<Type>(OutputVariableHolder));
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
				FText InputValue = CastField<FTextProperty>(Property)->GetPropertyValue(Property->ContainerPtrToValuePtr<void>(InputVariableHolder));
				FText OutputValue = CastField<FTextProperty>(OutputProperty)->GetPropertyValue(OutputProperty->ContainerPtrToValuePtr<void>(OutputVariableHolder));
				UE_LOG(LogTemp, Warning, TEXT("Output %s -> Input %s"), *OutputValue.ToString(), *InputValue.ToString())
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

	static FORCEINLINE_DEBUGGABLE TMultiMap<TWeakObjectPtr<UObject>, FProperty*> GatherProperties(UObject* Object, bool (&Predicate)(const FProperty*))
	{
		TMultiMap<TWeakObjectPtr<UObject>, FProperty*> Properties;

		for (TFieldIterator<FProperty> PropertyIterator(Object->GetClass()); PropertyIterator; ++PropertyIterator)
		{
			FProperty* Property = *PropertyIterator;
			if (Predicate(Property))
			{
				const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
				if (!StructProperty)
				{
					Properties.Add(Object, Property);
				}
				else
				{
					const UScriptStruct* GameplayTagContainerStruct = TBaseStructure<FGameplayTagContainer>::Get();
					const UScriptStruct* GameplayTagStruct = TBaseStructure<FGameplayTag>::Get();
					const UScriptStruct* VectorStruct = TBaseStructure<FVector>::Get();
					const UScriptStruct* RotatorStruct = TBaseStructure<FRotator>::Get();
					const UScriptStruct* TransformStruct = TBaseStructure<FTransform>::Get();
					if (StructProperty->Struct == GameplayTagContainerStruct
						|| StructProperty->Struct == GameplayTagStruct
						|| StructProperty->Struct == VectorStruct
						|| StructProperty->Struct == RotatorStruct
						|| StructProperty->Struct == TransformStruct)
					{
						Properties.Add(Object, Property);
					}
				}
			}
		}

		return Properties;
	}
}
