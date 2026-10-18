// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Subsystems/EngineSubsystem.h"

#include "Debugger/FlowDebuggerTypes.h"
#include "Interfaces/FlowExecutionGate.h"
#include "Types/FlowEnumUtils.h"

#include "FlowDebuggerSubsystem.generated.h"

class UEdGraphNode;

class UFlowAsset;
class UFlowNode;

UENUM()
enum class EFlowDebuggerState
{
	// Initialized, running, but never halted
	InitialRunning,

	// Running after being pausing
	Resumed,
	
	// Currently paused at a breakpoint
	Paused,

	Max UMETA(Hidden),
	Invalid = -1 UMETA(Hidden),
	Min = 0 UMETA(Hidden),

	// Subranges for classifier checks
	PausedGameFirst = Paused UMETA(Hidden),
	PausedGameLast = Paused UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowDebuggerState);

namespace EFlowDebuggerState_Classifiers
{
	FORCEINLINE bool IsPausedGameState(EFlowDebuggerState State) { return FLOW_IS_ENUM_IN_SUBRANGE(State, EFlowDebuggerState::PausedGame); }
}

DECLARE_MULTICAST_DELEGATE_OneParam(FFlowAssetDebuggerEvent, const UFlowAsset& /*FlowAsset*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FFlowAssetDebuggerBreakpointHitEvent, const UFlowNode* /*FlowNode*/);

/**
* Persistent subsystem supporting Flow Graph debugging.
* It might be utilized to use cook-specific graph debugger.
*/
UCLASS()
class FLOWDEBUGGER_API UFlowDebuggerSubsystem : public UEngineSubsystem, public IFlowExecutionGate
{
	GENERATED_BODY()

public:
	UFlowDebuggerSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
	virtual void OnInstancedTemplateAdded(UFlowAsset* AssetTemplate);
	virtual void OnInstancedTemplateRemoved(UFlowAsset* AssetTemplate);

public:
	//~ Begin IFlowExecutionGate
	virtual bool WantsFlowBreak(const FFlowBreakContext& Context) const override;
	virtual EFlowBreakAction BreakFlowExecution(const FFlowBreakContext& Context) override;
	//~ End IFlowExecutionGate

	/* True while execution is halted at a breakpoint. */
	bool IsPausedAtBreakpoint() const { return EFlowDebuggerState_Classifiers::IsPausedGameState(FlowDebuggerState); }

	virtual void AddBreakpoint(const FGuid& NodeGuid);
	virtual void AddBreakpoint(const FGuid& NodeGuid, const FName& PinName);

	virtual void RemoveAllBreakpoints(const TWeakObjectPtr<UFlowAsset> FlowAsset);
	virtual void RemoveAllBreakpoints(const FGuid& NodeGuid);
	virtual void RemoveNodeBreakpoint(const FGuid& NodeGuid);
	virtual void RemovePinBreakpoint(const FGuid& NodeGuid, const FName& PinName);

#if WITH_EDITOR
	/* Removes obsolete pin breakpoints for provided. Pin list can be changed during node reconstruction. */
	virtual void RemoveObsoletePinBreakpoints(const UEdGraphNode* Node);
#endif

	virtual void ToggleBreakpoint(const FGuid& NodeGuid);
	virtual void ToggleBreakpoint(const FGuid& NodeGuid, const FName& PinName);

	virtual FFlowBreakpoint* FindBreakpoint(const FGuid& NodeGuid);
	virtual FFlowBreakpoint* FindBreakpoint(const FGuid& NodeGuid, const FName& PinName);
	static const FFlowBreakpoint* FindBreakpointForRead(const FGuid& NodeGuid);
	static const FFlowBreakpoint* FindBreakpointForRead(const FGuid& NodeGuid, const FName& PinName);
	static bool HasAnyBreakpoints(const TWeakObjectPtr<UFlowAsset> FlowAsset);

	virtual void SetBreakpointEnabled(const FGuid& NodeGuid, bool bEnabled);
	virtual void SetBreakpointEnabled(const FGuid& NodeGuid, const FName& PinName, bool bEnabled);
	virtual void SetAllBreakpointsEnabled(const TWeakObjectPtr<UFlowAsset> FlowAsset, bool bEnabled);

	virtual bool IsBreakpointEnabled(const FGuid& NodeGuid);
	virtual bool IsBreakpointEnabled(const FGuid& NodeGuid, const FName& PinName);
	static bool HasAnyBreakpointsEnabled(const TWeakObjectPtr<UFlowAsset>& FlowAsset);
	static bool HasAnyBreakpointsDisabled(const TWeakObjectPtr<UFlowAsset>& FlowAsset);
	static bool HasAnyBreakpointsMatching(const TWeakObjectPtr<UFlowAsset>& FlowAsset, bool bDesiresEnabled);

protected:
	virtual void MarkAsHit(const UFlowNode* FlowNode);
	virtual void MarkAsHit(const UFlowNode* FlowNode, const FName& PinName);

	/* Blocks until the user releases execution, keeping the halted Flow call stack alive.
	 * The base implementation cannot halt without an application to pump, so it resumes immediately;
	 * the editor implementation runs a nested tick loop. */
	virtual EFlowBreakAction HaltUntilReleased(const FFlowBreakContext& Context) { return EFlowBreakAction::Continue; }

	virtual void PauseSession(UFlowAsset& FlowAssetInstance);
	virtual void ResumeSession(UFlowAsset& FlowAssetInstance);
	virtual void StopSession();

	/* Clears the "currently hit" breakpoint only (node or pin).
	 * This avoids races where blanket-clearing all hit flags can erase a newly-hit breakpoint during resume/flush. */
	void ClearLastHitBreakpoint();

	/* Clears hit state for all breakpoints. Prefer ClearLastHitBreakpoint() for resume/step logic. */
	virtual void ClearHitBreakpoints();

private:
	void SetFlowDebuggerState(EFlowDebuggerState NextState, UFlowAsset* FlowAssetInstance);
	void ManageGameModePaused(EFlowDebuggerState PrevState, EFlowDebuggerState NextState, UFlowAsset* FlowAssetInstance);

protected:
	virtual void OnFlowDebuggerStateChanged(EFlowDebuggerState PrevState, EFlowDebuggerState NextState, UFlowAsset* FlowAssetInstance) {}

public:
	virtual bool IsBreakpointHit(const FGuid& NodeGuid);
	virtual bool IsBreakpointHit(const FGuid& NodeGuid, const FName& PinName);

	// Delegates for debugger events (broadcast when pausing, resuming, or hitting breakpoints)
	FFlowAssetDebuggerEvent OnDebuggerPaused;
	FFlowAssetDebuggerEvent OnDebuggerResumed;
	FFlowAssetDebuggerBreakpointHitEvent OnDebuggerBreakpointHit;
	FFlowAssetDebuggerEvent OnDebuggerFlowAssetTemplateRemoved;

protected:
	EFlowDebuggerState FlowDebuggerState = EFlowDebuggerState::Invalid;

	// Track the single breakpoint location that is currently "hit" (node or pin).
	FGuid LastHitNodeGuid;
	FName LastHitPinName;

	/* Saves any modifications made to breakpoints. */
	virtual void SaveSettings();
};