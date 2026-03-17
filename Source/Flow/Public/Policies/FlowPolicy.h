// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "UObject/Class.h"

#include "FlowPolicy.generated.h"

// Flow Policy base-class, for policy structs to inherit from
USTRUCT()
struct FFlowPolicy
{
	GENERATED_BODY()

public:

	virtual ~FFlowPolicy() = default;

	// Nothing of interest here, yet, but defining a class for it, just-in-case
};
