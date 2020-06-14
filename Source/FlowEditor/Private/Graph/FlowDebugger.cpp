#include "Graph/FlowDebugger.h"

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

void FFlowDebugger::StopPlaySession()
{
	if (GUnrealEd->PlayWorld)
	{
		GEditor->RequestEndPlayMap();

		// Flow: comments below comes from BehaviorTree code...
		// @TODO: we need a unified flow to leave debugging mode from the different debuggers to prevent strong coupling between modules.
		// Each debugger (Blueprint & BehaviorTree for now) could then take the appropriate actions to resume the session.
		if (FSlateApplication::Get().InKismetDebuggingMode())
		{
			FSlateApplication::Get().LeaveDebuggingMode();
		}
	}
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

void FFlowDebugger::ResumePlaySession()
{
	bool bResumed = false;
	ForEachGameWorld([&](UWorld* World)
	{
		if (World->bDebugPauseExecution)
		{
			World->bDebugPauseExecution = false;
			bResumed = true;
		}
	});

	if (bResumed)
	{
		// Flow: comments below comes from BehaviorTree code...
		// @TODO: we need a unified flow to leave debugging mode from the different debuggers to prevent strong coupling between modules.
		// Each debugger (Blueprint & BehaviorTree for now) could then take the appropriate actions to resume the session.
		if (FSlateApplication::Get().InKismetDebuggingMode())
		{
			FSlateApplication::Get().LeaveDebuggingMode();
		}

		GUnrealEd->PlaySessionResumed();
	}
}

bool FFlowDebugger::IsPlaySessionPaused()
{
	return AreAllGameWorldPaused();
}

bool FFlowDebugger::IsPlaySessionRunning()
{
	return !AreAllGameWorldPaused();
}
