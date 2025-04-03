// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Subsystems/EngineSubsystem.h"

#include "Debugger/FlowDebuggerTypes.h"
#include "FlowDebuggerSubsystem.generated.h"

class UEdGraphNode;
class UFlowAsset;

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

protected:
	virtual void OnInstancedTemplateAdded(UFlowAsset* AssetTemplate);
	virtual void OnInstancedTemplateRemoved(UFlowAsset* AssetTemplate) const;

	virtual void OnPinTriggered(const FGuid& NodeGuid, const FName& PinName);

public:
	virtual void AddBreakpoint(const FGuid& NodeGuid);
	virtual void AddBreakpoint(const FGuid& NodeGuid, const FName& PinName);

	virtual void RemoveAllBreakpoints(const FGuid& NodeGuid);
	virtual void RemoveNodeBreakpoint(const FGuid& NodeGuid);
	virtual void RemovePinBreakpoint(const FGuid& NodeGuid, const FName& PinName);

#if WITH_EDITOR
	/** Removes obsolete pin breakpoints for provided. Pin list can be changed during node reconstruction. */
	virtual void RemoveObsoletePinBreakpoints(const UEdGraphNode* Node);
#endif

	virtual void ToggleBreakpoint(const FGuid& NodeGuid);
	virtual void ToggleBreakpoint(const FGuid& NodeGuid, const FName& PinName);

	virtual FFlowBreakpoint* FindBreakpoint(const FGuid& NodeGuid);
	virtual FFlowBreakpoint* FindBreakpoint(const FGuid& NodeGuid, const FName& PinName);

	virtual void SetBreakpointEnabled(const FGuid& NodeGuid, bool bEnabled);
	virtual void SetBreakpointEnabled(const FGuid& NodeGuid, const FName& PinName, bool bEnabled);

	virtual bool IsBreakpointEnabled(const FGuid& NodeGuid);
	virtual bool IsBreakpointEnabled(const FGuid& NodeGuid, const FName& PinName);

protected:
	virtual void MarkAsHit(const FGuid& NodeGuid);
	virtual void MarkAsHit(const FGuid& NodeGuid, const FName& PinName);
	
	virtual void PauseSession();
	virtual void ResumeSession();
	void SetPause(const bool bPause);

	virtual void ClearHitBreakpoints();

public:
	virtual bool IsBreakpointHit(const FGuid& NodeGuid);
	virtual bool IsBreakpointHit(const FGuid& NodeGuid, const FName& PinName);

private:
	/** Saves any modifications made to breakpoints */
	virtual void SaveSettings();
};
