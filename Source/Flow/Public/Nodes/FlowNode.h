#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "VisualLoggerDebugSnapshotInterface.h"

#include "FlowTypes.h"
#include "FlowNode.generated.h"

class UEdGraphNode;

class UFlowAsset;
class UFlowSubsystem;

USTRUCT()
struct FLOW_API FConnectedPin
{
	GENERATED_USTRUCT_BODY()
	
public:
	UPROPERTY()
	FGuid NodeGuid;

	UPROPERTY()
	FName PinName;

	FConnectedPin() {};

	FConnectedPin(const FGuid InNodeId, const uint8 InPinIndex, const FName& InPinName)
	{
		NodeGuid = InNodeId;
		PinName = InPinName;
	}
};

#if !UE_BUILD_SHIPPING
struct FLOW_API FPinRecord
{
public:
	double Time;
	FString HumanReadableTime;

	FPinRecord();
	FPinRecord(const double InTime);

private:
	FORCEINLINE const FString DoubleDigit(const int32 Number) const;
};
#endif

/**
 * Base for all Flow Nodes
 */
UCLASS(Abstract, hideCategories = Object)
class FLOW_API UFlowNode : public UObject, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_UCLASS_BODY()

	friend class UFlowAsset;

//////////////////////////////////////////////////////////////////////////
// Node

#if WITH_EDITORONLY_DATA
public:
	FString Category;
	EFlowNodeStyle NodeStyle;
#endif

private:
	UPROPERTY()
	UEdGraphNode* GraphNode;

public:
	UEdGraphNode* GetGraphNode() const { return GraphNode; };

#if WITH_EDITOR
	void SetGraphNode(UEdGraphNode* NewGraph);

	FString GetCategory() const { return Category; };
	virtual FText GetTitle() const { return GetClass()->GetDisplayNameText(); }

	// short summary of node's content - displayed over node as NodeInfoPopup
	virtual FString GetNodeDescription() const { return FString(); };
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
// All created pins (default, class-specific and added by user)

public:
	static FName DefaultInputName;
	static FName DefaultOutputName;

	// class-specific and user-added inputs
	UPROPERTY()
	TArray<FName> InputNames;

	// class-specific and user-added outputs
	UPROPERTY()
	TArray<FName> OutputNames;

#if WITH_EDITOR
	virtual bool CanUserAddInput() const { return false; };
	virtual bool CanUserAddOutput() const { return false; };

	void RemoveUserInput();
	void RemoveUserOutput();
#endif

protected:
	// always use default range for nodes with user-created outputs i.e. Execution Sequence
	void SetNumericalOutputs(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);

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

public:
	bool bPreloaded;

protected:
	FStreamableManager Streamable;

#if !UE_BUILD_SHIPPING
private:
	TMap<FName, TArray<FPinRecord>> InputRecords;
	TMap<FName, TArray<FPinRecord>> OutputRecords;
	EFlowActivationState ActivationState;
#endif

public:
	UFlowSubsystem* GetFlowSubsystem() const;
	virtual UWorld* GetWorld() const override;

	void TriggerPreload();
	void TriggerFlush();

protected:
	virtual void PreloadContent() {};
	virtual void FlushContent() {};

	// trigger execution of input pin
	void TriggerInput(const FName& PinName);

	// method implementing node-specific logic
	virtual void ExecuteInput(const FName& PinName);

	// if node has only one output, we can finish node in one simple call
	void TriggerFirstOutput(const bool bFinish);

	// trigger output pin
	void TriggerOutput(const FName& PinName, const bool bFinish = false);

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

#if !UE_BUILD_SHIPPING
private:
	void ResetRecords();
#endif

#if WITH_EDITOR
public:
	UFlowNode* GetInspectedInstance() const;
	EFlowActivationState GetActivationState() const { return ActivationState; };

	TMap<uint8, FPinRecord> GetWireRecords() const;
	TArray<FPinRecord> GetInputRecords(const FName& PinName) const;
	TArray<FPinRecord> GetOutputRecords(const FName& PinName) const;

	// information displayed while node is working - displayed over node as NodeInfoPopup
	virtual FString GetStatusString() const { return FString(); };

	virtual UObject* GetAssetToOpen() { return nullptr; };
	virtual AActor* GetActorToFocus() { return nullptr; };
#endif

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
};