// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "FlowTypes.h"
#include "Nodes/FlowPin.h"
#include "FlowNode.generated.h"

class UFlowAsset;
class UFlowSubsystem;

#if WITH_EDITOR
DECLARE_DELEGATE(FFlowNodeEvent);
#endif

/**
 * A Flow Node is UObject-based node designed to handle entire gameplay feature within single node.
 */
UCLASS(Abstract, Blueprintable, HideCategories = Object)
class FLOW_API UFlowNode : public UObject, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_UCLASS_BODY()

	friend class SFlowGraphNode;
	friend class UFlowAsset;
	friend class UFlowGraphNode;
	friend class UFlowGraphSchema;

//////////////////////////////////////////////////////////////////////////
// Node

private:
	UPROPERTY()
	UEdGraphNode* GraphNode;

#if WITH_EDITORONLY_DATA
protected:
	UPROPERTY()
	FString Category;

	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	EFlowNodeStyle NodeStyle;

	uint8 bCanDelete : 1;
	uint8 bCanDuplicate : 1;

	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	bool bNodeDeprecated;
	
	// If this node is deprecated, it might be replaced by another node
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TSubclassOf<UFlowNode> ReplacedBy;

public:
	FFlowNodeEvent OnReconstructionRequested;
#endif

public:
#if WITH_EDITOR
	// UObject	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
	// --

	// Opportunity to update node's data before UFlowGraphNode would call ReconstructNode()
	virtual void FixNode(UEdGraphNode* NewGraph);
#endif

	UEdGraphNode* GetGraphNode() const { return GraphNode; }

#if WITH_EDITOR
	void SetGraphNode(UEdGraphNode* NewGraph);

	virtual FString GetNodeCategory() const;
	virtual FText GetNodeTitle() const;
	virtual FText GetNodeToolTip() const;
	
	// This method allows to have different for every node instance, i.e. Red if node represents enemy, Green if node represents a friend
	virtual bool GetDynamicTitleColor(FLinearColor& OutColor) const { return false; }

	EFlowNodeStyle GetNodeStyle() const { return NodeStyle; }

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
	static FFlowPin DefaultInputPin;
	static FFlowPin DefaultOutputPin;

protected:
	// Class-specific and user-added inputs
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<FFlowPin> InputPins;

	// Class-specific and user-added outputs
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<FFlowPin> OutputPins;

	void AddInputPins(TArray<FName> PinNames);
	void AddOutputPins(TArray<FName> PinNames);

	// always use default range for nodes with user-created outputs i.e. Execution Sequence
	void SetNumberedInputPins(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);
	void SetNumberedOutputPins(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);

	TArray<FFlowPin> GetInputPins() const { return InputPins; }
	TArray<FFlowPin> GetOutputPins() const { return OutputPins; }

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	TArray<FName> GetInputNames() const;

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	TArray<FName> GetOutputNames() const;

public:
#if WITH_EDITOR
	virtual bool SupportsContextPins() const { return false; }

	// Be careful, enabling it might cause loading gigabytes of data as nodes would load all related data (i.e. Level Sequences)
	virtual bool CanRefreshContextPinsOnLoad() const { return false; }

	virtual TArray<FName> GetContextInputs() { return TArray<FName>(); }
	virtual TArray<FName> GetContextOutputs() { return TArray<FName>(); }

	virtual bool CanUserAddInput() const;
	virtual bool CanUserAddOutput() const;

	void RemoveUserInput();
	void RemoveUserOutput();
#endif

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "CanUserAddInput"))
	bool K2_CanUserAddInput() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "CanUserAddOutput"))
	bool K2_CanUserAddOutput() const;

//////////////////////////////////////////////////////////////////////////
// Connections to other nodes

private:
	// Map outputs to the connected node and input pin
	UPROPERTY()
	TMap<FName, FConnectedPin> Connections;

public:
	void SetConnections(const TMap<FName, FConnectedPin>& InConnections) { Connections = InConnections; }
	FConnectedPin GetConnection(const FName OutputName) const { return Connections.FindRef(OutputName); }
	TSet<UFlowNode*> GetConnectedNodes() const;

	UFUNCTION(BlueprintPure, Category= "FlowNode")
	bool IsInputConnected(const FName& PinName) const;

	UFUNCTION(BlueprintPure, Category= "FlowNode")
	bool IsOutputConnected(const FName& PinName) const;

	static void RecursiveFindNodesByClass(UFlowNode* Node, const TSubclassOf<UFlowNode> Class, uint8 Depth, TArray<UFlowNode*>& OutNodes);

//////////////////////////////////////////////////////////////////////////
// Debugger
protected:
	static FString MissingIdentityTag;
	static FString MissingNotifyTag;
	static FString MissingClass;
	static FString NoActorsFound;

//////////////////////////////////////////////////////////////////////////
// Executing node instance

public:
	bool bPreloaded;

protected:
	FStreamableManager StreamableManager;

	UPROPERTY(SaveGame)
	EFlowNodeState ActivationState;
	
#if !UE_BUILD_SHIPPING
private:
	TMap<FName, TArray<FPinRecord>> InputRecords;
	TMap<FName, TArray<FPinRecord>> OutputRecords;
#endif

public:
	UFUNCTION(BlueprintPure, Category = "FlowNode")
	UFlowSubsystem* GetFlowSubsystem() const;

	virtual UWorld* GetWorld() const override;

protected:
	// Method called just after creating the node instance, while initializing the Flow Asset instance
	// This happens before executing graph, only called during gameplay
	virtual void InitializeInstance();

	// Event called just after creating the node instance, while initializing the Flow Asset instance
	// This happens before executing graph, only called during gameplay
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "InitInstance"))
	void K2_InitializeInstance();

public:
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
	void TriggerInput(const FName& PinName, const bool bForcedActivation = false);

	// Method reacting on triggering Input pin
	virtual void ExecuteInput(const FName& PinName);

	// Event reacting on triggering Input pin
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "ExecuteInput"))
	void K2_ExecuteInput(const FName& PinName);

	// Simply trigger the first Output Pin, convenient to use if node has only one output
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void TriggerFirstOutput(const bool bFinish);

	// Trigger Output Pin
	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (HidePin = "bForcedActivation"))
	void TriggerOutput(const FName& PinName, const bool bFinish = false, const bool bForcedActivation = false);

	void TriggerOutput(const FString& PinName, const bool bFinish = false);
	void TriggerOutput(const FText& PinName, const bool bFinish = false);
	void TriggerOutput(const TCHAR* PinName, const bool bFinish = false);

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

private:
	void ResetRecords();

#if WITH_EDITOR
public:
	UFlowNode* GetInspectedInstance() const;
	EFlowNodeState GetActivationState() const { return ActivationState; }

	TMap<uint8, FPinRecord> GetWireRecords() const;
	TArray<FPinRecord> GetPinRecords(const FName& PinName, const EEdGraphPinDirection PinDirection) const;

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
	static FString GetIdentityTagDescription(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	static FString GetIdentityTagsDescription(const FGameplayTagContainer& Tags);

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	static FString GetNotifyTagsDescription(const FGameplayTagContainer& Tags);

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	static FString GetClassDescription(const TSubclassOf<UObject> Class);

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	static FString GetProgressAsString(float Value);

	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void LogError(FString Message, const EFlowOnScreenMessageType OnScreenMessageType = EFlowOnScreenMessageType::Permanent) const;

	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void SaveInstance(FFlowNodeSaveData& NodeRecord);

	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void LoadInstance(const FFlowNodeSaveData& NodeRecord);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "FlowNode")
	void OnSave();
	
	UFUNCTION(BlueprintNativeEvent, Category = "FlowNode")
	void OnLoad();

private:
	UPROPERTY()
	TArray<FName> InputNames_DEPRECATED;

	UPROPERTY()
	TArray<FName> OutputNames_DEPRECATED;
};
