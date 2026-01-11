// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Types/FlowNamedDataPinProperty.h"
#include "Types/FlowDataPinPropertyToValueMigration.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNamedDataPinProperty)

#define LOCTEXT_NAMESPACE "FlowNamedDataPinProperty"

#if WITH_EDITOR
FFlowPin FFlowNamedDataPinProperty::CreateFlowPin() const
{
	if (const FFlowDataPinValue* FlowDataPinValuePtr = DataPinValue.GetPtr<FFlowDataPinValue>())
	{
		if (const FFlowPinType* FlowPinType = FFlowPinType::LookupPinType(FlowDataPinValuePtr->GetPinTypeName()))
		{
			return FlowPinType->CreateFlowPinFromValueWrapper(Name, *FlowDataPinValuePtr);
		}
	}

	return FFlowPin();
}

FText FFlowNamedDataPinProperty::BuildHeaderText() const
{
	FFlowPinTypeName PinTypeName;

	if (const FFlowDataPinValue* FlowDataPinValuePtr = DataPinValue.GetPtr<FFlowDataPinValue>())
	{
		PinTypeName = FlowDataPinValuePtr->GetPinTypeName();
	}

	return FText::Format(LOCTEXT("FlowNamedFFlowDataPinValueHeader", "{0} ({1})"), { FText::FromName(Name), FText::FromString(PinTypeName.ToString()) });
}
#endif

#undef LOCTEXT_NAMESPACE
