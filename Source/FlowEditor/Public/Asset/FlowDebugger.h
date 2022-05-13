// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

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

	static void PausePlaySession();
	static bool IsPlaySessionPaused();
};
