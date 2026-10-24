// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Modules/ModuleManager.h"
#include "Misc/CoreDelegates.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

#include "FlowMCPToolset.h"
#include "FlowNodeUsageIndex.h"

class FFlowGraphCourierModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddLambda([]()
		{
			UToolsetRegistry::RegisterToolsetClass(UFlowMCPToolset::StaticClass());
		});
	}

	virtual void ShutdownModule() override
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);

		// The usage index registers its own save hook lazily on first build, so unhooking here is what
		// keeps a stale callback from surviving the module.
		FFlowNodeUsageIndex::Get().UnregisterCallbacks();

		if (UObjectInitialized())
		{
			UToolsetRegistry::UnregisterToolsetClass(UFlowMCPToolset::StaticClass());
		}
	}

private:
	FDelegateHandle PostEngineInitHandle;
};

IMPLEMENT_MODULE(FFlowGraphCourierModule, FlowGraphCourier)
