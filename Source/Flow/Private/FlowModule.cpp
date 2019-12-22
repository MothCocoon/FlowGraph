#include "FlowModule.h"

#define LOCTEXT_NAMESPACE "Flow"

void FFlowModule::StartupModule()
{
}

void FFlowModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFlowModule, Flow)
DEFINE_LOG_CATEGORY(LogFlow);