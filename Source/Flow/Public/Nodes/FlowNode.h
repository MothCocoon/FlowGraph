#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "FlowTypes.h"
#include "FlowNode.generated.h"

class UEdGraphNode;

class UFlowAsset;
class UFlowSubsystem;

// Processing Flow Nodes creates map of connected pins
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

// Every time pin is activated, we record it and display this data while user hovers mouse over pin
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

#if WITH_EDITOR
DECLARE_DELEGATE(FFlowNodeEvent);
#endif

/**
 * Base Flow Node class
 */
UCLASS(Abstract, Blueprintable, HideCategories = Object)
class FLOW_API UFlowNode : public UObject, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_UCLASS_BODY()

	friend class SFlowGraphNode;
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

public:
	FFlowNodeEvent OnReconstructionRequested;
#endif

public:
	UEdGraphNode* GetGraphNode() const { return GraphNode; }
	
#if WITH_EDITOR
	void SetGraphNode(UEdGraphNode* NewGraph);

	FString GetCategory() const { return Category; }
	EFlowNodeStyle GetNodeStyle() const { return NodeStyle; }

	virtual FText GetTitle() const { return GetClass()->GetDisplayNameText(); }

	// Short summary of node's content - displayed over node as NodeInfoPopup
	virtual FString GetNodeDescription() const;
#endif

protected:
	// Short summary of node's content - displayed over node as NodeInfoPopup
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "GetNodeDescription"))
	FString K2_GetNodeDescription() const;

	// Inherits Guid after graph node
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
	// Class-specific and user-added inputs
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<FName> InputNames;

	// Class-specific and user-added outputs
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<FName> OutputNames;

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	TArray<FName> GetInputNames() const { return InputNames; }

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	TArray<FName> GetOutputNames() const { return OutputNames; }

public:
#if WITH_EDITOR
	virtual bool SupportsContextPins() const { return false; }

	// Be careful, enabling it might cause loading gigabytes of data as nodes would load all related data (i.e. Level Sequences)
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
	// Map outputs to the connected node and input pin
	UPROPERTY()
	TMap<FName, FConnectedPin> Connections;

public:
	void SetConnections(TMap<FName, FConnectedPin>& InConnections) { Connections = InConnections; }
	FConnectedPin GetConnection(const FName OutputName) const { return Connections.FindRef(OutputName); }
	TSet<UFlowNode*> GetConnectedNodes() const;

	static void RecursiveFindNodesByClass(UFlowNode* Node, const TSubclassOf<UFlowNode> Class, uint8 Depth, TArray<UFlowNode*>& OutNodes);

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

	// Trigger execution of input pin
	void TriggerInput(const FName& PinName);

	// Method reacting on triggering Input pin
	virtual void ExecuteInput(const FName& PinName);

	// Event reacting on triggering Input pin
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "ExecuteInput"))
	void K2_ExecuteInput(const FName& PinName);

	// Simply trigger the first Output Pin, convenient to use if node has only one output
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void TriggerFirstOutput(const bool bFinish);

	// Trigger Output Pin
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void TriggerOutput(const FName& PinName, const bool bFinish = false);

	// Finish execution of node, it will call Cleanup
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void Finish();

	void Deactivate();

	// Method called after node finished the work
	virtual void Cleanup();

	// Event called after node finished the work
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Cleanup"))
	void K2_Cleanup();

public:
	// Define what happens when node is terminated from the outside
	virtual void ForceFinishNode();

protected:
	// Define what happens when node is terminated from the outside
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

	// Information displayed while node is working - displayed over node as NodeInfoPopup
	virtual FString GetStatusString() const;

	virtual FString GetAssetPath();
	virtual UObject* GetAssetToEdit();
	virtual AActor* GetActorToFocus();
#endif

protected:
	// Information displayed while node is working - displayed over node as NodeInfoPopup
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "GetStatusString"))
	FString K2_GetStatusString() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "GetAssetPath"))
    FString K2_GetAssetPath();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "GetAssetToEdit"))
    UObject* K2_GetAssetToEdit();

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
