// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "AddOns/FlowNodeAddOn.h"

#include "FlowLogChannels.h"

#include "Nodes/FlowNode.h"

#include "Misc/RuntimeErrors.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeAddOn)

UFlowNodeAddOn::UFlowNodeAddOn()
{
	// AddOns have no single creation choke point analogous to UFlowAsset::RegisterNode, so - unlike
	// UFlowNode - auto-mint a Guid here. See UFlowNodeBase::NodeGuid's doc comment.
	SetGuid(FGuid::NewGuid());

#if WITH_EDITOR
	NodeDisplayStyle = FlowNodeStyle::AddOn;
#endif
}

#if WITH_EDITOR
void UFlowNodeAddOn::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		// The constructor can record a CDO identity before serialized properties are copied.
		// At this hook, NodeGuid is the source identity that the copied graph pins actually use.
		ClearPreviousGuidForPinRemapping();

		SetGuid(FGuid::NewGuid());
	}
}

void UFlowNodeAddOn::PostEditImport()
{
	Super::PostEditImport();

	// Discard constructor history and remap from the imported, serialized identity.
	ClearPreviousGuidForPinRemapping();

	SetGuid(FGuid::NewGuid());
}
#endif

EFlowAddOnAcceptResult UFlowNodeAddOn::AcceptFlowNodeAddOnParent_Implementation(
	const UFlowNodeBase* ParentTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	// Subclasses may override this function to opt in to parent classes
	return EFlowAddOnAcceptResult::Undetermined;
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

void UFlowNodeAddOn::InitializeReplica()
{
	CacheFlowNode();

	Super::InitializeReplica();
}

UFlowNode* UFlowNodeAddOn::GetFlowNode() const
{
	// We are making the assumption that this would always be known during runtime 
	// and that we are not calling this method before the addon has been initialized.
	ensure(FlowNode);

	return FlowNode;
}

UFlowNode* UFlowNodeAddOn::FindOwningFlowNode() const
{
	UObject* OuterObject = GetOuter();
	UFlowNode* ParentFlowNode = nullptr;

	while (IsValid(OuterObject))
	{
		ParentFlowNode = Cast<UFlowNode>(OuterObject);
		if (ParentFlowNode)
		{
			break;
		}

		OuterObject = OuterObject->GetOuter();
	}

	return ParentFlowNode;
}

int32 UFlowNodeAddOn::GetRandomSeed() const
{
	if (ensure(FlowNode))
	{
		return FlowNode->GetRandomSeed();
	}

	return 0;
}

void UFlowNodeAddOn::NotifyPreloadComplete()
{
	if (ensure(FlowNode))
	{
		FlowNode->NotifyPreloadComplete();
	}
}

#if WITH_EDITOR
TArray<FFlowPin> UFlowNodeAddOn::GetContextInputs() const
{
	return GetPinsForContext(InputPins, Super::GetContextInputs());
}

TArray<FFlowPin> UFlowNodeAddOn::GetContextOutputs() const
{
	return GetPinsForContext(OutputPins, Super::GetContextOutputs());
}

void UFlowNodeAddOn::RequestReconstructionOnOwningFlowNode() const
{
	(void)OnAddOnRequestedParentReconstruction.ExecuteIfBound();
}
#endif // WITH_EDITOR

void UFlowNodeAddOn::CacheFlowNode()
{
	FlowNode = FindOwningFlowNode();

	ensureAsRuntimeWarning(FlowNode);
}

#if WITH_EDITOR
// Build an AddOn's context from the inherited context of the same direction before adding
// its own pins. This keeps nested input and output contributions separate during reconstruction.
TArray<FFlowPin> UFlowNodeAddOn::GetPinsForContext(
	const TArray<FFlowPin>& Context,
	const TArray<FFlowPin>& BaseContext) const
{
	TArray<FFlowPin> ContextPins = BaseContext;

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
#endif // WITH_EDITOR
