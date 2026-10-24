// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Nodes/Developer/FlowNode_Log.h"

#include "FlowAssetLifecycleTestTypes.generated.h"

UCLASS(NotPlaceable)
class UFlowNode_LogTestable : public UFlowNode_Log
{
	GENERATED_BODY()

public:
	void DeactivateForTest()
	{
		Deactivate();
	}

	virtual void Cleanup() override
	{
		++CleanupCount;
		Super::Cleanup();
	}

	int32 CleanupCount = 0;
};
