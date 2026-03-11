// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowSettings.h"
#include "FlowComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowSettings)

UFlowSettings::UFlowSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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
