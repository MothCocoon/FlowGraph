// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Subsystems/EngineSubsystem.h"

#include "Debugger/FlowDebuggerTypes.h"
#include "FlowDebuggerSubsystem.generated.h"

class UEdGraphNode;
class UEdGraphPin;

/**
 * Persistent subsystem supporting Flow Graph debugging.
 * It might be utilized to use cook-specific graph debugger.
 */
UCLASS()
class FLOWDEBUGGER_API UFlowDebuggerSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	UFlowDebuggerSubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void PausePlaySession() {}
	virtual bool IsPlaySessionPaused() { return false; }

	virtual void AddBreakpoint(const UEdGraphNode* Node);
	virtual void AddBreakpoint(const UEdGraphPin* Pin);

	virtual void RemoveAllBreakpoints(const UEdGraphNode* Node);
	virtual void RemoveNodeBreakpoint(const UEdGraphNode* Node);
	virtual void RemovePinBreakpoint(const UEdGraphPin* Pin);

	/** Removes obsolete pin breakpoints for provided. Pin list can be changed during node reconstruction. */
	virtual void RemoveObsoletePinBreakpoints(const UEdGraphNode* Node);

	virtual void ToggleBreakpoint(const UEdGraphNode* Node);
	virtual void ToggleBreakpoint(const UEdGraphPin* Pin);
	
	virtual FFlowBreakpoint* FindBreakpoint(const UEdGraphNode* Node);
	virtual FFlowBreakpoint* FindBreakpoint(const UEdGraphPin* Pin);

	virtual void SetBreakpointEnabled(const UEdGraphNode* Node, bool bEnabled);
	virtual void SetBreakpointEnabled(const UEdGraphPin* Pin, bool bEnabled);

	virtual bool IsBreakpointEnabled(const UEdGraphNode* Node);
	virtual bool IsBreakpointEnabled(const UEdGraphPin* Pin);

	virtual bool MarkAsHit(const UEdGraphNode* Node);
	virtual bool MarkAsHit(const UEdGraphPin* Pin);

	virtual void ResetHit(const UEdGraphNode* Node);
	virtual void ResetHit(const UEdGraphPin* Pin);

	virtual bool IsBreakpointHit(const UEdGraphNode* Node);
	virtual bool IsBreakpointHit(const UEdGraphPin* Pin);

protected:
	/** Saves any modifications made to breakpoints */
	virtual void SaveSettings();
};
