// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "UObject/Interface.h"

#include "FlowSaveDataContainerInterface.generated.h"

struct FFlowSaveData;

// (optional) interface to allow for save data containers that are not USaveGame
UINTERFACE(MinimalAPI, BlueprintType)
class UFlowSaveDataContainerInterface : public UInterface
{
	GENERATED_BODY()
};

class IFlowSaveDataContainerInterface
{
	GENERATED_BODY()

public:
	/** Get flow save data for modification. */
	virtual FFlowSaveData& GetSaveDataMutable() = 0;
	
	/** Get flow save data for reading. */
	virtual const FFlowSaveData& GetSaveData() const = 0;
};
