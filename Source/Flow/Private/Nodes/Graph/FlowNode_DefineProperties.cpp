// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Graph/FlowNode_DefineProperties.h"
#include "Types/FlowPinTypesStandard.h"
#include "Types/FlowDataPinValuesStandard.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_DefineProperties)

UFlowNode_DefineProperties::UFlowNode_DefineProperties()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::Terminal;
	Category = TEXT("Graph");
#endif

	InputPins.Empty();
	OutputPins.Empty();

	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

void UFlowNode_DefineProperties::PostLoad()
{
	Super::PostLoad();

	if (!HasAnyFlags(RF_ArchetypeObject | RF_ClassDefaultObject))
	{
		// Migrate the named properties over to the new structs

		for (FFlowNamedDataPinProperty& NamedProperty : NamedProperties)
		{
			NamedProperty.FixupDataPinProperty();
		}
	}
}

#if WITH_EDITOR
bool UFlowNode_DefineProperties::SupportsContextPins() const
{
	return Super::SupportsContextPins() || !NamedProperties.IsEmpty();
}

void UFlowNode_DefineProperties::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChainEvent)
{
	Super::PostEditChangeChainProperty(PropertyChainEvent);

	if (PropertyChainEvent.PropertyChain.Num() == 0)
	{
		return;
	}

	auto& Property = PropertyChainEvent.PropertyChain.GetActiveMemberNode()->GetValue();

	// The DetailsCustomization for FFlowDataPinValue_Enum isn't being called when using an InstancedStruct
	// so we need to call OnEnumNameChanged refresh by hand...
	if (PropertyChainEvent.ChangeType == EPropertyChangeType::ValueSet &&
		Property->GetFName() == GET_MEMBER_NAME_CHECKED(FFlowDataPinOutputProperty_Enum, EnumName))
	{
		for (FFlowNamedDataPinProperty& NamedProperty : NamedProperties)
		{
			if (!NamedProperty.IsValid())
			{
				continue;
			}

			const FFlowDataPinValue& FlowDataPinProperty = NamedProperty.DataPinValue.Get();

			if (FlowDataPinProperty.GetPinTypeName() == FFlowPinType_Enum::GetPinTypeNameStatic())
			{
				FFlowDataPinValue_Enum& EnumProperty = NamedProperty.DataPinValue.GetMutable<FFlowDataPinValue_Enum>();
				EnumProperty.OnEnumNameChanged();
			}
		}
	}

	constexpr EPropertyChangeType::Type RelevantChangeTypesForReconstructionMask =
		EPropertyChangeType::Unspecified |
		EPropertyChangeType::ArrayAdd |
		EPropertyChangeType::ArrayRemove |
		EPropertyChangeType::ArrayClear |
		EPropertyChangeType::ValueSet |
		EPropertyChangeType::Redirected |
		EPropertyChangeType::ArrayMove;

	const uint32 PropertyChangedTypeFlags = (PropertyChainEvent.ChangeType & RelevantChangeTypesForReconstructionMask);
	const bool bIsRelevantChangeTypeForReconstruction = PropertyChangedTypeFlags != 0;
	const bool bChangedOutputProperties = Property->GetFName() == GET_MEMBER_NAME_CHECKED(UFlowNode_DefineProperties, NamedProperties);
	if (bIsRelevantChangeTypeForReconstruction && bChangedOutputProperties)
	{
		OnReconstructionRequested.ExecuteIfBound();
	}
}
#endif // WITH_EDITOR

bool UFlowNode_DefineProperties::TryFormatTextWithNamedPropertiesAsParameters(const FText& FormatText, FText& OutFormattedText) const
{
	if (NamedProperties.IsEmpty())
	{
		return false;
	}

	FFormatNamedArguments Arguments;
	for (const FFlowNamedDataPinProperty& NamedProperty : NamedProperties)
	{
		if (!NamedProperty.Name.IsValid())
		{
			LogWarning(TEXT("Could not format text with a nameless named property"));
		}
		else if (!TryAddValueToFormatNamedArguments(NamedProperty, Arguments))
		{
			LogWarning(FString::Printf(TEXT("Could not format text for named property %s"), *NamedProperty.Name.ToString()));
		}
	}

	OutFormattedText = FText::Format(FormatText, Arguments);

	return true;
}

#if WITH_EDITOR
void UFlowNode_DefineProperties::OnPostEditEnsureAllNamedPropertiesPinDirection(const FProperty& Property, bool bIsInput)
{
	if (Property.GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, NamedProperties))
	{
		for (FFlowNamedDataPinProperty& NamedProperty : NamedProperties)
		{
			const UScriptStruct* ScriptStruct = NamedProperty.DataPinValue.GetScriptStruct();
			if (IsValid(ScriptStruct) && ScriptStruct->IsChildOf<FFlowDataPinValue>())
			{
				FFlowDataPinValue& Value = NamedProperty.DataPinValue.GetMutable<FFlowDataPinValue>();
				Value.bIsInputPin = bIsInput;
			}
		}
	}
}
#endif