#include "Asset/FlowDebugger.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Templates/Function.h"
#include "UnrealEd.h"

FFlowDebugger::FFlowDebugger()
{
}

FFlowDebugger::~FFlowDebugger()
{
}

void ForEachGameWorld(const TFunction<void(UWorld*)>& Func)
{
	for (const FWorldContext& PieContext : GUnrealEd->GetWorldContexts())
	{
		UWorld* PlayWorld = PieContext.World();
		if (PlayWorld && PlayWorld->IsGameWorld())
		{
			Func(PlayWorld);
		}
	}
}

bool AreAllGameWorldPaused()
{
	bool bPaused = true;
	ForEachGameWorld([&](UWorld* World)
	{
		bPaused = bPaused && World->bDebugPauseExecution;
	});
	return bPaused;
}

void FFlowDebugger::PausePlaySession()
{
	bool bPaused = false;
	ForEachGameWorld([&](UWorld* World)
	{
		if (!World->bDebugPauseExecution)
		{
			World->bDebugPauseExecution = true;
			bPaused = true;
		}
	});
	if (bPaused)
	{
		GUnrealEd->PlaySessionPaused();
	}
}

bool FFlowDebugger::IsPlaySessionPaused()
{
	return AreAllGameWorldPaused();
}
