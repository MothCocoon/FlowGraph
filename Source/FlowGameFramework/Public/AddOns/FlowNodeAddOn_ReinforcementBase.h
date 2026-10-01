// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "ActorSpawning/FlowFightActorsTypes.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Engine/TimerHandle.h"

#include "FlowNodeAddOn_ReinforcementBase.generated.h"

class IFlowFightActorsReinforcementHost;

/** Shared authored fight reinforcement and timer, round, threshold, and state lifecycle. */
UCLASS(Abstract, EditInlineNew, Blueprintable, DisplayName = "Fight Reinforcement Base")
class FLOWGAMEFRAMEWORK_API UFlowNodeAddOn_ReinforcementBase : public UFlowNodeAddOn
{
	GENERATED_BODY()

protected:
	/** Shared authored timing and pass policy for Flow and project-specific reinforcement addons. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Configuration,
		DisplayName = "Reinforcement Parameters", meta = (ShowOnlyInnerProperties))
	FFlowFightReinforcementParameters Params;

	/** Activation-local reinforcement state; Invalid before activation and after cleanup. */
	UPROPERTY(Transient)
	EFlowFightReinforcementState ReinforcementState = EFlowFightReinforcementState::Invalid;

	/** Repeating opportunity timer while timer-driven auto-reinforcement is enabled and idle. */
	UPROPERTY(Transient)
	FTimerHandle AutoReinforcementTimerHandle;

	/** One-shot delay between a reinforcement request and its spawning pass. */
	UPROPERTY(Transient)
	FTimerHandle ScheduledReinforcementTimerHandle;

	/** Runtime enablement, which may differ from the authored initial value in Params. */
	UPROPERTY(Transient)
	bool bAutoReinforcementEnabled = false;

	/** Number of productive rounds completed during this activation. */
	UPROPERTY(Transient)
	uint16 CurrentReinforcementRound = 0;

public:
	UFlowNodeAddOn_ReinforcementBase();
	virtual void OnActivate() override;
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
	/** The authored Flow parameters used by vanilla and project-specific reinforcement addons. */
	const FFlowFightReinforcementParameters& GetParameters() const { return Params; }

#if WITH_SERVER_CODE
	bool IsReinforcementScheduledOrInProgress() const;
	bool IsExhausted() const;
	bool ShouldAutoReinforceOnActorDefeated(float AliveUnitPercent) const;
	void RequestReinforcement();
	void AbortReinforcement(bool bDisableAutoReinforcement);
#endif

	static const FName INPIN_RequestReinforce;
	static const FName INPIN_EnableAutoReinforce;
	static const FName INPIN_DisableAutoReinforce;

protected:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

#if WITH_SERVER_CODE
	IFlowFightActorsReinforcementHost* GetFightHost() const;
	void SetReinforcementState(EFlowFightReinforcementState NextState);
	void UpdateReinforcementState();
	void ResetReinforcementState();
	void ReconcileAutoReinforcementTimer();
	void ManageScheduledReinforcementTimer(EFlowFightReinforcementState PreviousState,
		EFlowFightReinforcementState NextState);
	bool ShouldHaveAutoReinforcementTimer() const;
	void StartReinforcement();
#endif

	UFUNCTION()
	void OnAutoReinforcementTimerExpired();

	UFUNCTION()
	void OnScheduledReinforcementTimerExpired();
};
