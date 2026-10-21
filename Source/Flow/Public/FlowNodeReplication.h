// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Net/Serialization/FastArraySerializer.h"

#include "FlowNodeReplication.generated.h"

class UFlowAsset;
class UFlowNodeBase;
struct FFlowNodeReplicatedStateArray;

/**
 * Replicated runtime state of a single Flow Node and its AddOns, identified by NodeGuid within a Flow Asset instance.
 */
USTRUCT()
struct FLOW_API FFlowNodeReplicatedState : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid NodeGuid;

	/* Properties marked as UPROPERTY(Replicated) of the node and its AddOns, serialized to bytes. */
	UPROPERTY()
	TArray<uint8> CustomData;

	// FFastArraySerializerItem
	void PostReplicatedAdd(const FFlowNodeReplicatedStateArray& Serializer) const;
	void PostReplicatedChange(const FFlowNodeReplicatedStateArray& Serializer) const;
	// --
};

/**
 * Replicated states of Flow Nodes belonging to a single Flow Asset instance.
 */
USTRUCT()
struct FLOW_API FFlowNodeReplicatedStateArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FFlowNodeReplicatedState> Items;

	/* Flow Asset instance holding this array. */
	TWeakObjectPtr<UFlowAsset> OwningAsset;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FFlowNodeReplicatedState, FFlowNodeReplicatedStateArray>(Items, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FFlowNodeReplicatedStateArray> : public TStructOpsTypeTraitsBase2<FFlowNodeReplicatedStateArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/**
 * Serialization of properties marked as UPROPERTY(Replicated) on a Flow Node and its AddOns.
 * Supports value types: numbers, enums, names, strings, texts, structs, gameplay tags.
 * Object references resolve by path, so assets and actors placed on the map work, actors spawned at runtime don't.
 */
namespace FlowNodeReplication
{
	/* Returns true, if the class of the node or any of its AddOns has a property marked as UPROPERTY(Replicated).
	 * Properties declared in UFlowNode don't count, since every node has them. */
	FLOW_API bool HasReplicatedProperties(const UFlowNodeBase& Node);

	FLOW_API void SerializeReplicatedProperties(UFlowNodeBase& Node, FArchive& Ar);
	FLOW_API void WriteReplicatedProperties(UFlowNodeBase& Node, TArray<uint8>& OutData);

	/* Reads properties of the node and its AddOns.
	 * Then calls RepNotify functions of properties marked as UPROPERTY(ReplicatedUsing), which value changed. */
	FLOW_API void ReadReplicatedProperties(UFlowNodeBase& Node, const TArray<uint8>& Data);
}
