#pragma once

#include "CoreMinimal.h"

/**
** Minimalistic form of breakpoint debugger
** See BehaviorTreeDebugger for a more complex example
 */
class FFlowDebugger
{
public:
	FFlowDebugger();
	~FFlowDebugger();

public:
	static void StopPlaySession();
	static void PausePlaySession();
	static void ResumePlaySession();

	static bool IsPlaySessionPaused();
	static bool IsPlaySessionRunning();
};
