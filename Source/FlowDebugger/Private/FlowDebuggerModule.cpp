// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowDebuggerModule.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FlowDebuggerModule"

void FFlowDebuggerModule::StartupModule()
{
}

void FFlowDebuggerModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FFlowDebuggerModule, FlowDebugger)
