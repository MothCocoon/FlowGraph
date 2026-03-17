// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowSettings.h"
#include "FlowComponent.h"
#include "FlowLogChannels.h"
#include "Policies/FlowStandardPinConnectionPolicies.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowSettings)

UFlowSettings::UFlowSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FlowPinConnectionPolicy(FFlowPinConnectionPolicy_VeryRelaxed::StaticStruct())
	, bDeferTriggeredOutputsWhileTriggering(true)
	, bLogOnSignalDisabled(true)
	, bLogOnSignalPassthrough(true)
	, bCreateFlowSubsystemOnClients(true)
	, bUseAdaptiveNodeTitles(false)
	, DefaultExpectedOwnerClass(UFlowComponent::StaticClass())
	, bWarnAboutMissingIdentityTags(true)
{
}

#if WITH_EDITOR
void UFlowSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowSettings, bUseAdaptiveNodeTitles))
	{
		(void)OnAdaptiveNodeTitlesChanged.ExecuteIfBound();
	}
}

void UFlowSettings::GetFlowPinConnectionPolicy(TInstancedStruct<FFlowPinConnectionPolicy>& MutablePinConnectionPolicy) const
{
	if (!ensure(FlowPinConnectionPolicy.IsValid()))
	{
		return;
	}

	const UScriptStruct* BaseStruct = FFlowPinConnectionPolicy::StaticStruct();
	const UScriptStruct* DerivedStruct = FlowPinConnectionPolicy.GetScriptStruct();

	// Accept FFlowPinConnectionPolicy or any struct derived from it
	if (!ensure(IsValid(DerivedStruct)) || !DerivedStruct->IsChildOf(BaseStruct))
	{
		UE_LOG(LogFlow, Error,
			TEXT("FlowPinConnectionPolicy must derive from %s, but was %s"),
			*GetNameSafe(BaseStruct),
			*GetNameSafe(DerivedStruct));

		return;
	}

	// Copy the instanced struct payload (preserving the actual derived type)
	MutablePinConnectionPolicy.InitializeAsScriptStruct(DerivedStruct, FlowPinConnectionPolicy.GetMemory());
}

#endif

UClass* UFlowSettings::GetDefaultExpectedOwnerClass() const
{
	UClass* Result = DefaultExpectedOwnerClass.ResolveClass();

	if (Result == nullptr)
	{
		Result = DefaultExpectedOwnerClass.TryLoadClass<UObject>();
	}

	return CastChecked<UClass>(Result, ECastCheckedType::NullAllowed);
}
