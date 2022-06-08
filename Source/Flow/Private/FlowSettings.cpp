// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowSettings.h"

UFlowSettings::UFlowSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bCreateFlowSubsystemOnClients(true)
	, bWarnAboutMissingIdentityTags(true)
{
}
