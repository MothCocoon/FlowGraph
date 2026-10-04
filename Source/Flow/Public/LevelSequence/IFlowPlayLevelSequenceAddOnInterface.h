// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Interface.h"

#include "IFlowPlayLevelSequenceAddOnInterface.generated.h"

class AActor;
class AFlowLevelSequenceActor;
class UFlowNodeAddOn;

/**
 * Interface for add-ons that want to apply setup (e.g. actor binding overrides) to a
 * level sequence actor immediately after the sequence player is created, before Play() is called.
 *
 * Attach to any flow node that spawns an AFlowLevelSequenceActor and supports this interface.
 */
UINTERFACE(MinimalAPI)
class UFlowPlayLevelSequenceAddOnInterface : public UInterface
{
	GENERATED_BODY()
};

class FLOW_API IFlowPlayLevelSequenceAddOnInterface
{
	GENERATED_BODY()

public:

	static bool ImplementsInterfaceSafe(const UFlowNodeAddOn* AddOnTemplate);

	/**
	 * Called after the sequence player is created, before Play() is invoked.
	 *
	 * @param SequenceActor  The spawned AFlowLevelSequenceActor. Never null when called.
	 * @param FlowOwner      The actor that owns the flow graph. May be null.
	 */
	virtual void OnSequencePlayerCreated(AFlowLevelSequenceActor& SequenceActor, AActor* FlowOwner) {}
};
