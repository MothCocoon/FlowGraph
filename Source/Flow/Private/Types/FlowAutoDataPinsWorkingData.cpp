// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowAutoDataPinsWorkingData.h"
#include "FlowLogChannels.h"
#include "Types/FlowDataPinValue.h"
#include "Types/FlowStructUtils.h"

#if WITH_EDITOR
bool FFlowAutoDataPinsWorkingData::DidAutoInputDataPinsChange() const
{
	return !FFlowPin::DeepArePinArraysMatching(AutoInputDataPinsPrev, AutoInputDataPinsNext);
}

bool FFlowAutoDataPinsWorkingData::DidAutoOutputDataPinsChange() const
{
	return !FFlowPin::DeepArePinArraysMatching(AutoOutputDataPinsPrev, AutoOutputDataPinsNext);
}

void FFlowAutoDataPinsWorkingData::AddFlowDataPinsForClassProperties(const UObject& ObjectContainer)
{
	// Try to harvest pins to auto-generate and/or bind to for each property in the flow node
	UClass* Class = ObjectContainer.GetClass();
	for (TFieldIterator<FProperty> PropertyIt(Class); PropertyIt; ++PropertyIt)
	{
		AddFlowDataPinForProperty(*PropertyIt, ObjectContainer);
	}
}

void FFlowAutoDataPinsWorkingData::AddFlowDataPinForProperty(const FProperty* Property, const UObject& ObjectContainer)
{
	bool bIsInputPin = false;

	const FString* AutoPinType = nullptr;
	const FString* SourceForOutputFlowPinName = nullptr;
	const FString* DefaultForInputFlowPinName = nullptr;

	const void* Container = &ObjectContainer;

	const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
	const FFlowDataPinValue* DataPinValue = nullptr;
	if (StructProperty && StructProperty->Struct)
	{
		const UScriptStruct* ScriptStruct = StructProperty->Struct;

		AutoPinType = ScriptStruct->FindMetaData(FFlowPin::MetadataKey_FlowPinType);
		SourceForOutputFlowPinName = ScriptStruct->FindMetaData(FFlowPin::MetadataKey_SourceForOutputFlowPin);
		DefaultForInputFlowPinName = ScriptStruct->FindMetaData(FFlowPin::MetadataKey_DefaultForInputFlowPin);

		// For blueprint use, we allow the Value structs to set input pins via editor-only data
		DataPinValue = FlowStructUtils::CastStructValue<FFlowDataPinValue>(StructProperty, Container);
		if (DataPinValue)
		{
			bIsInputPin = DataPinValue->IsInputPin();
		}
	}

	if (!AutoPinType)
	{
		AutoPinType = Property->FindMetaData(FFlowPin::MetadataKey_FlowPinType);

		if (!AutoPinType)
		{
			return;
		}
	}

	const FFlowPinType* FlowPinType = FFlowPinType::LookupPinType(FFlowPinTypeName(*AutoPinType));
	if (!FlowPinType)
	{
		UE_LOG(LogFlow, Error, TEXT("Unknown pin type %s for property %s"), **AutoPinType, *Property->GetName());

		return;
	}

	if (!SourceForOutputFlowPinName)
	{
		SourceForOutputFlowPinName = Property->FindMetaData(FFlowPin::MetadataKey_SourceForOutputFlowPin);
	}

	if (!DefaultForInputFlowPinName)
	{
		DefaultForInputFlowPinName = Property->FindMetaData(FFlowPin::MetadataKey_DefaultForInputFlowPin);
	}

	if (SourceForOutputFlowPinName && DefaultForInputFlowPinName)
	{
		UE_LOG(LogFlow, Error, TEXT("Error.  A property cannot be both a %s and %s"),
		       *FFlowPin::MetadataKey_SourceForOutputFlowPin.ToString(),
		       *FFlowPin::MetadataKey_DefaultForInputFlowPin.ToString());

		return;
	}

	bIsInputPin = bIsInputPin || DefaultForInputFlowPinName != nullptr;

	// Default assumption is the pin will be an output pin, unless metadata specifies otherwise
	TArray<FFlowPin>* FlowPinArray = bIsInputPin ? &AutoInputDataPinsNext : &AutoOutputDataPinsNext;

	// Create the new FlowPin
	FFlowPin NewFlowPin = FlowPinType->CreateFlowPinFromProperty(*Property, Container);

	// Potentially override the PinFriendlyName if the metadata specified an alternative
	if (DefaultForInputFlowPinName)
	{
		const FString SpecifyInputPinNameString = *DefaultForInputFlowPinName;
		if (SpecifyInputPinNameString.Len() > 0)
		{
			NewFlowPin.PinFriendlyName = FText::FromString(SpecifyInputPinNameString);
		}
	}
	else if (SourceForOutputFlowPinName)
	{
		const FString SpecifyOutputPinNameString = *SourceForOutputFlowPinName;
		if (SpecifyOutputPinNameString.Len() > 0)
		{
			NewFlowPin.PinFriendlyName = FText::FromString(SpecifyOutputPinNameString);
		}
	}

	FlowPinArray->Add(NewFlowPin);

	if (DataPinValue)
	{
		// Store the PropertyPinName in the property, for blueprint lookup functions.
		DataPinValue->PropertyPinName = NewFlowPin.PinName;
	}
}

#endif
