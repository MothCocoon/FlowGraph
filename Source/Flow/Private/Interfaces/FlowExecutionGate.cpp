// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Interfaces/FlowExecutionGate.h"

#include "FlowLogChannels.h"
#include "Nodes/FlowNode.h"

#include "CoreGlobals.h"
#include "Templates/UnrealTemplate.h"

IFlowExecutionGate* FFlowExecutionGate::Gate = nullptr;

bool FFlowExecutionGate::bIsBreaking = false;

void FFlowExecutionGate::SetGate(IFlowExecutionGate* InGate)
{
	Gate = InGate;
}

IFlowExecutionGate* FFlowExecutionGate::GetGate()
{
	return Gate;
}

EFlowBreakAction FFlowExecutionGate::BreakIfRequested(const FFlowBreakContext& Context)
{
	if (Gate == nullptr || !IsValid(Context.Node))
	{
		return EFlowBreakAction::Continue;
	}

	// Halting suspends the calling stack and pumps the owning application from within it, which is
	// only meaningful on the game thread.
	if (!IsInGameThread())
	{
		return EFlowBreakAction::Continue;
	}

	// A halt pumps UI that can drive Flow again, and nested halts are not supported. Ignoring the
	// inner break keeps the outer one intact, which is the one the user is looking at.
	if (bIsBreaking || GIntraFrameDebuggingGameThread)
	{
		static bool bLoggedNestedBreak = false;
		if (!bLoggedNestedBreak)
		{
			bLoggedNestedBreak = true;

			UE_LOG(LogFlow, Warning,
				TEXT("Ignoring a Flow breakpoint on node '%s' pin '%s': execution is already halted at a breakpoint. ")
				TEXT("Nested breaks are not supported, so this pin will not stop execution while halted."),
				*Context.Node->GetName(), *Context.PinName.ToString());
		}

		return EFlowBreakAction::Continue;
	}

	if (!Gate->WantsFlowBreak(Context))
	{
		return EFlowBreakAction::Continue;
	}

	TGuardValue<bool> BreakingGuard(bIsBreaking, true);

	return Gate->BreakFlowExecution(Context);
}
