// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Graph/FlowNode_Start.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Start)

UFlowNode_Start::UFlowNode_Start(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Graph");
	NodeDisplayStyle = FlowNodeStyle::InOut;
	bCanDelete = bCanDuplicate = false;
#endif

	OutputPins = { UFlowNode::DefaultOutputPin };
}

void UFlowNode_Start::ExecuteInput(const FName& PinName)
{
	TriggerFirstOutput(true);
}

void UFlowNode_Start::SetDataPinValueSupplier(IFlowDataPinValueSupplierInterface* DataPinValueSupplier)
{
	FlowDataPinValueSupplierInterface = Cast<UObject>(DataPinValueSupplier);
}

#if WITH_EDITOR

bool UFlowNode_Start::TryAppendExternalInputPins(TArray<FFlowPin>& InOutPins) const
{
	// Add pins for all of the Flow DataPin Properties
	for (const FFlowNamedDataPinOutputProperty& DataPinProperty : OutputProperties)
	{
		if (DataPinProperty.IsValid())
		{
			InOutPins.AddUnique(DataPinProperty.CreateFlowPin());
		}
	}
	
	return !OutputProperties.IsEmpty();
}

#endif // WITH_EDITOR

// Must implement TrySupplyDataPinAs... for every EFlowPinType
FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

FFlowDataPinResult_Bool UFlowNode_Start::TrySupplyDataPinAsBool_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Bool SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsBool(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsBool_Implementation(PinName);
}

FFlowDataPinResult_Int UFlowNode_Start::TrySupplyDataPinAsInt_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Int SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsInt(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsInt_Implementation(PinName);
}

FFlowDataPinResult_Float UFlowNode_Start::TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Float SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsFloat(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsFloat_Implementation(PinName);
}

FFlowDataPinResult_Name UFlowNode_Start::TrySupplyDataPinAsName_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Name SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsName(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsName_Implementation(PinName);
}

FFlowDataPinResult_String UFlowNode_Start::TrySupplyDataPinAsString_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_String SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsString(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsString_Implementation(PinName);
}

FFlowDataPinResult_Text UFlowNode_Start::TrySupplyDataPinAsText_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Text SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsText(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsText_Implementation(PinName);
}

FFlowDataPinResult_Enum UFlowNode_Start::TrySupplyDataPinAsEnum_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Enum SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsEnum(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsEnum_Implementation(PinName);
}

FFlowDataPinResult_Vector UFlowNode_Start::TrySupplyDataPinAsVector_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Vector SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsVector(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsVector_Implementation(PinName);
}

FFlowDataPinResult_Rotator UFlowNode_Start::TrySupplyDataPinAsRotator_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Rotator SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsRotator(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsRotator_Implementation(PinName);
}

FFlowDataPinResult_Transform UFlowNode_Start::TrySupplyDataPinAsTransform_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Transform SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsTransform(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsTransform_Implementation(PinName);
}

FFlowDataPinResult_GameplayTag UFlowNode_Start::TrySupplyDataPinAsGameplayTag_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_GameplayTag SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsGameplayTag(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsGameplayTag_Implementation(PinName);
}

FFlowDataPinResult_GameplayTagContainer UFlowNode_Start::TrySupplyDataPinAsGameplayTagContainer_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_GameplayTagContainer SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsGameplayTagContainer(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsGameplayTagContainer_Implementation(PinName);
}

FFlowDataPinResult_InstancedStruct UFlowNode_Start::TrySupplyDataPinAsInstancedStruct_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_InstancedStruct SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsInstancedStruct(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsInstancedStruct_Implementation(PinName);
}

FFlowDataPinResult_Object UFlowNode_Start::TrySupplyDataPinAsObject_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Object SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsObject(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsObject_Implementation(PinName);
}

FFlowDataPinResult_Class UFlowNode_Start::TrySupplyDataPinAsClass_Implementation(const FName& PinName) const
{
	if (FlowDataPinValueSupplierInterface)
	{
		FFlowDataPinResult_Class SuppliedResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsClass(FlowDataPinValueSupplierInterface.GetObject(), PinName);

		if (SuppliedResult.Result == EFlowDataPinResolveResult::Success)
		{
			return SuppliedResult;
		}
	}

	return Super::TrySupplyDataPinAsClass_Implementation(PinName);
}
