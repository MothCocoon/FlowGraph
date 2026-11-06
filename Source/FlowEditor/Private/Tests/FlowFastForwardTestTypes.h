// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/FlowNode.h"

#include "FlowFastForwardTestTypes.generated.h"

UCLASS(Hidden, NotBlueprintable, NotPlaceable, meta = (ExcludeFromFlowCatalog))
class UFlowFastForwardTestNode : public UFlowNode
{
	GENERATED_BODY()

public:
	void AddTestOutputPin(const FName& PinName) { OutputPins.Add(FFlowPin(PinName)); }
	void SetTestSignalMode(const EFlowSignalMode InSignalMode) { SignalMode = InSignalMode; }

	/* Stays active, like a node waiting for gameplay. */
	virtual void ExecuteInput(const FName& PinName) override { ++ExecuteInputCount; }

protected:
	virtual void FastForwardInput(const FName& PinName, const FName& OutputPinName) override
	{
		++FastForwardInputCount;
		Super::FastForwardInput(PinName, OutputPinName);
	}

public:
	int32 ExecuteInputCount = 0;
	int32 FastForwardInputCount = 0;
};

/**
 * Finishes by another output than the one expected by Fast Forward.
 */
UCLASS(Hidden, NotBlueprintable, NotPlaceable, meta = (ExcludeFromFlowCatalog))
class UFlowFastForwardWrongOutputTestNode : public UFlowNode
{
	GENERATED_BODY()

public:
	UFlowFastForwardWrongOutputTestNode()
	{
		OutputPins.Add(FFlowPin(TEXT("Alt")));
	}

protected:
	virtual void FastForwardInput(const FName& PinName, const FName& OutputPinName) override
	{
		TriggerOutput(TEXT("Alt"), true);
	}
};

/**
 * Waits for something that never happens while fast forwarding.
 */
UCLASS(Hidden, NotBlueprintable, NotPlaceable, meta = (ExcludeFromFlowCatalog))
class UFlowFastForwardWaitingTestNode : public UFlowNode
{
	GENERATED_BODY()

protected:
	virtual void FastForwardInput(const FName& PinName, const FName& OutputPinName) override {}
};
