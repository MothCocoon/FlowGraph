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
#include "Types/FlowDataPinProperties.h"

#include "FlowNode.generated.h"

/**
 * A Flow Node is UObject-based node designed to handle entire gameplay feature within single node.
 */
UCLASS(Abstract, Blueprintable, HideCategories = Object)
class FLOW_API UFlowNode 
	: public UFlowNodeBase
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
#if WITH_EDITOR
	// UObject	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
	// --

	virtual EDataValidationResult ValidateNode() { return EDataValidationResult::NotValidated; }

#endif

	// Inherits Guid after graph node
	UPROPERTY()
	FGuid NodeGuid;

public:
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void SetGuid(const FGuid& NewGuid) { NodeGuid = NewGuid; }

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	const FGuid& GetGuid() const { return NodeGuid; }

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

	// Functions to determine acceptance for 'wildcard' data pin types (eg., singular, array, set, map)
	// TODO (gtaylor) The data pins feature is under construction
	bool DoesInputWildcardPinAcceptArray(const UEdGraphPin* Pin) const { return true; }
	bool DoesOutputWildcardPinAcceptContainer(const UEdGraphPin* Pin) const { return true; }
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
	void SetConnections(const TMap<FName, FConnectedPin>& InConnections) { Connections = InConnections; }
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

	static void RecursiveFindNodesByClass(UFlowNode* Node, const TSubclassOf<UFlowNode> Class, uint8 Depth, TArray<UFlowNode*>& OutNodes);

protected:

	// Slow and fast lookup functions, based on whether we are proactively caching the connections for quick lookup 
	// in the Connections array (by PinCategory)
	bool FindConnectedNodeForPinFast(const FName& FlowPinName, FGuid* FoundGuid = nullptr, FName* OutConnectedPinName = nullptr) const;
	bool FindConnectedNodeForPinSlow(const FName& FlowPinName, FGuid* FoundGuid = nullptr, FName* OutConnectedPinName = nullptr) const;

//////////////////////////////////////////////////////////////////////////
// Data Pins

public:

	// Map of DataPin Name to its Bound Property, 
	// when using metadata tag 'BindOutputFlowDataPin' to bind properties to data pins for automatic supplier support
	UPROPERTY(VisibleDefaultsOnly, AdvancedDisplay, Category = "FlowNode", meta = (GetByRef))
	TMap<FName, FName> PinNameToBoundPropertyNameMap;

	const TMap<FName, FName>& GetPinNameToBoundPropertyNameMap() const { return PinNameToBoundPropertyNameMap; }

#if WITH_EDITORONLY_DATA	
	UPROPERTY(VisibleDefaultsOnly, AdvancedDisplay, Category = "FlowNode", meta = (GetByRef))
	TArray<FFlowPin> AutoInputDataPins;

	UPROPERTY(VisibleDefaultsOnly, AdvancedDisplay, Category = "FlowNode", meta = (GetByRef))
	TArray<FFlowPin> AutoOutputDataPins;
#endif // WITH_EDITORONLY_DATA	

#if WITH_EDITOR
	void SetPinNameToBoundPropertyNameMap(const TMap<FName, FName>& Map);
	TMap<FName, FName>& GetMutablePinNameToBoundPropertyNameMap() { return PinNameToBoundPropertyNameMap; }

	void SetAutoInputDataPins(const TArray<FFlowPin>& AutoInputPins);
	void SetAutoOutputDataPins(const TArray<FFlowPin>& AutoOutputPins);
	const TArray<FFlowPin>& GetAutoInputDataPins() const { return AutoInputDataPins; }
	const TArray<FFlowPin>& GetAutoOutputDataPins() const { return AutoOutputDataPins; }
	
	TArray<FFlowPin>& GetMutableAutoInputDataPins() { return AutoInputDataPins; }
	TArray<FFlowPin>& GetMutableAutoOutputDataPins() { return AutoOutputDataPins; }
#endif // WITH_EDITOR

	// IFlowDataPinValueSupplierInterface
	virtual bool CanSupplyDataPinValues_Implementation() const override;
	virtual FFlowDataPinResult_Bool TrySupplyDataPinAsBool_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Int TrySupplyDataPinAsInt_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Float TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Name TrySupplyDataPinAsName_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_String TrySupplyDataPinAsString_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Text TrySupplyDataPinAsText_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Enum TrySupplyDataPinAsEnum_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Vector TrySupplyDataPinAsVector_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Rotator TrySupplyDataPinAsRotator_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Transform TrySupplyDataPinAsTransform_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_GameplayTag TrySupplyDataPinAsGameplayTag_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_GameplayTagContainer TrySupplyDataPinAsGameplayTagContainer_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_InstancedStruct TrySupplyDataPinAsInstancedStruct_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Object TrySupplyDataPinAsObject_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Class TrySupplyDataPinAsClass_Implementation(const FName& PinName) const override;

	bool TryGetFlowDataPinSupplierDatasForPinName(
		const FName& PinName,
		TArray<FFlowPinValueSupplierData>& InOutPinValueSupplierDatas) const;
	// --

protected:

	// Helper functions for the TrySupplyDataPin...() functions
	bool TryFindPropertyByPinName(
		const FName& PinName,
		const FProperty*& OutFoundProperty,
		TInstancedStruct<FFlowDataPinProperty>& OutFoundInstancedStruct,
		EFlowDataPinResolveResult& InOutResult) const;
	virtual bool TryFindPropertyByRemappedPinName(
		const FName& RemappedPinName,
		const FProperty*& OutFoundProperty,
		TInstancedStruct<FFlowDataPinProperty>& OutFoundInstancedStruct,
		EFlowDataPinResolveResult& InOutResult) const;

	// Functions to supply the pin data value from a variety of supported property types
	template <typename TFlowDataPinResultType, typename TFlowDataPinProperty, typename TFieldPropertyType>
	TFlowDataPinResultType TrySupplyDataPinAsType(const FName& PinName) const;

	template <typename TFlowDataPinResultType, typename TFlowLargeDataPinProperty, typename TFlowMediumDataPinProperty>
	TFlowDataPinResultType TrySupplyDataPinAsNumericType(const FName& PinName) const;

	template <typename TFlowDataPinResultType>
	TFlowDataPinResultType TrySupplyDataPinAsAnyTextType(const FName& PinName) const;

	FORCEINLINE_DEBUGGABLE FFlowDataPinResult_Enum TrySupplyDataPinAsEnumType(const FName& PinName) const;

	template <typename TFlowDataPinResultType, typename TFlowDataPinProperty, typename TTargetStruct>
	TFlowDataPinResultType TrySupplyDataPinAsStructType(const FName& PinName) const;

	template <typename TFlowDataPinResultType, typename TFlowDataPinProperty, typename TUObjectType,
		typename TFieldPropertyObjectType0, typename TFieldPropertySoftObjectType1>
	TFlowDataPinResultType TrySupplyDataPinAsUObjectTypeCommon(const FName& PinName, const FProperty*& OutFoundProperty) const;

	template <typename TFlowDataPinResultType, typename TFlowDataPinProperty, typename TUObjectType,
		typename TFieldPropertyObjectType0, typename TFieldPropertySoftObjectType1,
		typename TFieldPropertyWeakType2, typename TFieldPropertyLazyType3>
	TFlowDataPinResultType TrySupplyDataPinAsUObjectType(const FName& PinName) const;

	template <typename TFlowDataPinResultType, typename TFlowDataPinProperty, typename TUObjectType,
		typename TFieldPropertyObjectType0, typename TFieldPropertySoftObjectType1>
	TFlowDataPinResultType TrySupplyDataPinAsUClassType(const FName& PinName) const;

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

#if !UE_BUILD_SHIPPING

private:
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

	virtual void TriggerFirstOutput(const bool bFinish) override;
	virtual void TriggerOutput(FName PinName, const bool bFinish = false, const EFlowPinActivationType ActivationType = EFlowPinActivationType::Default) override;
public:
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

// Templates & inline implementations:

template <typename TFlowDataPinResultType, typename TFlowDataPinProperty, typename TFieldPropertyType>
TFlowDataPinResultType UFlowNode::TrySupplyDataPinAsType(const FName& PinName) const
{
	TFlowDataPinResultType SuppliedResult;

	const FProperty* FoundProperty = nullptr;
	TInstancedStruct<FFlowDataPinProperty> InstancedStruct;
	if (!TryFindPropertyByPinName(PinName, FoundProperty, InstancedStruct, SuppliedResult.Result))
	{
		return SuppliedResult;
	}

	if (const TFlowDataPinProperty* FlowDataPinProp = InstancedStruct.GetPtr<TFlowDataPinProperty>())
	{
		// In some cases, TryFindPropertyByPinName can find an instanced struct for the wrapper,
		// so get the value from it and return straight away

		SuppliedResult.Value = FlowDataPinProp->Value;
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}

	// Check for struct-based wrapper for the property and get the value out of it
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(FoundProperty))
	{
		const UScriptStruct* FlowDataPinPropertyStruct = TFlowDataPinProperty::StaticStruct();

		if (StructProperty->Struct == FlowDataPinPropertyStruct)
		{
			TFlowDataPinProperty ValueStruct;
			StructProperty->GetValue_InContainer(this, &ValueStruct);

			SuppliedResult.Value = ValueStruct.Value;
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;
		}

		return SuppliedResult;
	}

	// Get the value from a UE simple property type
	if (const TFieldPropertyType* UnrealProperty = CastField<TFieldPropertyType>(FoundProperty))
	{
		SuppliedResult.Value = UnrealProperty->GetPropertyValue_InContainer(this);
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}

	SuppliedResult.Result = EFlowDataPinResolveResult::FailedMismatchedType;

	return SuppliedResult;
}

template <typename TFlowDataPinResultType, typename TFlowLargeDataPinProperty, typename TFlowMediumDataPinProperty>
TFlowDataPinResultType UFlowNode::TrySupplyDataPinAsNumericType(const FName& PinName) const
{
	TFlowDataPinResultType SuppliedResult;

	const FProperty* FoundProperty = nullptr;
	TInstancedStruct<FFlowDataPinProperty> InstancedStruct;
	if (!TryFindPropertyByPinName(PinName, FoundProperty, InstancedStruct, SuppliedResult.Result))
	{
		return SuppliedResult;
	}

	if (const FFlowDataPinProperty* FlowDataPinProp = InstancedStruct.GetPtr<FFlowDataPinProperty>())
	{
		// In some cases, TryFindPropertyByPinName can find an instanced struct for the wrapper,
		// so get the value from it and return straight away

		if (const TFlowLargeDataPinProperty* FlowDataPinPropLarge = InstancedStruct.GetPtr<TFlowLargeDataPinProperty>())
		{
			SuppliedResult.Value = FlowDataPinPropLarge->Value;
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;

			return SuppliedResult;
		}
		else if (const TFlowMediumDataPinProperty* FlowDataPinPropMedium = InstancedStruct.GetPtr<TFlowMediumDataPinProperty>())
		{
			SuppliedResult.Value = FlowDataPinPropMedium->Value;
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;

			return SuppliedResult;
		}
	}

	// Check for struct-based wrapper for the property and get the value out of it
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(FoundProperty))
	{
		const UScriptStruct* FlowLargeDataPinPropertyStruct = TFlowLargeDataPinProperty::StaticStruct();
		const UScriptStruct* FlowMediumDataPinPropertyStruct = TFlowMediumDataPinProperty::StaticStruct();

		// Supporting both a 64 and 32 bit wrapper for ints/floats, given the ubiquity of int32/float.
		if (StructProperty->Struct == FlowLargeDataPinPropertyStruct)
		{
			TFlowLargeDataPinProperty ValueStruct;
			StructProperty->GetValue_InContainer(this, &ValueStruct);

			SuppliedResult.Value = ValueStruct.Value;
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;
		}
		else if (StructProperty->Struct == FlowMediumDataPinPropertyStruct)
		{
			TFlowMediumDataPinProperty ValueStruct;
			StructProperty->GetValue_InContainer(this, &ValueStruct);

			SuppliedResult.Value = ValueStruct.Value;
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;
		}

		return SuppliedResult;
	}

	// Get the value from a UE simple property type
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(FoundProperty))
	{
		if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(NumericProperty))
		{
			float FloatValue;
			FloatProperty->GetValue_InContainer(this, &FloatValue);
			SuppliedResult.Value = FloatValue;
		}
		else if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(NumericProperty))
		{
			double DoubleValue;
			DoubleProperty->GetValue_InContainer(this, &DoubleValue);
			SuppliedResult.Value = DoubleValue;
		}
		else
		{
			SuppliedResult.Value = NumericProperty->GetSignedIntPropertyValue_InContainer(this);
		}

		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}

	SuppliedResult.Result = EFlowDataPinResolveResult::FailedMismatchedType;

	return SuppliedResult;
}

template <typename TFlowDataPinResultType>
TFlowDataPinResultType UFlowNode::TrySupplyDataPinAsAnyTextType(const FName& PinName) const
{
	TFlowDataPinResultType SuppliedResult;

	const FProperty* FoundProperty = nullptr;
	TInstancedStruct<FFlowDataPinProperty> InstancedStruct;
	if (!TryFindPropertyByPinName(PinName, FoundProperty, InstancedStruct, SuppliedResult.Result))
	{
		return SuppliedResult;
	}

	if (const FFlowDataPinProperty* FlowDataPinProp = InstancedStruct.GetPtr<FFlowDataPinProperty>())
	{
		// In some cases, TryFindPropertyByPinName can find an instanced struct for the wrapper,
		// so get the value from it and return straight away

		if (const FFlowDataPinOutputProperty_Name* FlowDataPinPropName = InstancedStruct.GetPtr<FFlowDataPinOutputProperty_Name>())
		{
			SuppliedResult.SetValue(FlowDataPinPropName->Value);
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;

			return SuppliedResult;
		}
		else if (const FFlowDataPinOutputProperty_String* FlowDataPinPropString = InstancedStruct.GetPtr<FFlowDataPinOutputProperty_String>())
		{
			SuppliedResult.SetValue(FlowDataPinPropString->Value);
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;

			return SuppliedResult;
		}
		else if (const FFlowDataPinOutputProperty_Text* FlowDataPinPropText = InstancedStruct.GetPtr<FFlowDataPinOutputProperty_Text>())
		{
			SuppliedResult.SetValue(FlowDataPinPropText->Value);
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;

			return SuppliedResult;
		}
	}

	// Check for struct-based wrapper for the property and get the value out of it
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(FoundProperty))
	{
		const UScriptStruct* FlowDataPinPropertyStruct_Name = FFlowDataPinOutputProperty_Name::StaticStruct();
		const UScriptStruct* FlowDataPinPropertyStruct_String = FFlowDataPinOutputProperty_String::StaticStruct();
		const UScriptStruct* FlowDataPinPropertyStruct_Text = FFlowDataPinOutputProperty_Text::StaticStruct();

		if (StructProperty->Struct == FlowDataPinPropertyStruct_Name)
		{
			FFlowDataPinOutputProperty_Name ValueStruct;
			StructProperty->GetValue_InContainer(this, &ValueStruct);

			SuppliedResult.SetValue(ValueStruct.Value);
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;
		}
		else if (StructProperty->Struct == FlowDataPinPropertyStruct_String)
		{
			FFlowDataPinOutputProperty_String ValueStruct;
			StructProperty->GetValue_InContainer(this, &ValueStruct);

			SuppliedResult.SetValue(ValueStruct.Value);
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;
		}
		else if (StructProperty->Struct == FlowDataPinPropertyStruct_Text)
		{
			FFlowDataPinOutputProperty_Text ValueStruct;
			StructProperty->GetValue_InContainer(this, &ValueStruct);

			SuppliedResult.SetValue(ValueStruct.Value);
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;
		}

		return SuppliedResult;
	}
	
	// Get the value from a UE simple property type
	if (const FNameProperty* NameProperty = CastField<FNameProperty>(FoundProperty))
	{
		SuppliedResult.SetValue(NameProperty->GetPropertyValue_InContainer(this));
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}
	else if (const FStrProperty* StrProperty = CastField<FStrProperty>(FoundProperty))
	{
		SuppliedResult.SetValue(StrProperty->GetPropertyValue_InContainer(this));
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}
	else if (const FTextProperty* TextProperty = CastField<FTextProperty>(FoundProperty))
	{
		SuppliedResult.SetValue(TextProperty->GetPropertyValue_InContainer(this));
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}
	else
	{
		SuppliedResult.Result = EFlowDataPinResolveResult::FailedMismatchedType;

		return SuppliedResult;
	}
}

FFlowDataPinResult_Enum UFlowNode::TrySupplyDataPinAsEnumType(const FName& PinName) const
{
	FFlowDataPinResult_Enum SuppliedResult;

	const FProperty* FoundProperty = nullptr;
	TInstancedStruct<FFlowDataPinProperty> InstancedStruct;
	if (!TryFindPropertyByPinName(PinName, FoundProperty, InstancedStruct, SuppliedResult.Result))
	{
		return SuppliedResult;
	}

	if (const FFlowDataPinOutputProperty_Enum* FlowDataPinProp = InstancedStruct.GetPtr<FFlowDataPinOutputProperty_Enum>())
	{
		// In some cases, TryFindPropertyByPinName can find an instanced struct for the wrapper,
		// so get the value from it and return straight away

		SuppliedResult.Value = FlowDataPinProp->Value;
		SuppliedResult.EnumClass = FlowDataPinProp->EnumClass;
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}

	// Check for struct-based wrapper for the property and get the value out of it
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(FoundProperty))
	{
		const UScriptStruct* FlowDataPinPropertyStruct_Enum = FFlowDataPinOutputProperty_Enum::StaticStruct();

		if (StructProperty->Struct == FlowDataPinPropertyStruct_Enum)
		{
			FFlowDataPinOutputProperty_Enum ValueStruct;
			StructProperty->GetValue_InContainer(this, &ValueStruct);

			SuppliedResult.Value = ValueStruct.Value;
			SuppliedResult.EnumClass = ValueStruct.EnumClass;
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;
		}

		return SuppliedResult;
	}

	// Get the value from a UE enum property type
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(FoundProperty))
	{
		UEnum* EnumClass = EnumProperty->GetEnum();

		const FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
		const int64 SignedIntValue = UnderlyingProperty->GetSignedIntPropertyValue_InContainer(this);
		const FString StringValue = EnumClass->GetAuthoredNameStringByValue(SignedIntValue);

		SuppliedResult.Value = FName(StringValue);
		SuppliedResult.EnumClass = EnumClass;
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}
	else
	{
		SuppliedResult.Result = EFlowDataPinResolveResult::FailedMismatchedType;

		return SuppliedResult;
	}
}

template <typename TFlowDataPinResultType, typename TFlowDataPinProperty, typename TTargetStruct>
TFlowDataPinResultType UFlowNode::TrySupplyDataPinAsStructType(const FName& PinName) const
{
	TFlowDataPinResultType SuppliedResult;

	const FProperty* FoundProperty = nullptr;
	TInstancedStruct<FFlowDataPinProperty> InstancedStruct;
	if (!TryFindPropertyByPinName(PinName, FoundProperty, InstancedStruct, SuppliedResult.Result))
	{
		return SuppliedResult;
	}

	if (const TFlowDataPinProperty* FlowDataPinProp = InstancedStruct.GetPtr<TFlowDataPinProperty>())
	{
		// In some cases, TryFindPropertyByPinName can find an instanced struct for the wrapper,
		// so get the value from it and return straight away

		SuppliedResult.Value = FlowDataPinProp->Value;
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}

	const FStructProperty* StructProperty = CastField<FStructProperty>(FoundProperty);
	if (!StructProperty)
	{
		SuppliedResult.Result = EFlowDataPinResolveResult::FailedMismatchedType;

		return SuppliedResult;
	}

	const UScriptStruct* FlowDataPinPropertyStruct = TFlowDataPinProperty::StaticStruct();
	static const UScriptStruct* TargetPropertyStruct = TBaseStructure<TTargetStruct>::Get();

	if (StructProperty->Struct == FlowDataPinPropertyStruct)
	{
		// Check for struct-based wrapper for the property and get the value out of it

		TFlowDataPinProperty ValueStruct;
		StructProperty->GetValue_InContainer(this, &ValueStruct);

		SuppliedResult.Value = ValueStruct.Value;
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}
	else if (StructProperty->Struct == TargetPropertyStruct)
	{
		// Get the value from a UE struct (non-wrapper) property type

		TTargetStruct TargetStruct;
		StructProperty->GetValue_InContainer(this, &TargetStruct);

		SuppliedResult.Value = TargetStruct;
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}
	else
	{
		SuppliedResult.Result = EFlowDataPinResolveResult::FailedMismatchedType;

		return SuppliedResult;
	}
}

template <typename TFlowDataPinResultType, typename TFlowDataPinProperty, typename TUObjectType,
	typename TFieldPropertyObjectType0, typename TFieldPropertySoftObjectType1>
TFlowDataPinResultType UFlowNode::TrySupplyDataPinAsUObjectTypeCommon(const FName& PinName, const FProperty*& OutFoundProperty) const
{
	TFlowDataPinResultType SuppliedResult;

	TInstancedStruct<FFlowDataPinProperty> InstancedStruct;
	if (!TryFindPropertyByPinName(PinName, OutFoundProperty, InstancedStruct, SuppliedResult.Result))
	{
		return SuppliedResult;
	}

	if (const TFlowDataPinProperty* FlowDataPinProp = InstancedStruct.GetPtr<TFlowDataPinProperty>())
	{
		// In some cases, TryFindPropertyByPinName can find an instanced struct for the wrapper,
		// so get the value from it and return straight away

		SuppliedResult.SetValueFromPropertyWrapper(*FlowDataPinProp);
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}

	// Check for struct-based wrapper for the property and get the value out of it
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(OutFoundProperty))
	{
		const UScriptStruct* FlowDataPinPropertyStruct = TFlowDataPinProperty::StaticStruct();

		if (StructProperty->Struct == FlowDataPinPropertyStruct)
		{
			TFlowDataPinProperty ValueStruct;
			StructProperty->GetValue_InContainer(this, &ValueStruct);

			SuppliedResult.SetValueFromPropertyWrapper(ValueStruct);
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;
		}

		return SuppliedResult;
	}

	// Get the value from one of the UE simple property types
	if (const TFieldPropertyObjectType0* UnrealProperty0 = CastField<TFieldPropertyObjectType0>(OutFoundProperty))
	{
		// TObjectPtr / UObject*
		TUObjectType* Object = Cast<TUObjectType>(UnrealProperty0->GetPropertyValue_InContainer(this));
		SuppliedResult.SetValueFromObjectPtr(Object);
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}

	if (const TFieldPropertySoftObjectType1* UnrealProperty1 = CastField<TFieldPropertySoftObjectType1>(OutFoundProperty))
	{
		// FSoftObjectPath / TSoftObjectPtr (or their Class variants)
		const FSoftObjectPath SoftObjectPath = UnrealProperty1->GetPropertyValue_InContainer(this).ToSoftObjectPath();
		SuppliedResult.SetValueFromSoftPath(SoftObjectPath);
		SuppliedResult.Result = EFlowDataPinResolveResult::Success;

		return SuppliedResult;
	}

	SuppliedResult.Result = EFlowDataPinResolveResult::FailedMismatchedType;

	return SuppliedResult;
}

template <typename TFlowDataPinResultType, typename TFlowDataPinProperty, typename TUObjectType,
		  typename TFieldPropertyObjectType0, typename TFieldPropertySoftObjectType1, typename TFieldPropertyWeakType2, typename TFieldPropertyLazyType3>
TFlowDataPinResultType UFlowNode::TrySupplyDataPinAsUObjectType(const FName& PinName) const
{
	// First execute TrySupplyDataPinAsUObjectTypeCommon to handle all of the shared cases between UObject and UClass properties:
	const FProperty* FoundProperty = nullptr;
	TFlowDataPinResultType SuppliedResult = 
		TrySupplyDataPinAsUObjectTypeCommon<TFlowDataPinResultType, TFlowDataPinProperty, TUObjectType, TFieldPropertyObjectType0, TFieldPropertySoftObjectType1>(PinName, FoundProperty);

	if (SuppliedResult.Result == EFlowDataPinResolveResult::FailedMismatchedType)
	{
		if (const TFieldPropertyWeakType2* UnrealProperty2 = CastField<TFieldPropertyWeakType2>(FoundProperty))
		{
			// TWeakObjectPtr
			TUObjectType* Object = Cast<TUObjectType>(UnrealProperty2->GetPropertyValue_InContainer(this).Get());
			SuppliedResult.SetValueFromObjectPtr(Object);
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;

			return SuppliedResult;
		}

		if (const TFieldPropertyLazyType3* UnrealProperty3 = CastField<TFieldPropertyLazyType3>(FoundProperty))
		{
			// FLazyObjectPtr
			TUObjectType* Object = Cast<TUObjectType>(UnrealProperty3->GetPropertyValue_InContainer(this).Get());
			SuppliedResult.SetValueFromObjectPtr(Object);
			SuppliedResult.Result = EFlowDataPinResolveResult::Success;

			return SuppliedResult;
		}
	}

	return SuppliedResult;
}

template <typename TFlowDataPinResultType, typename TFlowDataPinProperty, typename TUObjectType,
	typename TFieldPropertyObjectType0, typename TFieldPropertySoftObjectType1>
TFlowDataPinResultType UFlowNode::TrySupplyDataPinAsUClassType(const FName& PinName) const
{
	const FProperty* FoundProperty = nullptr;
	return TrySupplyDataPinAsUObjectTypeCommon<TFlowDataPinResultType, TFlowDataPinProperty, TUObjectType, TFieldPropertyObjectType0, TFieldPropertySoftObjectType1>(PinName, FoundProperty);
}
