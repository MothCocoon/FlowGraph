// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Logging/LogMacros.h"
#include "Modules/ModuleInterface.h"

class FFlowModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
