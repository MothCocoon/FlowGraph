// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EdGraph/EdGraphNode.h"
#include "GameplayTagContainer.h"
#include "UObject/TextProperty.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "FlowNodeBase.h"
#include "FlowTypes.h"
#include "Interfaces/FlowDataPinGeneratorInterface.h"
#include "Interfaces/FlowDataPinValueSupplierInterface.h"
#include "Nodes/FlowPin.h"
#include "Types/FlowArray.h"

#include "FlowNode.generated.h"

/**
 * A Flow Node is UObject-based node designed to handle entire gameplay feature within single node.
 */
UCLASS(Abstract, Blueprintable, HideCategories = Object)
class FLOW_API UFlowNode 
	: public UFlowNodeBase
	, public IFlowDataPinGeneratorInterface
	, public IFlowDataPinValueSupplierInterface
	, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_UCLASS_BODY()

	friend class SFlowGraphNode;
	friend class UFlowAsset;
	friend class UFlowGraphNode;
	friend class UFlowNodeAddOn;
	friend class SFlowInputPinHandle;
	friend class SFlowOutputPinHandle;

//////////////////////////////////////////////////////////////////////////
// Node

#if WITH_EDITORONLY_DATA

protected:
	UPROPERTY()
	TArray<TSubclassOf<UFlowAsset>> AllowedAssetClasses;

	UPROPERTY()
	TArray<TSubclassOf<UFlowAsset>> DeniedAssetClasses;
#endif

public:
	// UFlowNodeBase
	virtual UFlowNode* GetFlowNodeSelfOrOwner() override { return this; }
	virtual bool IsSupportedInputPinName(const FName& PinName) const override;
	// --

public:
	// UObject	
	virtual void PostLoad() override;
	// --

#if WITH_EDITOR
	// UObject	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --

	virtual EDataValidationResult ValidateNode();
	void ValidateFlowPinArrayIsUnique(const TArray<FFlowPin>& FlowPins, TSet<FName>& InOutUniquePinNames, EDataValidationResult& InOutResult);
#endif

	// Inherits Guid after graph node
	UPROPERTY()
	FGuid NodeGuid;

public:
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void SetGuid(const FGuid& NewGuid) { NodeGuid = NewGuid; }

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	const FGuid& GetGuid() const { return NodeGuid; }

	// Returns a random seed suitable for this flow node,
	// by default based on the node Guid, 
	// but may be overridden in subclasses to supply some other value.
	virtual int32 GetRandomSeed() const override { return GetTypeHash(NodeGuid); }

public:	
	virtual bool CanFinishGraph() const { return false; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<EFlowSignalMode> AllowedSignalModes;

	// If enabled, signal will pass through node without calling ExecuteInput()
	// Designed to handle patching
	UPROPERTY()
	EFlowSignalMode SignalMode;

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

	void AddInputPins(const TArray<FFlowPin>& Pins);
	void AddOutputPins(const TArray<FFlowPin>& Pins);

#if WITH_EDITOR
	// Utility function to rebuild a pin array in editor (either InputPins or OutputPins, passed as InOutPins)
	// returns true if the InOutPins array was rebuilt
	bool RebuildPinArray(const TArray<FName>& NewPinNames, TArray<FFlowPin>& InOutPins, const FFlowPin& DefaultPin);
	bool RebuildPinArray(const TArray<FFlowPin>& NewPins, TArray<FFlowPin>& InOutPins, const FFlowPin& DefaultPin);
#endif // WITH_EDITOR;

	// always use default range for nodes with user-created outputs i.e. Execution Sequence
	void SetNumberedInputPins(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);
	void SetNumberedOutputPins(const uint8 FirstNumber = 0, const uint8 LastNumber = 1);

	uint8 CountNumberedInputs() const;
	uint8 CountNumberedOutputs() const;

public:
	const TArray<FFlowPin>& GetInputPins() const { return InputPins; }
	const TArray<FFlowPin>& GetOutputPins() const { return OutputPins; }

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	TArray<FName> GetInputNames() const;

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	TArray<FName> GetOutputNames() const;

#if WITH_EDITOR
	// IFlowContextPinSupplierInterface
	virtual bool SupportsContextPins() const override;
	virtual TArray<FFlowPin> GetContextInputs() const override;
	virtual TArray<FFlowPin> GetContextOutputs() const override;
	// --

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
	// Map input/outputs to the connected node and input pin
	UPROPERTY()
	TMap<FName, FConnectedPin> Connections;

public:
	FConnectedPin GetConnection(const FName OutputName) const { return Connections.FindRef(OutputName); }

	UE_DEPRECATED(5.5, "Please use GatherConnectedNodes instead.")
	TSet<UFlowNode*> GetConnectedNodes() const { return GatherConnectedNodes(); }

	UFUNCTION(BlueprintPure, Category= "FlowNode")
	TSet<UFlowNode*> GatherConnectedNodes() const;
	
	FName GetPinConnectedToNode(const FGuid& OtherNodeGuid);

	UFUNCTION(BlueprintPure, Category= "FlowNode")
	bool IsInputConnected(const FName& PinName, bool bErrorIfPinNotFound = true) const;

	UFUNCTION(BlueprintPure, Category= "FlowNode")
	bool IsOutputConnected(const FName& PinName, bool bErrorIfPinNotFound = true) const;

	bool IsInputConnected(const FFlowPin& FlowPin) const;
	bool IsOutputConnected(const FFlowPin& FlowPin) const;

	FFlowPin* FindInputPinByName(const FName& PinName);
	FFlowPin* FindOutputPinByName(const FName& PinName);
	const FFlowPin* FindInputPinByName(const FName& PinName) const { return const_cast<UFlowNode*>(this)->FindInputPinByName(PinName); }
	const FFlowPin* FindOutputPinByName(const FName& PinName) const { return const_cast<UFlowNode*>(this)->FindOutputPinByName(PinName); }

	static void RecursiveFindNodesByClass(UFlowNode* Node, const TSubclassOf<UFlowNode> Class, uint8 Depth, TArray<UFlowNode*>& OutNodes);

protected:

	// Slow and fast lookup functions, based on whether we are proactively caching the connections for quick lookup 
	// in the Connections array (by PinCategory)
	bool FindConnectedNodeForPinFast(const FName& FlowPinName, FGuid* FoundGuid = nullptr, FName* OutConnectedPinName = nullptr) const;
	bool FindConnectedNodeForPinSlow(const FName& FlowPinName, FGuid* FoundGuid = nullptr, FName* OutConnectedPinName = nullptr) const;

	// Return all connections to a Pin this Node knows about.
	// Connections are only stored on one of the Nodes they connect depending on pin type.
	// As such, this function may not return anything even if the Node is connected to the Pin.
	// Use UFlowAsset::GetAllPinsConnectedToPin() to do a guaranteed find of all Connections.
	TArray<FConnectedPin> GetKnownConnectionsToPin(const FConnectedPin& Pin) const;
	
//////////////////////////////////////////////////////////////////////////
// Data Pins

public:

#if WITH_EDITORONLY_DATA	
	UPROPERTY(VisibleDefaultsOnly, AdvancedDisplay, Category = "FlowNode", meta = (GetByRef))
	TArray<FFlowPin> AutoInputDataPins;

	UPROPERTY(VisibleDefaultsOnly, AdvancedDisplay, Category = "FlowNode", meta = (GetByRef))
	TArray<FFlowPin> AutoOutputDataPins;
#endif // WITH_EDITORONLY_DATA	

#if WITH_EDITOR
	void SetAutoInputDataPins(const TArray<FFlowPin>& AutoInputPins);
	void SetAutoOutputDataPins(const TArray<FFlowPin>& AutoOutputPins);
	const TArray<FFlowPin>& GetAutoInputDataPins() const { return AutoInputDataPins; }
	const TArray<FFlowPin>& GetAutoOutputDataPins() const { return AutoOutputDataPins; }
	
	TArray<FFlowPin>& GetMutableAutoInputDataPins() { return AutoInputDataPins; }
	TArray<FFlowPin>& GetMutableAutoOutputDataPins() { return AutoOutputDataPins; }
	
	void SetConnections(const TMap<FName, FConnectedPin>& InConnections);
	virtual void OnConnectionsChanged(const TMap<FName, FConnectedPin>& OldConnections) { }
#endif // WITH_EDITOR

	// IFlowDataPinValueSupplierInterface
public:
	virtual FFlowDataPinResult TrySupplyDataPin_Implementation(FName PinName) const override;

	// Advanced helper for TrySupplyDataPin, which can be overridden in subclasses to provide alternate sourcing for properties.
	// If returns true, either OutFoundProperty or OutFoundInstancedStruct is expected to carry the property value.
	// (this function is used for cases like DefineProperties, Start, and blackboard lookup nodes)
	virtual bool TryFindPropertyByPinName(
		const UObject& PropertyOwnerObject,
		const FName& PinName,
		const FProperty*& OutFoundProperty,
		TInstancedStruct<FFlowDataPinValue>& OutFoundInstancedStruct) const;

	// Advanced helper for TrySupplyDataPin, which can be overridden in subclasses to provide additional or replacement object(s)
	// for sourcing the properties for the given pin name. These objects will have PopulateResult called on them.
	// (this function is used for cases like ExecuteComponent)
	virtual void GatherPotentialPropertyOwnersForDataPins(TArray<const UObject*>& InOutOwners) const;

	bool TryGatherPropertyOwnersAndPopulateResult(
		const FName& PinName,
		const FFlowPinType& DataPinType,
		const FFlowPin& FlowPin,
		FFlowDataPinResult& OutSuppliedResult) const;

protected:
	// Static implementation of the default TryFindPropertyByPinName (which subclasses can incorperate into overrides)
	static bool TryFindPropertyByPinName_Static(
		const UObject& PropertyOwnerObject,
		const FName& PinName,
		const FProperty*& OutFoundProperty,
		TInstancedStruct<FFlowDataPinValue>& OutFoundInstancedStruct);

	// #FlowDataPinLegacy
public:
	void FixupDataPinTypes();

protected:
	static void FixupDataPinTypesForArray(TArray<FFlowPin>& MutableDataPinArray);
	static void FixupDataPinTypesForPin(FFlowPin& MutableDataPin);
	// --

public:

	using TFlowPinValueSupplierDataArray = FlowArray::TInlineArray<FFlowPinValueSupplierData,4>;
	bool TryGetFlowDataPinSupplierDatasForPinName(
		const FName& PinName,
		TFlowPinValueSupplierDataArray& InOutPinValueSupplierDatas) const;

	// IFlowDataPinGeneratorInterface
#if WITH_EDITOR
	virtual void AutoGenerateDataPins(FFlowAutoDataPinsWorkingData& InOutWorkingData) const override;
#endif
	// --

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
	UPROPERTY(SaveGame)
	EFlowNodeState ActivationState;

public:
	EFlowNodeState GetActivationState() const { return ActivationState; }
	bool HasFinished() const { return EFlowNodeState_Classifiers::IsFinishedState(ActivationState); }

#if !UE_BUILD_SHIPPING

protected:
	TMap<FName, TArray<FPinRecord>> InputRecords;
	TMap<FName, TArray<FPinRecord>> OutputRecords;
#endif

public:
	void TriggerPreload();
	void TriggerFlush();

protected:

	// Trigger execution of input pin
	void TriggerInput(const FName& PinName, const EFlowPinActivationType ActivationType = EFlowPinActivationType::Default);

protected:
	void Deactivate();

public:
	virtual void TriggerFirstOutput(const bool bFinish) override;
	virtual void TriggerOutput(FName PinName, const bool bFinish = false, const EFlowPinActivationType ActivationType = EFlowPinActivationType::Default) override;
	virtual void Finish() override;

private:
	void ResetRecords();

//////////////////////////////////////////////////////////////////////////
// SaveGame support

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
	
//////////////////////////////////////////////////////////////////////////
// Utils

#if WITH_EDITOR
public:
	UFlowNode* GetInspectedInstance() const;

	TMap<uint8, FPinRecord> GetWireRecords() const;
	TArray<FPinRecord> GetPinRecords(const FName& PinName, const EEdGraphPinDirection PinDirection) const;

	// Information displayed while node is working - displayed over node as NodeInfoPopup
	FString GetStatusStringForNodeAndAddOns() const;
	virtual bool GetStatusBackgroundColor(FLinearColor& OutColor) const;

	virtual FString GetAssetPath();
	virtual UObject* GetAssetToEdit();
	virtual AActor* GetActorToFocus();
#endif

protected:
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
};
