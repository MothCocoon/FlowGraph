// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EdGraph/EdGraphNode.h"
#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "FlowMessageLog.h"
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
	friend class SFlowInputPinHandle;
	friend class SFlowOutputPinHandle;

//////////////////////////////////////////////////////////////////////////
// Node

private:
	UPROPERTY()
	UEdGraphNode* GraphNode;

#if WITH_EDITORONLY_DATA

protected:
	TArray<TSubclassOf<UFlowAsset>> AllowedAssetClasses;
	TArray<TSubclassOf<UFlowAsset>> DeniedAssetClasses;

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
	virtual void FixNode(UEdGraphNode* NewGraphNode);

	virtual EDataValidationResult ValidateNode() { return EDataValidationResult::NotValidated; }

	// used when import graph from another asset
	virtual void PostImport() {}
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
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Get Node Description"))
	FString K2_GetNodeDescription() const;

	// Inherits Guid after graph node
	UPROPERTY()
	FGuid NodeGuid;

public:
	void SetGuid(const FGuid NewGuid) { NodeGuid = NewGuid; }
	FGuid GetGuid() const { return NodeGuid; }

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	UFlowAsset* GetFlowAsset() const;

protected:
	virtual bool CanFinishGraph() const { return false; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<EFlowSignalMode> AllowedSignalModes;

	// If enabled, signal will pass through node without calling ExecuteInput()
	// Designed to handle patching
	UPROPERTY()
	EFlowSignalMode SignalMode;

#if WITH_EDITOR
	FFlowMessageLog ValidationLog;
#endif

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

	void AddInputPins(TArray<FFlowPin> Pins);
	void AddOutputPins(TArray<FFlowPin> Pins);

	// always use default range for nodes with user-created outputs i.e. Execution Sequence
	void SetNumberedInputPins(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);
	void SetNumberedOutputPins(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);

	uint8 CountNumberedInputs() const;
	uint8 CountNumberedOutputs() const;

	TArray<FFlowPin> GetInputPins() const { return InputPins; }
	TArray<FFlowPin> GetOutputPins() const { return OutputPins; }

public:
	UFUNCTION(BlueprintPure, Category = "FlowNode")
	TArray<FName> GetInputNames() const;

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	TArray<FName> GetOutputNames() const;

#if WITH_EDITOR
	virtual bool SupportsContextPins() const { return false; }

	// Be careful, enabling it might cause loading gigabytes of data as nodes would load all related data (i.e. Level Sequences)
	virtual bool CanRefreshContextPinsOnLoad() const { return false; }

	virtual TArray<FFlowPin> GetContextInputs() { return TArray<FFlowPin>(); }
	virtual TArray<FFlowPin> GetContextOutputs() { return TArray<FFlowPin>(); }

	virtual bool CanUserAddInput() const;
	virtual bool CanUserAddOutput() const;

	void RemoveUserInput(const FName& PinName);
	void RemoveUserOutput(const FName& PinName);
#endif

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Can User Add Input"))
	bool K2_CanUserAddInput() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Can User Add Output"))
	bool K2_CanUserAddOutput() const;

//////////////////////////////////////////////////////////////////////////
// Connections to other nodes

protected:
	// Map outputs to the connected node and input pin
	UPROPERTY()
	TMap<FName, FConnectedPin> Connections;

public:
	void SetConnections(const TMap<FName, FConnectedPin>& InConnections) { Connections = InConnections; }
	FConnectedPin GetConnection(const FName OutputName) const { return Connections.FindRef(OutputName); }

	TSet<UFlowNode*> GetConnectedNodes() const;
	FName GetPinConnectedToNode(const FGuid& OtherNodeGuid);

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

public:
	EFlowNodeState GetActivationState() const { return ActivationState; }

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
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Init Instance"))
	void K2_InitializeInstance();

public:
	void TriggerPreload();
	void TriggerFlush();

protected:
	virtual void PreloadContent();
	virtual void FlushContent();

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Preload Content"))
	void K2_PreloadContent();

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Flush Content"))
	void K2_FlushContent();

	virtual void OnActivate();

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "On Activate"))
	void K2_OnActivate();

	// Trigger execution of input pin
	void TriggerInput(const FName& PinName, const EFlowPinActivationType ActivationType = EFlowPinActivationType::Default);

	// Method reacting on triggering Input pin
	virtual void ExecuteInput(const FName& PinName);

	// Event reacting on triggering Input pin
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Execute Input"))
	void K2_ExecuteInput(const FName& PinName);

	// Simply trigger the first Output Pin, convenient to use if node has only one output
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void TriggerFirstOutput(const bool bFinish);

	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (HidePin = "bForcedActivation"))
	void TriggerOutput(const FName& PinName, const bool bFinish = false, const EFlowPinActivationType ActivationType = EFlowPinActivationType::Default);

	void TriggerOutput(const FString& PinName, const bool bFinish = false);
	void TriggerOutput(const FText& PinName, const bool bFinish = false);
	void TriggerOutput(const TCHAR* PinName, const bool bFinish = false);

	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (HidePin = "ActivationType"))
	void TriggerOutputPin(const FFlowOutputPinHandle Pin, const bool bFinish = false, const EFlowPinActivationType ActivationType = EFlowPinActivationType::Default);

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
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Force Finish Node"))
	void K2_ForceFinishNode();

private:
	void ResetRecords();

#if WITH_EDITOR

public:
	UFlowNode* GetInspectedInstance() const;

	TMap<uint8, FPinRecord> GetWireRecords() const;
	TArray<FPinRecord> GetPinRecords(const FName& PinName, const EEdGraphPinDirection PinDirection) const;

	// Information displayed while node is working - displayed over node as NodeInfoPopup
	virtual FString GetStatusString() const;
	virtual bool GetStatusBackgroundColor(FLinearColor& OutColor) const;

	virtual FString GetAssetPath();
	virtual UObject* GetAssetToEdit();
	virtual AActor* GetActorToFocus();
#endif

protected:
	// Information displayed while node is working - displayed over node as NodeInfoPopup
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Get Status String"))
	FString K2_GetStatusString() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Get Status Background Color"))
	bool K2_GetStatusBackgroundColor(FLinearColor& OutColor) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Get Asset Path"))
	FString K2_GetAssetPath();

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Get Asset To Edit"))
	UObject* K2_GetAssetToEdit();

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Get Actor To Focus"))
	AActor* K2_GetActorToFocus();

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

public:
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void SaveInstance(FFlowNodeSaveData& NodeRecord);

	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void LoadInstance(const FFlowNodeSaveData& NodeRecord);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "FlowNode")
	void OnSave();

	UFUNCTION(BlueprintNativeEvent, Category = "FlowNode")
	void OnLoad();

	UFUNCTION(BlueprintNativeEvent, Category = "FlowNode")
	void OnPassThrough();

public:
	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (DevelopmentOnly))
	void LogError(FString Message, const EFlowOnScreenMessageType OnScreenMessageType = EFlowOnScreenMessageType::Permanent);

	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (DevelopmentOnly))
	void LogWarning(FString Message);

	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (DevelopmentOnly))
	void LogNote(FString Message);

#if !UE_BUILD_SHIPPING
private:
	bool BuildMessage(FString& Message) const;
#endif
};
