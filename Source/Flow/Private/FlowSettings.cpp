// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowSettings.h"
#include "FlowComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowSettings)

UFlowSettings::UFlowSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bCreateFlowSubsystemOnClients(true)
	, bWarnAboutMissingIdentityTags(true)
	, bLogOnSignalDisabled(true)
	, bLogOnSignalPassthrough(true)
	, bUseAdaptiveNodeTitles(false)
	, DefaultExpectedOwnerClass(UFlowComponent::StaticClass())
{
}

#if WITH_EDITOR
void UFlowSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED( UFlowSettings, bUseAdaptiveNodeTitles ))
	{
		(void) OnAdaptiveNodeTitlesChanged.ExecuteIfBound();
	}
}
#endif

UClass* UFlowSettings::GetDefaultExpectedOwnerClass() const
{
	return CastChecked<UClass>(TryResolveOrLoadSoftClass(DefaultExpectedOwnerClass), ECastCheckedType::NullAllowed);
}

UClass* UFlowSettings::TryResolveOrLoadSoftClass(const FSoftClassPath& SoftClassPath)
{
	if (UClass* Resolved = SoftClassPath.ResolveClass())
	{
		return Resolved;
	}

	return SoftClassPath.TryLoadClass<UObject>();
}