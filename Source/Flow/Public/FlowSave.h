// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "FlowSave.generated.h"

USTRUCT(BlueprintType)
struct FLOW_API FFlowNodeSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Flow")
	FGuid NodeGuid;

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Flow")
	TArray<uint8> NodeData;

	friend FArchive& operator<<(FArchive& Ar, FFlowNodeSaveData& InNodeData)
	{
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct FLOW_API FFlowAssetSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Flow")
	FString WorldName;

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Flow")
	FString InstanceName;

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Flow")
	TArray<uint8> AssetData;

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Flow")
	TArray<FFlowNodeSaveData> NodeRecords;

	friend FArchive& operator<<(FArchive& Ar, FFlowAssetSaveData& InAssetData)
	{
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct FLOW_API FFlowComponentSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Flow")
	FString WorldName;

	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Flow")
	FString ActorInstanceName;

	UPROPERTY(SaveGame)
	TArray<uint8> ComponentData;

	friend FArchive& operator<<(FArchive& Ar, FFlowComponentSaveData& InComponentData)
	{
		return Ar;
	}
};

struct FLOW_API FFlowArchive : public FObjectAndNameAsStringProxyArchive
{
	FFlowArchive(FArchive& InInnerArchive) : FObjectAndNameAsStringProxyArchive(InInnerArchive, true)
	{
		ArIsSaveGame = true;
	}
};

UCLASS(BlueprintType)
class FLOW_API UFlowSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UFlowSaveGame() {};

	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	FString SaveSlotName = TEXT("FlowSave");

	UPROPERTY(VisibleAnywhere, Category = "Flow")
	TArray<FFlowComponentSaveData> FlowComponents;

	UPROPERTY(VisibleAnywhere, Category = "Flow")
	TArray<FFlowAssetSaveData> FlowInstances;
	
	friend FArchive& operator<<(FArchive& Ar, UFlowSaveGame& SaveGame)
	{
		Ar << SaveGame.FlowComponents;
		Ar << SaveGame.FlowInstances;
		return Ar;
	}
};
