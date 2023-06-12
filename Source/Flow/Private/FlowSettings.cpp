// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowSettings.h"
#include "FlowComponent.h"
#include "FlowOwnerInterface.h"

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