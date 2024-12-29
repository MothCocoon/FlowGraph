// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "AddOns/FlowNodeAddOn.h"

#include "FlowLogChannels.h"
#include "Nodes/FlowNode.h"

#include "Misc/RuntimeErrors.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn)

UFlowNodeAddOn::UFlowNodeAddOn()
{
#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::AddOn;
#endif
}

void UFlowNodeAddOn::InitializeInstance()
{
	CacheFlowNode();

	Super::InitializeInstance();
}

void UFlowNodeAddOn::DeinitializeInstance()
{
	Super::DeinitializeInstance();

	FlowNode = nullptr;
}

void UFlowNodeAddOn::TriggerFirstOutput(const bool bFinish)
{
	if (ensure(FlowNode))
	{
		FlowNode->TriggerFirstOutput(bFinish);
	}
}

void UFlowNodeAddOn::TriggerOutput(const FName PinName, const bool bFinish, const EFlowPinActivationType ActivationType)
{
	if (ensure(FlowNode))
	{
		FlowNode->TriggerOutput(PinName, bFinish, ActivationType);
	}
}

void UFlowNodeAddOn::Finish()
{
	if (ensure(FlowNode))
	{
		FlowNode->Finish();
	}
}

EFlowAddOnAcceptResult UFlowNodeAddOn::AcceptFlowNodeAddOnParent_Implementation(
	const UFlowNodeBase* ParentTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	// Subclasses may override this function to opt in to parent classes
	return EFlowAddOnAcceptResult::Undetermined;
}

UFlowNode* UFlowNodeAddOn::GetFlowNode() const
{
	// We are making the assumption that this would addlways be known
	// during runtime and that we are not calling this method before the addon has been
	// initialized.
	ensure(FlowNode);

	return FlowNode;
}

bool UFlowNodeAddOn::IsSupportedInputPinName(const FName& PinName) const
{
	if (InputPins.IsEmpty())
	{
		return true;
	}

	if (const FFlowPin* FoundFlowPin = FindFlowPinByName(PinName, InputPins))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void UFlowNodeAddOn::CacheFlowNode()
{
	UObject* OuterObject = GetOuter();
	while (IsValid(OuterObject))
	{
		FlowNode = Cast<UFlowNode>(OuterObject);
		if (FlowNode)
		{
			break;
		}

		OuterObject = OuterObject->GetOuter();
	}

	ensureAsRuntimeWarning(FlowNode);
}

#if WITH_EDITOR
TArray<FFlowPin> UFlowNodeAddOn::GetPinsForContext(const TArray<FFlowPin>& Context) const
{
	TArray<FFlowPin> ContextPins = Super::GetContextInputs();

	ContextPins.Reserve(ContextPins.Num() + Context.Num());
	
	for (const FFlowPin& InputPin : Context)
	{
		if (InputPin.IsValid())
		{
			ContextPins.Add(InputPin);
		}
		else
		{
			UE_LOG(LogFlow, Warning, TEXT("Addon %s has invalid pins (name: None), you should clean these up."), *GetName());
		}
	}

	return ContextPins;
}

TArray<FFlowPin> UFlowNodeAddOn::GetContextInputs() const
{
	return GetPinsForContext(InputPins);
}

TArray<FFlowPin> UFlowNodeAddOn::GetContextOutputs() const
{
	return GetPinsForContext(OutputPins);
}
#endif // WITH_EDITOR
