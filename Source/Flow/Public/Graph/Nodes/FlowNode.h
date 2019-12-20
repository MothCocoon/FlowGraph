#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/Class.h"

#include "Graph/FlowAssetTypes.h"
#include "FlowNode.generated.h"

class UFlowAsset;
class UFlowSubsystem;

#if WITH_EDITOR
class UEdGraphNode;
#endif

USTRUCT()
struct FLOW_API FConnectedPin
{
	GENERATED_USTRUCT_BODY()
	
public:
	FConnectedPin() {};

	FConnectedPin(const FGuid InNodeId, const uint8 InPinIndex, const FName& InPinName)
	{
		NodeGuid = InNodeId;
		PinName = InPinName;
	}

	UPROPERTY()
	FGuid NodeGuid;

	UPROPERTY()
	FName PinName;
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
// Node

#if WITH_EDITORONLY_DATA
public:
	FString Category;
	EFlowNodeStyle NodeStyle;

private:
	UPROPERTY()
	UEdGraphNode* GraphNode;
#endif

#if WITH_EDITOR
public:
	void SetGraphNode(UEdGraphNode* NewGraph);
	UEdGraphNode* GetGraphNode() const { return GraphNode; };

	FString GetCategory() const { return Category; };
	virtual FText GetTitle() const { return GetClass()->GetDisplayNameText(); }

	// short summary of node's content - displayed over node as NodeInfoPopup
	virtual FString GetShortDescription() const { return FString(); };
#endif

protected:
	// inherits Guid after graph node
	UPROPERTY()
	FGuid NodeGuid;

public:
	void SetGuid(const FGuid NewGuid) { NodeGuid = NewGuid; };
	FGuid GetGuid() const { return NodeGuid; };

	UFlowAsset* GetFlowAsset() const;

//////////////////////////////////////////////////////////////////////////
// Every node has different pin setup

public:
	// names of node-specific inputs
	UPROPERTY()
	TArray<FName> InputNames;

	// names of node-specific outputs
	UPROPERTY()
	TArray<FName> OutputNames;

	virtual bool CanUserAddInput() const { return false; };
	virtual bool CanUserAddOutput() const { return false; };

	bool HasUserCreatedInputs() const;
	bool HasUserCreatedOutputs() const;

protected:
	// always use default range for nodes with user-created outputs i.e. Execution Sequence
	void SetNumericalOutputs(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);

//////////////////////////////////////////////////////////////////////////
// All created pins (default, node-specific and added by user)

protected:
	UPROPERTY()
	TMap<FName, uint8> CreatedInputs;

	UPROPERTY()
	TMap<FName, uint8> CreatedOutputs;

public:
	void AddCreatedInput(const uint8 PinIndex, const FName& PinName);
	void AddCreatedOutput(const uint8 PinIndex, const FName& PinName);

	void RemoveCreatedInput(const FName& PinName);
	void RemoveCreatedOutput(const FName& PinName);

	FName GetInputName(const uint8 PinIndex) const;
	FName GetOutputName(const uint8 PinIndex) const;

	bool HasInputs() const { return CreatedInputs.Num() > 0; };
	bool HasOutputs() const { return CreatedOutputs.Num() > 0; };

//////////////////////////////////////////////////////////////////////////
// Connections to other nodes

private:
	// map outputs to the connected node and input pin
	UPROPERTY()
	TMap<FName, FConnectedPin> Connections;

public:
	void SetConnections(TMap<FName, FConnectedPin>& InConnections) { Connections = InConnections; };
	FConnectedPin GetConnection(const FName OutputName) const { return Connections.FindRef(OutputName); };
	TSet<UFlowNode*> GetConnectedNodes() const;

//////////////////////////////////////////////////////////////////////////
// Runtime

protected:
	FStreamableManager Streamable;

private:
	TMap<FName, TArray<double>> InputRecords;
	TMap<FName, TArray<double>> OutputRecords;

public:
	UFlowSubsystem* GetFlowSubsystem() const;
	virtual void PreloadContent() {};

protected:
	// trigger execution of input pin
	void TriggerInput(const FName& PinName);

	// method implementing node-specific logic
	virtual void ExecuteInput(const FName& PinName);

	// if node has only one output, we can finish node in one simple call
	void TriggerDefaultOutput(const bool bFinish);

	// trigger output pin
	void TriggerOutput(const FName& PinName, const bool bFinish = false);
	FORCEINLINE void TriggerOutput(const uint8 PinName, const bool bFinish = false);
	FORCEINLINE void TriggerOutput(const int32 PinName, const bool bFinish = false);

	// finish execution of node, it will call Cleanup()
	void Finish();

	// method implementing UObject's cleanup after node finished the work
	virtual void Cleanup() {};

public:
	// use it to finish node from the outside, i.e. automation tests
	void ForceFinishNode();

protected:
	// define what happens when node is finished from outside
	virtual void OnForceFinished() {};

private:
	void ResetRecords();

protected:
	template<typename T>
	T* LoadAsset(TSoftObjectPtr<UObject> AssetPtr)
	{
		ensure(!AssetPtr.IsNull());

		if (AssetPtr.IsPending())
		{
			const FSoftObjectPath& AssetRef = AssetPtr.ToSoftObjectPath();
			AssetPtr = Cast<T>(Streamable.LoadSynchronous(AssetRef, false));
		}

		return Cast<T>(AssetPtr.Get());
	}

#if WITH_EDITOR
public:
	TMap<uint8, double> GetWireRecords() const;

	UFlowNode* GetInspectedInstance();

	// information displayed while node is working - displayed over node as NodeInfoPopup
	virtual FString GetStatus() const { return FString(); };

	virtual UObject* GetAssetToOpen() { return nullptr; };
	virtual AActor* GetActorToFocus() { return nullptr; };
#endif
};
