// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Engine/DeveloperSettings.h"

#include "FlowDebuggerTypes.h"
#include "FlowDebuggerSettings.generated.h"

/**
 * 
 */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Flow Debugger"))
class FLOWDEBUGGER_API UFlowDebuggerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFlowDebuggerSettings();

	static UFlowDebuggerSettings* Get() { return StaticClass()->GetDefaultObject<UFlowDebuggerSettings>(); }

	UPROPERTY(config)
	TMap<FGuid, FFlowTraitSettings> PerNodeTraits;
};
