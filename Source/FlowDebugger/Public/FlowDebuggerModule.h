// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Modules/ModuleInterface.h"

class FLOWDEBUGGER_API FFlowDebuggerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
