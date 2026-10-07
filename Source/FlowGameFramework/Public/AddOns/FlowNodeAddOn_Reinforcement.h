// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "AddOns/FlowNodeAddOn_ReinforcementBase.h"

#include "FlowNodeAddOn_Reinforcement.generated.h"

/** Authored Flow fight reinforcement; project subclasses can add lifecycle policy. */
UCLASS(EditInlineNew, Blueprintable, DisplayName = "Fight Reinforcement")
class FLOWGAMEFRAMEWORK_API UFlowNodeAddOn_Reinforcement : public UFlowNodeAddOn_ReinforcementBase
{
	GENERATED_BODY()

public:
	UFlowNodeAddOn_Reinforcement();
};
