#pragma once

#include "CoreMinimal.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "FlowSave.generated.h"

USTRUCT(BlueprintType)
struct FLOW_API FFlowNodeSaveData
{
	GENERATED_USTRUCT_BODY()

	FGuid NodeGuid;
	TArray<uint8> NodeData;

	friend FArchive& operator<<(FArchive& Ar, FFlowNodeSaveData& InNodeData)
	{
		Ar << InNodeData.NodeGuid;
		Ar << InNodeData.NodeData;
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct FLOW_API FFlowAssetSaveData
{
	GENERATED_USTRUCT_BODY()

	FString AssetPath;
	FString InstanceName;
	TArray<uint8> AssetData;
	
	TArray<FFlowNodeSaveData> NodeRecords;
	TArray<FFlowAssetSaveData> FlowRecords;

	friend FArchive& operator<<(FArchive& Ar, FFlowAssetSaveData& InAssetData)
	{
		Ar << InAssetData.AssetPath;
		Ar << InAssetData.InstanceName;
		Ar << InAssetData.AssetData;
		Ar << InAssetData.NodeRecords;
		Ar << InAssetData.FlowRecords;
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct FLOW_API FFlowComponentSaveData
{
	GENERATED_USTRUCT_BODY()

	FString WorldName;
	FString ActorInstanceName;

	TArray<uint8> ComponentData;

	friend FArchive& operator<<(FArchive& Ar, FFlowComponentSaveData& InComponentData)
	{
		Ar << InComponentData.WorldName;
		Ar << InComponentData.ActorInstanceName;
		Ar << InComponentData.ComponentData;
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

// it's just an example
USTRUCT(BlueprintType)
struct FLOW_API FFlowSaveData
{
	GENERATED_USTRUCT_BODY()

	FDateTime Timestamp;
	TArray<FFlowComponentSaveData> SavedFlowComponents;
	TArray<FFlowAssetSaveData> SavedRootFlowInstances;

	friend FArchive& operator<<(FArchive& Ar, FFlowSaveData& FlowData)
	{
		Ar << FlowData.Timestamp;
		Ar << FlowData.SavedFlowComponents;
		Ar << FlowData.SavedRootFlowInstances;
		return Ar;
	}

	bool IsValid() const
	{
		return SavedRootFlowInstances.Num() > 0;
	}
};
