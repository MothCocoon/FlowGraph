#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "FlowNode.generated.h"

class UFlowAsset;

#if WITH_EDITOR
class UEdGraphNode;
#endif

USTRUCT()
struct FLOW_API FFlowPin
{
	GENERATED_USTRUCT_BODY()
	
public:
	UPROPERTY()
	FGuid NodeGuid;

	UPROPERTY()
	uint8 PinIndex;

	FFlowPin() {};

	FFlowPin(const FGuid InNodeId, const uint8 InPinIndex)
	{
		NodeGuid = InNodeId;
		PinIndex = InPinIndex;
	}
};

/**
 * Base for all Flow Nodes
 */
UCLASS(Abstract, hideCategories = Object)
class FLOW_API UFlowNode : public UObject
{
	GENERATED_UCLASS_BODY()

	friend class UFlowAsset;

//////////////////////////////////////////////////////////////////////////
// Graph

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	UEdGraphNode* GraphNode;

protected:
	UPROPERTY()
	FString Category;
#endif

#if WITH_EDITOR
public:
	void SetGraphNode(UEdGraphNode* NewGraph);
	UEdGraphNode* GetGraphNode() const { return GraphNode; };

	FString GetCategory() const { return Category; };
	virtual bool HasInputPins() const { return true; };
	virtual bool HasOutputPins() const { return true; };

	virtual FText GetTitle() const { return GetClass()->GetDisplayNameText(); }
	virtual FName GetInputPinName(int32 PinIndex) const { return TEXT("In"); }
	virtual FName GetOutputPinName(int32 PinIndex) const { return TEXT("Out"); }
#endif

//////////////////////////////////////////////////////////////////////////
// Node

private:
	// Inherits Guid after graph node
	UPROPERTY()
	FGuid NodeGuid;

	// Map outputs to the connected node and input pin
 	UPROPERTY()
	TMap<uint8, FFlowPin> Connections;

public:
	UFlowAsset* GetFlowAsset() const;

	void SetGuid(const FGuid NewGuid) { NodeGuid = NewGuid; };
	FGuid GetGuid() const { return NodeGuid; };

	void SetConnections(TMap<uint8, FFlowPin>& InConnections) { Connections = InConnections; };
	FFlowPin GetConnection(const uint8 OutputIndex) const { return Connections.FindRef(OutputIndex); };

//////////////////////////////////////////////////////////////////////////
// Runtime

private:
	FStreamableManager Streamable;
	TMap<uint8, TArray<double>> InputRecords;
	TMap<uint8, TArray<double>> OutputRecords;

protected:
	virtual void ExecuteInput(const uint8 Pin);
	virtual void ExecuteOutput(const uint8 Pin);
	void ActivateOutput(const uint8 Pin);
	void Finish();
	virtual void Cleanup() {};

private:
	void RecordInput(const uint8 PinIndex);
	void RecordOutput(const uint8 PinIndex);
	void ResetRecords();

protected:
	template<typename T>
	T* LoadAsset(TSoftObjectPtr<UObject> AssetPtr)
	{
		ensure(!AssetPtr.IsNull());

		if (AssetPtr.IsPending())
		{
			const FSoftObjectPath& AssetRef = AssetPtr.ToSoftObjectPath();
			AssetPtr = Cast<UFlowAsset>(Streamable.LoadSynchronous(AssetRef, false));
		}

		return AssetPtr.Get();
	}

#if WITH_EDITOR
public:
	TMap<uint8, double> GetWireRecords() const;

	UFlowNode* GetInspectedInstance();
	virtual UObject* GetAssetToOpen() { return nullptr; };
	virtual AActor* GetActorToFocus() { return nullptr; };
#endif
};
