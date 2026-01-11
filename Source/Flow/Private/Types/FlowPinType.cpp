// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowPinType.h"
#include "Types/FlowDataPinValue.h"
#include "Types/FlowStructUtils.h"
#include "Nodes/FlowNode.h"
#include "FlowPinSubsystem.h"
#include "FlowLogChannels.h"

#if WITH_EDITOR
#include "PropertyHandle.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowPinType)

const FFlowPinTypeName FFlowPinType::PinTypeNameUnknown = FFlowPinTypeName(FFlowPinTypeNamesStandard::PinTypeNameUnknown);

const FFlowPinType* FFlowPinType::LookupPinType(const FFlowPinTypeName& PinTypeName)
{
	const FFlowPinType* PinType = UFlowPinSubsystem::Get()->FindPinType(PinTypeName);

	if (!PinType)
	{
		UE_LOG(LogFlow, Error, TEXT("Could not find pin type %s in FlowPinSubsystem"), *PinTypeName.ToString());
		return nullptr;
	}

	return PinType;
}

bool FFlowPinType::ResolveAndFormatPinValue(const UFlowNodeBase& Node, const FName& PinName, FFormatArgumentValue& OutValue) const
{
	return false;
}

bool FFlowPinType::PopulateResult(const UObject& PropertyOwnerObject, const UFlowNode& Node, const FFlowPin& Pin, FFlowDataPinResult& OutResult) const
{
	OutResult.Result = EFlowDataPinResolveResult::FailedMismatchedType;
	return false;
}

#if WITH_EDITOR
FFlowPin FFlowPinType::CreateFlowPinFromProperty(const FProperty& Property, void const* InContainer) const
{
	FFlowPin NewFlowPin;

	if (const FFlowDataPinValue* DataPinValue = FlowStructUtils::CastStructValue<FFlowDataPinValue>(&Property, InContainer))
	{
		// Create the pin from a FFlowDataPinValue property wrapper
		NewFlowPin = CreateFlowPinFromValueWrapper(Property.GetFName(), *DataPinValue);
	}
	else
	{
		// Create the pin from a native property
		NewFlowPin.PinName = Property.GetFName();
		NewFlowPin.SetPinTypeName(GetPinTypeName());

		FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2);
		if (CastField<FArrayProperty>(&Property))
		{
			NewFlowPin.ContainerType = EPinContainerType::Array;
		}

		UObject* SubCategoryObject = GetPinSubCategoryObjectFromProperty(&Property, InContainer, DataPinValue);
		NewFlowPin.SetPinSubCategoryObject(SubCategoryObject);
	}

	// Common property settings for both versions
	NewFlowPin.PinFriendlyName = Property.GetDisplayNameText();
	NewFlowPin.PinToolTip = Property.GetToolTipText().ToString();

	return NewFlowPin;
}

FFlowPin FFlowPinType::CreateFlowPinFromValueWrapper(const FName& PinName, const FFlowDataPinValue& Wrapper) const
{
	FFlowPin NewFlowPin(PinName);

	FLOW_ASSERT_ENUM_MAX(EFlowDataMultiType, 2);
	if (Wrapper.IsArray())
	{
		NewFlowPin.ContainerType = EPinContainerType::Array;
	}

	constexpr const FProperty* Property = nullptr;
	constexpr void const* InContainer = nullptr;
	UObject* SubCategoryObject = GetPinSubCategoryObjectFromProperty(Property, InContainer, &Wrapper);
	NewFlowPin.SetPinSubCategoryObject(SubCategoryObject);

	// Common property settings for both versions
	NewFlowPin.SetPinTypeName(GetPinTypeName());

	return NewFlowPin;
}

TSharedPtr<IPropertyHandle> FFlowPinType::GetValuesHandle(const TSharedRef<IPropertyHandle>& FlowDataPinValuePropertyHandle) const
{
	return FlowDataPinValuePropertyHandle.Get().GetChildHandle(TEXT("Values"));
}
#endif