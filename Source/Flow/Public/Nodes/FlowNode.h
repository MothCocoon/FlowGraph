// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "EdGraph/EdGraphNode.h"
#include "GameplayTagContainer.h"
#include "UObject/TextProperty.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "FlowNodeBase.h"
#include "FlowTypes.h"
#include "Interfaces/FlowDataPinValueSupplierInterface.h"
#include "Nodes/FlowPin.h"
#include "Types/FlowArray.h"
#include "Types/FlowAutoDataPinsWorkingData.h"
#include "Types/FlowPinConnectionChange.h"
#include "FlowNode.generated.h"


/**
 * A Flow Node is UObject-based node designed to handle entire gameplay feature within single node.
 */
UCLASS(Abstract, Blueprintable, HideCategories = Object)
class FLOW_API UFlowNode : public UFlowNodeBase
						 , public IFlowDataPinValueSupplierInterface
						 , public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()

public:
	UFlowNode();

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

#if WITH_EDITOR
	/* Set up UFlowNodeBase when being opened for edit in the editor. */
	virtual void SetupForEditing(UEdGraphNode& EdGraphNode) override;

	/**
	* Editor-only: ensure any editor-time parent pointers are correctly set for this node and any child AddOns.
	* Goal: AddOns always have a valid FlowNode pointer while being edited (creation/paste/undo/reconstruct/open).
	* Safe to call repeatedly.
	*/
	virtual void EnsureAddOnFlowNodePointersForEditor();
#endif

public:
	// UObject	
	virtual void PostLoad() override;
	// --

#if WITH_EDITOR
	// UObject	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --
#endif

	/* Inherits Guid after graph node. */
	UPROPERTY()
	FGuid NodeGuid;

public:
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void SetGuid(const FGuid& NewGuid) { NodeGuid = NewGuid; }

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	const FGuid& GetGuid() const { return NodeGuid; }

	/* Returns a random seed suitable for this flow node,
	 * by default based on the node Guid,
	 * but may be overridden in subclasses to supply some other value. */
	virtual int32 GetRandomSeed() const override { return GetTypeHash(NodeGuid); }

	virtual const UFlowNode* GetParentNode() const override
	{
		return UFlowNodeBase::GetFlowNodeSelfOrOwner();
	}

public:
	virtual bool CanFinishGraph() const { return K2_CanFinishGraph(); }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<EFlowSignalMode> AllowedSignalModes;

	/* If enabled, signal will pass through node without calling ExecuteInput().
	 * Designed to handle patching already released games. */
	UPROPERTY()
	EFlowSignalMode SignalMode;

//////////////////////////////////////////////////////////////////////////
// All created pins (default, class-specific and added by user)

public:
	static FFlowPin DefaultInputPin;
	static FFlowPin DefaultOutputPin;

protected:
	/* Class-specific and user-added inputs. */
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<FFlowPin> InputPins;

	/* Class-specific and user-added outputs. */
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TArray<FFlowPin> OutputPins;

	void AddInputPins(const TArray<FFlowPin>& Pins);
	void AddOutputPins(const TArray<FFlowPin>& Pins);

#if WITH_EDITOR
	/* Utility function to rebuild a pin array in editor (either InputPins or OutputPins, passed as InOutPins)
	 * returns true if the InOutPins array was rebuilt. */
	bool RebuildPinArray(const TArray<FName>& NewPinNames, TArray<FFlowPin>& InOutPins, const FFlowPin& DefaultPin);
	bool RebuildPinArray(const TArray<FFlowPin>& NewPins, TArray<FFlowPin>& InOutPins, const FFlowPin& DefaultPin);
#endif

	/* Always use default range for nodes with user-created outputs i.e. Execution Sequence. */
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
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Can Finish Graph"))
	bool K2_CanFinishGraph() const;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Can User Add Input"))
	bool K2_CanUserAddInput() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Can User Add Output"))
	bool K2_CanUserAddOutput() const;

//////////////////////////////////////////////////////////////////////////
// Connections to other nodes

protected:
	/* Map input/outputs to the connected node and input pin. */
	UPROPERTY()
	TMap<FName, FConnectedPin> Connections;

public:
#if WITH_EDITOR
	void SetConnections(const TMap<FName, FConnectedPin>& InConnections);
#endif

	FConnectedPin GetConnection(const FName OutputName) const { return Connections.FindRef(OutputName); }

	UFUNCTION(BlueprintPure, Category= "FlowNode")
	TSet<UFlowNode*> GatherConnectedNodes() const;

	FName GetPinConnectedToNode(const FGuid& OtherNodeGuid);

	UFUNCTION(BlueprintPure, Category= "FlowNode")
	bool IsInputConnected(const FName& PinName, bool bErrorIfPinNotFound = true) const;

	UFUNCTION(BlueprintPure, Category= "FlowNode")
	bool IsOutputConnected(const FName& PinName, bool bErrorIfPinNotFound = true) const;

	// Preferred signatures for:
	// - exec output pins
	// - data input pins
	// ... otherwise use the array signatures below
	bool FindFirstInputPinConnection(const FName& PinName, bool bErrorIfPinNotFound, FConnectedPin& FirstConnectedPin) const;
	bool FindFirstOutputPinConnection(const FName& PinName, bool bErrorIfPinNotFound, FConnectedPin& FirstConnectedPin) const;
	bool FindFirstInputPinConnection(const FFlowPin& FlowPin, FConnectedPin& FirstConnectedPin) const;
	bool FindFirstOutputPinConnection(const FFlowPin& FlowPin, FConnectedPin& FirstConnectedPin) const;

	// Preferred signatures for:
	// - exec input pins
	// - data output pins
	// - cases where you do not need the connection info (with ConnectedPins == nullptr)
	// ... otherwise use the non-array signatures above
	bool FindInputPinConnections(const FName& PinName, bool bErrorIfPinNotFound, TArray<FConnectedPin>* ConnectedPins = nullptr) const;
	bool FindOutputPinConnections(const FName& PinName, bool bErrorIfPinNotFound, TArray<FConnectedPin>* ConnectedPins = nullptr) const;
	bool FindInputPinConnections(const FFlowPin& FlowPin, TArray<FConnectedPin>* ConnectedPins = nullptr) const;
	bool FindOutputPinConnections(const FFlowPin& FlowPin, TArray<FConnectedPin>* ConnectedPins = nullptr) const;

	FFlowPin* FindInputPinByName(const FName& PinName);
	FFlowPin* FindOutputPinByName(const FName& PinName);
	const FFlowPin* FindInputPinByName(const FName& PinName) const { return const_cast<UFlowNode*>(this)->FindInputPinByName(PinName); }
	const FFlowPin* FindOutputPinByName(const FName& PinName) const { return const_cast<UFlowNode*>(this)->FindOutputPinByName(PinName); }

	static void RecursiveFindNodesByClass(UFlowNode* Node, const TSubclassOf<UFlowNode> Class, uint8 Depth, TArray<UFlowNode*>& OutNodes);

protected:
	/* Slow and fast lookup functions, based on whether we are proactively caching the connections for quick lookup
	 * in the Connections array (by PinCategory). */
	bool FindConnectedNodeForPinCached(const FName& FlowPinName, FConnectedPin& ConnectedPin) const;
	bool FindConnectedNodeForPinUncached(const FName& FlowPinName, TArray<FConnectedPin>* ConnectedPins = nullptr) const;

	/* Helper templates for Find*PinConnection* functions */
	template <bool bExecIsCached>
	bool FindFirstPinConnection(const FFlowPin& FlowPin, const TArray<FFlowPin>& FlowPinArray, FConnectedPin& FirstConnectedPin) const;		
	template <bool bExecIsCached>
	bool FindPinConnections(const FFlowPin& FlowPin, const TArray<FFlowPin>& FlowPinArray, TArray<FConnectedPin>* ConnectedPins) const;

	/* Return all connections to a Pin this Node knows about.
	 * Connections are only stored on one of the Nodes they connect depending on pin type.
	 * As such, this function may not return anything even if the Node is connected to the Pin.
	 * Use UFlowAsset::GetAllPinsConnectedToPin() to do a guaranteed find of all Connections. */
	TArray<FConnectedPin> GetKnownConnectionsToPin(const FConnectedPin& Pin) const;

#if WITH_EDITOR
	static void BuildConnectionChangeList(
		const UFlowAsset& FlowAsset,
		const TMap<FName, FConnectedPin>& OldConnections,
		const TMap<FName, FConnectedPin>& NewConnections,
		TArray<FFlowPinConnectionChange>& OutChanges);

	/* Broadcasts OnEditorPinConnectionsChanged to this node and all AddOns */
	void BroadcastEditorPinConnectionsChanged(const TArray<FFlowPinConnectionChange>& Changes);
#endif

//////////////////////////////////////////////////////////////////////////
// Data Pins

public:
	using TFlowPinValueSupplierDataArray = FlowArray::TInlineArray<FFlowPinValueSupplierData, 4>;

	/* Map for PinName to Property supplier for non-trivial data pin property lookups.
	 * Non-trivial means a different pin name from its property source, or a non-zero property owner object index.
	 * See TryGatherPropertyOwnersAndPopulateResult(). */
	UPROPERTY()
	TMap<FName, FFlowPinPropertySource> MapDataPinNameToPropertySource;

#if WITH_EDITORONLY_DATA
protected:	
	UPROPERTY(VisibleDefaultsOnly, AdvancedDisplay, Category = "FlowNode", meta = (GetByRef))
	TArray<FFlowPin> AutoInputDataPins;

	UPROPERTY(VisibleDefaultsOnly, AdvancedDisplay, Category = "FlowNode", meta = (GetByRef))
	TArray<FFlowPin> AutoOutputDataPins;
#endif

#if WITH_EDITOR
public:
	bool TryUpdateAutoDataPins();
#endif

	// IFlowDataPinValueSupplierInterface
public:
	virtual FFlowDataPinResult TrySupplyDataPin(FName PinName) const override;

protected:
	/* Helper for TryGetFlowDataPinSupplierDatasForPinName(). */
	void TryAddSupplierDataToArray(FFlowPinValueSupplierData& InOutSupplierData, TFlowPinValueSupplierDataArray& InOutPinValueSupplierDatas) const;

public:
	/* Advanced helper for TrySupplyDataPin, which can be overridden in subclasses to provide additional or replacement object(s)
	 * for sourcing the properties for the given pin name. These objects will have PopulateResult called on them.
	 * This function is used for cases like ExecuteComponent. */
	virtual void GatherDataPinValueOwnerCollection(FFlowDataPinValueOwnerCollection& ValueOwnerCollection) const;

	bool TryGatherPropertyOwnersAndPopulateResult(
		const FName& PinName,
		const FFlowPinType& DataPinType,
		const FFlowPin& FlowPin,
		FFlowDataPinResult& OutSuppliedResult) const;

	bool TryGetFlowDataPinSupplierDatasForPinName(const FName& PinName, TFlowPinValueSupplierDataArray& InOutPinValueSupplierDatas) const;

	// #FlowDataPinLegacy
public:
	void FixupDataPinTypes();

protected:
	static void FixupDataPinTypesForArray(TArray<FFlowPin>& MutableDataPinArray);
	static void FixupDataPinTypesForPin(FFlowPin& MutableDataPin);
	// --

//////////////////////////////////////////////////////////////////////////
// Debugger

protected:
	static FString MissingIdentityTag;
	static FString MissingNotifyTag;
	static FString MissingClass;
	static FString NoActorsFound;

#if WITH_EDITOR

protected:
	virtual EDataValidationResult ValidateNode() override;
	void ValidateFlowPinArrayIsUnique(const TArray<FFlowPin>& FlowPins, TSet<FName>& InOutUniquePinNames, EDataValidationResult& InOutResult);
#endif

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
	/* Trigger execution of input pin. */
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

	UFUNCTION(BlueprintNativeEvent, Category = "FlowNode")
	bool ShouldSave();
	
//////////////////////////////////////////////////////////////////////////
// Utils

public:
#if WITH_EDITOR
	UFlowNode* GetInspectedInstance() const;

	TMap<uint8, FPinRecord> GetWireRecords() const;
	TArray<FPinRecord> GetPinRecords(const FName& PinName, const EEdGraphPinDirection PinDirection) const;
#endif

	/* Information displayed while node is working - displayed over node as NodeInfoPopup. */
	FString GetStatusStringForNodeAndAddOns() const;

#if WITH_EDITOR
	virtual bool GetStatusBackgroundColor(FLinearColor& OutColor) const;
#endif

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Get Status Background Color"))
	bool K2_GetStatusBackgroundColor(FLinearColor& OutColor) const;

#if WITH_EDITOR

public:
	virtual FString GetAssetPath();
	virtual UObject* GetAssetToEdit();
	virtual AActor* GetActorToFocus();
#endif

protected:
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
