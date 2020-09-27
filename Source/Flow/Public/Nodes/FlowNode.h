#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "FlowTypes.h"
#include "FlowNode.generated.h"

class UEdGraphNode;

class UFlowAsset;
class UFlowSubsystem;

USTRUCT()
struct FLOW_API FConnectedPin
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGuid NodeGuid;

	UPROPERTY()
	FName PinName;

	FConnectedPin()
	{
		NodeGuid = FGuid();
		PinName = NAME_None;
	}

	FConnectedPin(const FGuid InNodeId, const FName& InPinName)
	{
		NodeGuid = InNodeId;
		PinName = InPinName;
	}
};

#if !UE_BUILD_SHIPPING
struct FLOW_API FPinRecord
{
	double Time;
	FString HumanReadableTime;

	FPinRecord();
	FPinRecord(const double InTime);

private:
	FORCEINLINE static FString DoubleDigit(const int32 Number);
};
#endif

/**
 * Base for all Flow Nodes
 */
UCLASS(Abstract, Blueprintable, HideCategories = Object)
class FLOW_API UFlowNode : public UObject, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_UCLASS_BODY()

	friend class UFlowAsset;
	friend class UFlowGraphNode;

//////////////////////////////////////////////////////////////////////////
// Node

private:
	UPROPERTY()
	UEdGraphNode* GraphNode;
	
#if WITH_EDITORONLY_DATA
protected:
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	FString Category;

	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	EFlowNodeStyle NodeStyle;
#endif

public:
	UEdGraphNode* GetGraphNode() const { return GraphNode; }
	
#if WITH_EDITOR
	void SetGraphNode(UEdGraphNode* NewGraph);

	FString GetCategory() const { return Category; }
	EFlowNodeStyle GetNodeStyle() const { return NodeStyle; }

	virtual FText GetTitle() const { return GetClass()->GetDisplayNameText(); }

	// short summary of node's content - displayed over node as NodeInfoPopup
	virtual FString GetNodeDescription() const;
#endif

protected:
	// short summary of node's content - displayed over node as NodeInfoPopup
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "GetNodeDescription"))
	FString K2_GetNodeDescription() const;

	// inherits Guid after graph node
	UPROPERTY()
	FGuid NodeGuid;

public:
	void SetGuid(const FGuid NewGuid) { NodeGuid = NewGuid; }
	FGuid GetGuid() const { return NodeGuid; }

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	UFlowAsset* GetFlowAsset() const;

//////////////////////////////////////////////////////////////////////////
// All created pins (default, class-specific and added by user)

public:
	static FName DefaultInputName;
	static FName DefaultOutputName;

protected:
	// class-specific and user-added inputs
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<FName> InputNames;

	// class-specific and user-added outputs
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<FName> OutputNames;

public:
	TArray<FName> GetInputNames() const { return InputNames; }
	TArray<FName> GetOutputNames() const { return OutputNames; }
	
#if WITH_EDITOR
	virtual bool SupportsContextPins() const { return false; }

	/** Enabling it might cause loading gigabytes of data as nodes would load all related data (i.e. Level Sequences)
	/** This refresh needs to be enabled by UFlowEditorSettings::bRefreshContextPinsOnLoad */
	virtual bool CanRefreshContextPinsOnLoad() const { return false; }

	virtual TArray<FName> GetContextInputs() { return TArray<FName>(); }
	virtual TArray<FName> GetContextOutputs() { return TArray<FName>(); }

	virtual bool CanUserAddInput() const { return false; }
	virtual bool CanUserAddOutput() const { return false; }

	void RemoveUserInput();
	void RemoveUserOutput();
#endif

protected:
	// always use default range for nodes with user-created outputs i.e. Execution Sequence
	void SetNumericalInputs(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);
	void SetNumericalOutputs(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);

//////////////////////////////////////////////////////////////////////////
// Connections to other nodes

private:
	// map outputs to the connected node and input pin
	UPROPERTY()
	TMap<FName, FConnectedPin> Connections;

public:
	void SetConnections(TMap<FName, FConnectedPin>& InConnections) { Connections = InConnections; }
	FConnectedPin GetConnection(const FName OutputName) const { return Connections.FindRef(OutputName); }
	TSet<UFlowNode*> GetConnectedNodes() const;

//////////////////////////////////////////////////////////////////////////
// Runtime

public:
	bool bPreloaded;

protected:
	FStreamableManager StreamableManager;

#if !UE_BUILD_SHIPPING
private:
	TMap<FName, TArray<FPinRecord>> InputRecords;
	TMap<FName, TArray<FPinRecord>> OutputRecords;
	EFlowActivationState ActivationState;
#endif

public:
	UFUNCTION(BlueprintPure, Category = "FlowNode")
	UFlowSubsystem* GetFlowSubsystem() const;

	virtual UWorld* GetWorld() const override;

	void TriggerPreload();
	void TriggerFlush();

protected:
	virtual void PreloadContent();
	virtual void FlushContent();

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "PreloadContent"))
	void K2_PreloadContent();

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "FlushContent"))
	void K2_FlushContent();

	// trigger execution of input pin
	void TriggerInput(const FName& PinName);

	// method implementing node-specific logic
	virtual void ExecuteInput(const FName& PinName);

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "ExecuteInput"))
	void K2_ExecuteInput(const FName& PinName);

	// if node has only one output, we can finish node in one simple call
	void TriggerFirstOutput(const bool bFinish);

	// trigger output pin
	void TriggerOutput(const FName& PinName, const bool bFinish = false);

	// finish execution of node, it will call Cleanup()
	void Finish();

	// method implementing UObject's cleanup after node finished the work
	virtual void Cleanup();

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Cleanup"))
	void K2_Cleanup();

public:
	// define what happens when node is terminated from the outside
	virtual void ForceFinishNode();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "ForceFinishNode"))
	void K2_ForceFinishNode();

#if !UE_BUILD_SHIPPING
private:
	void ResetRecords();
#endif

#if WITH_EDITOR
public:
	UFlowNode* GetInspectedInstance() const;
	EFlowActivationState GetActivationState() const { return ActivationState; }

	TMap<uint8, FPinRecord> GetWireRecords() const;
	TArray<FPinRecord> GetInputRecords(const FName& PinName) const;
	TArray<FPinRecord> GetOutputRecords(const FName& PinName) const;

	// information displayed while node is working - displayed over node as NodeInfoPopup
	virtual FString GetStatusString() const;

	virtual UObject* GetAssetToOpen();
	virtual AActor* GetActorToFocus();
#endif

protected:
	// information displayed while node is working - displayed over node as NodeInfoPopup
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "GetStatusString"))
	FString K2_GetStatusString() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "GetAssetToOpen"))
	UObject* K2_GetAssetToOpen();

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "GetActorToFocus"))
	AActor* K2_GetActorToFocus();

	template <typename T>
	T* LoadAsset(TSoftObjectPtr<UObject> AssetPtr)
	{
		ensure(!AssetPtr.IsNull());

		if (AssetPtr.IsPending())
		{
			const FSoftObjectPath& AssetRef = AssetPtr.ToSoftObjectPath();
			AssetPtr = Cast<T>(StreamableManager.LoadSynchronous(AssetRef, false));
		}

		return Cast<T>(AssetPtr.Get());
	}

public:
	UFUNCTION(BlueprintPure, Category = "FlowNode")
	static FString GetProgressAsString(float Value);

	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void LogError(FString Message);
};
