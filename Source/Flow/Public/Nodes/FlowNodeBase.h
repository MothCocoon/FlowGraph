// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Templates/SubclassOf.h"

#include "Interfaces/FlowCoreExecutableInterface.h"
#include "Interfaces/FlowContextPinSupplierInterface.h"
#include "Interfaces/FlowDataPinValueOwnerInterface.h"
#include "FlowMessageLog.h"
#include "FlowTags.h" // used by subclasses
#include "FlowTypes.h"
#include "Types/FlowDataPinResults.h"
#include "Types/FlowPinTypeTemplates.h"

#include "FlowNodeBase.generated.h"

class UFlowAsset;
class UFlowNode;
class UFlowNodeAddOn;
class UFlowSubsystem;
class UEdGraphNode;
class IFlowDataPinValueSupplierInterface;
struct FFlowPin;
struct FFlowNamedDataPinProperty;
struct FFlowPinType;

#if WITH_EDITORONLY_DATA
DECLARE_DELEGATE(FFlowNodeEvent);
#endif

typedef TFunction<EFlowForEachAddOnFunctionReturnValue(const UFlowNodeAddOn&)> FConstFlowNodeAddOnFunction;
typedef TFunction<EFlowForEachAddOnFunctionReturnValue(UFlowNodeAddOn&)> FFlowNodeAddOnFunction;

// Supplier + PinName (in that supplier) for a Flow Data Pin value
struct FFlowPinValueSupplierData
{
	FName SupplierPinName;
	const IFlowDataPinValueSupplierInterface* PinValueSupplier = nullptr;
};

/**
 * The base class for UFlowNode and UFlowNodeAddOn, with their shared functionality
 */
UCLASS(Abstract, BlueprintType, HideCategories = Object)
class FLOW_API UFlowNodeBase
	: public UObject
	, public IFlowCoreExecutableInterface
	, public IFlowContextPinSupplierInterface
	, public IFlowDataPinValueOwnerInterface
{
	GENERATED_UCLASS_BODY()

	friend class SFlowGraphNode;
	friend class UFlowAsset;
	friend class UFlowGraphNode;
	friend class UFlowGraphSchema;

//////////////////////////////////////////////////////////////////////////
// Node

public:
	// UObject
	virtual UWorld* GetWorld() const override;
	// --

	// Dispatcher for ExecuteInput to ensure the AddOns get their ExecuteInput calls even if the node/addon
	void ExecuteInputForSelfAndAddOns(const FName& PinName);

	// IFlowCoreExecutableInterface
	virtual void InitializeInstance() override;
	virtual void DeinitializeInstance() override;

	virtual void PreloadContent() override;
	virtual void FlushContent() override;

	virtual void OnActivate() override;
	virtual void ExecuteInput(const FName& PinName) override;

	virtual void ForceFinishNode() override;
	virtual void Cleanup() override;
	// --

	// Finish execution of node, it will call Cleanup
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	virtual void Finish() PURE_VIRTUAL(Finish)

	// Simply trigger the first Output Pin, convenient to use if node has only one output
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	virtual void TriggerFirstOutput(const bool bFinish) PURE_VIRTUAL(TriggerFirstOutput)

	// Cause a specific output to be triggered (by PinName)
	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (HidePin = "ActivationType"))
	virtual void TriggerOutput(const FName PinName, const bool bFinish = false, const EFlowPinActivationType ActivationType = EFlowPinActivationType::Default) PURE_VIRTUAL(TriggerOutput)

	// TriggerOutput convenience aliases
	void TriggerOutput(const FString& PinName, const bool bFinish = false);
	void TriggerOutput(const FText& PinName, const bool bFinish = false);
	void TriggerOutput(const TCHAR* PinName, const bool bFinish = false);

	// Cause a specific output to be triggered (by PinHandle)
	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (HidePin = "ActivationType"))
	virtual void TriggerOutputPin(const FFlowOutputPinHandle Pin, const bool bFinish = false, const EFlowPinActivationType ActivationType = EFlowPinActivationType::Default);

	// Returns a random seed suitable for this flow node base
	UFUNCTION(BlueprintPure, Category = "FlowNode")
	virtual int32 GetRandomSeed() const PURE_VIRTUAL(GetRandomSeed, return 0;);

//////////////////////////////////////////////////////////////////////////
// Pins	

public:
	static const FFlowPin* FindFlowPinByName(const FName& PinName, const TArray<FFlowPin>& FlowPins);
	static FFlowPin* FindFlowPinByName(const FName& PinName, TArray<FFlowPin>& FlowPins);
	virtual bool IsSupportedInputPinName(const FName& PinName) const PURE_VIRTUAL(IsSupportedInputPinName, return true;);

#if WITH_EDITOR
public:	
	// IFlowContextPinSupplierInterface
	virtual bool SupportsContextPins() const override { return IFlowContextPinSupplierInterface::SupportsContextPins(); }
	virtual TArray<FFlowPin> GetContextInputs() const override;
	virtual TArray<FFlowPin> GetContextOutputs() const override;
	// --
#endif // WITH_EDITOR
	
//////////////////////////////////////////////////////////////////////////
// Owners

public:	
	UFUNCTION(BlueprintPure, Category = "FlowNode")
	UFlowAsset* GetFlowAsset() const;

	const UFlowNode* GetFlowNodeSelfOrOwner() const;
	virtual UFlowNode* GetFlowNodeSelfOrOwner() PURE_VIRTUAL(GetFlowNodeSelfOrOwner, return nullptr;);
	
	UFUNCTION(BlueprintPure, Category = "FlowNode")
	UFlowSubsystem* GetFlowSubsystem() const;

	// Gets the Owning Actor for this Node's RootFlow
	// (if the immediate parent is an UActorComponent, it will get that Component's actor)
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	AActor* TryGetRootFlowActorOwner() const;

	// Gets the Owning Object for this Node's RootFlow
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	UObject* TryGetRootFlowObjectOwner() const;

	static TArray<UFlowNodeBase*> BuildFlowNodeBaseAncestorChain(UFlowNodeBase& FromFlowNodeBase, bool bIncludeFromFlowNodeBase);

//////////////////////////////////////////////////////////////////////////
// AddOn support

protected:
	// Flow Node AddOn attachments
	UPROPERTY(BlueprintReadOnly, Instanced, Category = "FlowNode")
	TArray<TObjectPtr<UFlowNodeAddOn>> AddOns;

protected:
	// FlowNodes and AddOns may determine which AddOns are eligible to be their children
	// - AddOnTemplate - the template of the FlowNodeAddOn that is being considered to be added as a child
	// - AdditionalAddOnsToAssumeAreChildren - other AddOns to assume that are already child AddOns for the purposes of checking is AddOnTemplate is allowed.
	//   This list will be populated with the 'other' AddOns in a multi-paste operation in the editor,
	//   because some paste-targets can only accept a certain mix of addons, so we must know the rest of the set being pasted
	//   to make the correct decision about whether to allow AddOnTemplate to be added.
	// https://forums.unrealengine.com/t/default-parameters-with-tarrays/330225 for details on AutoCreateRefTerm
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "FlowNode", meta = (AutoCreateRefTerm = AdditionalAddOnsToAssumeAreChildren))
	EFlowAddOnAcceptResult AcceptFlowNodeAddOnChild(const UFlowNodeAddOn* AddOnTemplate, const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const;

public:
	virtual const TArray<UFlowNodeAddOn*>& GetFlowNodeAddOnChildren() const { return AddOns; }

#if WITH_EDITOR
	virtual TArray<UFlowNodeAddOn*>& GetFlowNodeAddOnChildrenByEditor() { return MutableView(AddOns); }
	EFlowAddOnAcceptResult CheckAcceptFlowNodeAddOnChild(const UFlowNodeAddOn* AddOnTemplate, const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const;
#endif // WITH_EDITOR

	bool IsClassOrImplementsInterface(const UClass& InterfaceOrClass) const
	{
		// InterfaceOrClass can either be the AddOn's UClass (or its superclass)
		// or an interface (the UClass version) that its UClass implements 
		return IsA(&InterfaceOrClass) || GetClass()->ImplementsInterface(&InterfaceOrClass);
	}

	template <typename TInterfaceOrClass>
	bool IsClassOrImplementsInterface() const
	{
		return IsClassOrImplementsInterface(*TInterfaceOrClass::StaticClass());
	}

	// Call a function for all of this object's AddOns (recursively iterating AddOns inside AddOn)
	EFlowForEachAddOnFunctionReturnValue ForEachAddOnConst(const FConstFlowNodeAddOnFunction& Function, EFlowForEachAddOnChildRule AddOnChildRule = EFlowForEachAddOnChildRule::AllChildren) const;
	EFlowForEachAddOnFunctionReturnValue ForEachAddOn(const FFlowNodeAddOnFunction& Function, EFlowForEachAddOnChildRule AddOnChildRule = EFlowForEachAddOnChildRule::AllChildren) const;

	template <typename TInterfaceOrClass, EFlowForEachAddOnChildRule TAddOnChildRule = EFlowForEachAddOnChildRule::AllChildren>
	EFlowForEachAddOnFunctionReturnValue ForEachAddOnForClassConst(const FConstFlowNodeAddOnFunction Function) const
	{
		return ForEachAddOnForClassConst(*TInterfaceOrClass::StaticClass(), Function, TAddOnChildRule);
	}

	EFlowForEachAddOnFunctionReturnValue ForEachAddOnForClassConst(const UClass& InterfaceOrClass, const FConstFlowNodeAddOnFunction& Function, EFlowForEachAddOnChildRule AddOnChildRule = EFlowForEachAddOnChildRule::AllChildren) const;

	template <typename TInterfaceOrClass, EFlowForEachAddOnChildRule TAddOnChildRule = EFlowForEachAddOnChildRule::AllChildren>
	EFlowForEachAddOnFunctionReturnValue ForEachAddOnForClass(const FFlowNodeAddOnFunction Function) const
	{
		return ForEachAddOnForClass(*TInterfaceOrClass::StaticClass(), Function, TAddOnChildRule);
	}

	EFlowForEachAddOnFunctionReturnValue ForEachAddOnForClass(const UClass& InterfaceOrClass, const FFlowNodeAddOnFunction& Function, EFlowForEachAddOnChildRule AddOnChildRule = EFlowForEachAddOnChildRule::AllChildren) const;

public:

//////////////////////////////////////////////////////////////////////////
// Data Pins

	// IFlowDataPinValueOwnerInterface
#if WITH_EDITOR
public:
	virtual bool CanModifyFlowDataPinType() const override;
	virtual bool ShowFlowDataPinValueInputPinCheckbox() const override;
	virtual bool ShowFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const override;
	virtual bool CanEditFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const override;
	virtual void SetFlowDataPinValuesRebuildDelegate(FSimpleDelegate InDelegate) override
	{
		FlowDataPinValuesRebuildDelegate = InDelegate;
	}
	virtual void RequestFlowDataPinValuesDetailsRebuild() override
	{
		if (FlowDataPinValuesRebuildDelegate.IsBound())
		{
			FlowDataPinValuesRebuildDelegate.Execute();
		}
	}
private:
	FSimpleDelegate FlowDataPinValuesRebuildDelegate;
protected:
	// Helpers for IFlowDataPinValueOwnerInterface
	bool IsPlacedInFlowAsset() const;
	bool IsFlowNamedPropertiesSupplier() const;
#endif
	// --

private:
	UFUNCTION(BlueprintPure, Category = DataPins, DisplayName = "Resolve DataPin By Name")
	FFlowDataPinResult TryResolveDataPin(FName PinName) const;
	
public:
	// Generic single-value resolve & extractor
	template <typename TFlowPinType>
	EFlowDataPinResolveResult TryResolveDataPinValue(const FName& PinName, typename TFlowPinType::ValueType& OutValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue) const;

	// Generic array-value resolve & extractor
	template <typename TFlowPinType>
	EFlowDataPinResolveResult TryResolveDataPinValues(const FName& PinName, TArray<typename TFlowPinType::ValueType>& OutValues) const;

	// Special-case single-value resolve & extractor for native enums
	template <typename TEnumType> requires std::is_enum_v<TEnumType>
	EFlowDataPinResolveResult TryResolveDataPinValue(const FName& PinName, TEnumType& OutValue, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue) const;

	// Special-case array-value resolve & extractor for native enums 
	template <typename TEnumType> requires std::is_enum_v<TEnumType>
	EFlowDataPinResolveResult TryResolveDataPinValues(const FName& PinName, TArray<TEnumType>& OutValues) const;

	// Special-case single-value resolve & extractor for enums (as FName values)
	template <typename TFlowPinType = FFlowPinType_Enum>
	EFlowDataPinResolveResult TryResolveDataPinValue(const FName& PinName, FName& OutEnumValue, UEnum*& OutEnumClass, EFlowSingleFromArray SingleFromArray = EFlowSingleFromArray::LastValue) const;

	// Special-case array-value resolve & extractor for enums (as FName values)
	template <typename TFlowPinType = FFlowPinType_Enum>
	EFlowDataPinResolveResult TryResolveDataPinValues(const FName& PinName, TArray<FName>& OutEnumValues, UEnum*& OutEnumClass) const;

public:

	// #FlowDataPinLegacy
	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Bool", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Bool TryResolveDataPinAsBool(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Int", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Int TryResolveDataPinAsInt(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Float", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Float TryResolveDataPinAsFloat(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Name", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Name TryResolveDataPinAsName(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As String", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_String TryResolveDataPinAsString(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Text", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Text TryResolveDataPinAsText(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Enum", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Enum TryResolveDataPinAsEnum(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Vector", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Vector TryResolveDataPinAsVector(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Rotator", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Rotator TryResolveDataPinAsRotator(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Transform", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Transform TryResolveDataPinAsTransform(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As GameplayTag", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_GameplayTag TryResolveDataPinAsGameplayTag(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As GameplayTagContainer", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_GameplayTagContainer TryResolveDataPinAsGameplayTagContainer(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As InstancedStruct", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_InstancedStruct TryResolveDataPinAsInstancedStruct(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Object", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Object TryResolveDataPinAsObject(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Class", meta = (DeprecatedFunction, DeprecationMessage = "Use TryResolveDataPin (in blueprint) or TryResolveDataPinValue(s) (in code) instead"))
	FFlowDataPinResult_Class TryResolveDataPinAsClass(const FName& PinName) const;
	// --

protected:
	bool TryAddValueToFormatNamedArguments(const FFlowNamedDataPinProperty& NamedDataPinProperty, FFormatNamedArguments& InOutArguments) const;

//////////////////////////////////////////////////////////////////////////
// Editor

#if WITH_EDITORONLY_DATA
protected:
	UPROPERTY()
	TObjectPtr<UEdGraphNode> GraphNode;
	
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	uint8 bDisplayNodeTitleWithoutPrefix : 1;
	
	uint8 bCanDelete : 1 ;
	uint8 bCanDuplicate : 1;
	
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	bool bNodeDeprecated;

	// If this node is deprecated, it might be replaced by another node
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode")
	TSubclassOf<UFlowNode> ReplacedBy;

	FFlowNodeEvent OnReconstructionRequested;
	FFlowNodeEvent OnAddOnRequestedParentReconstruction;
	FFlowMessageLog ValidationLog;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
public:
	virtual void PostLoad() override;
	
	void SetGraphNode(UEdGraphNode* NewGraphNode);
	UEdGraphNode* GetGraphNode() const { return GraphNode; }

	// Set up UFlowNodeBase when being opened for edit in the editor
	virtual void SetupForEditing(UEdGraphNode& EdGraphNode);

	// Opportunity to update node's data before UFlowGraphNode would call ReconstructNode()
	virtual void FixNode(UEdGraphNode* NewGraphNode);

	// UObject
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --
	
	// used when import graph from another asset
	virtual void PostImport() {}

	// Called by owning FlowNode to add to its Status String.
	// (may be multi-line)
	virtual FString GetStatusString() const;

	void RequestReconstruction() const { (void) OnReconstructionRequested.ExecuteIfBound(); };
	
	void SetCanDelete(bool CanDelete) { bCanDelete = CanDelete;}
	
#endif

protected:
	// Information displayed while node is working - displayed over node as NodeInfoPopup
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Get Status String"))
	FString K2_GetStatusString() const;

#if WITH_EDITORONLY_DATA
protected:
	UPROPERTY()
	FString Category;
	
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode", meta = (Categories = "Flow.NodeStyle"))
	FGameplayTag NodeDisplayStyle;

	// Deprecated NodeStyle, replaced by NodeDisplayStyle
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use the NodeDisplayStyle instead."))
	EFlowNodeStyle NodeStyle;

	// Set Node Style to custom to use your own color for this node (if using Flow.NodeStyle.Custom)
	UPROPERTY(EditDefaultsOnly, Category = "FlowNode", DisplayName = "Custom Node Color")
	FLinearColor NodeColor;

	// Optional developer-facing text to explain the configuration of this node when viewed in the editor
	// may be authored or set procedurally via UpdateNodeConfigText and SetNodeConfigText
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "FlowNode")
	FText DevNodeConfigText;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
public:
	// WARNING! Call UFlowGraphSettings::GetNodeCategoryForNode() instead!
	virtual FString GetNodeCategory() const;

	const FGameplayTag& GetNodeDisplayStyle() const { return NodeDisplayStyle; }

	// This method allows to have different for every node instance, i.e. Red if node represents enemy, Green if node represents a friend
	virtual bool GetDynamicTitleColor(FLinearColor& OutColor) const;

	virtual FText GetNodeTitle() const { return K2_GetNodeTitle(); }
	virtual FText GetNodeToolTip() const { return K2_GetNodeToolTip(); }

	FText GetGeneratedDisplayName() const;

protected:
	void EnsureNodeDisplayStyle();
#endif // WITH_EDITOR

public:
	UFUNCTION(BlueprintNativeEvent, Category = "FlowNode")
	FText K2_GetNodeTitle() const;
	
	UFUNCTION(BlueprintNativeEvent, Category = "FlowNode")
	FText K2_GetNodeToolTip() const;

	UFUNCTION(BlueprintPure, Category = "FlowNode")
	virtual FText GetNodeConfigText() const;

protected:	
	// Set the editor-only Config Text 
	// (for displaying config info on the Node in the flow graph, ignored in non-editor builds)
	UFUNCTION(BlueprintCallable, Category = "FlowNode")
	void SetNodeConfigText(const FText& NodeConfigText);

	// Called whenever a property change event occurs on this flow node object,
	// giving the implementor a chance to update their NodeConfigText (via SetNodeConfigText)
	UFUNCTION(BlueprintNativeEvent, Category = "FlowNode")
	void UpdateNodeConfigText();

//////////////////////////////////////////////////////////////////////////
// Debug support
	
#if WITH_EDITOR
public:
	// Short summary of node's content - displayed over node as NodeInfoPopup
	virtual FString GetNodeDescription() const;
#endif

protected:	
	// Short summary of node's content - displayed over node as NodeInfoPopup
	UFUNCTION(BlueprintImplementableEvent, Category = "FlowNode", meta = (DisplayName = "Get Node Description"))
	FString K2_GetNodeDescription() const;

public:
	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (DevelopmentOnly))
	void LogError(FString Message, const EFlowOnScreenMessageType OnScreenMessageType = EFlowOnScreenMessageType::Permanent) const;

	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (DevelopmentOnly))
	void LogWarning(FString Message) const;

	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (DevelopmentOnly))
	void LogNote(FString Message) const;

	UFUNCTION(BlueprintCallable, Category = "FlowNode", meta = (DevelopmentOnly))
	void LogVerbose(FString Message) const;

#if !UE_BUILD_SHIPPING
protected:
	bool BuildMessage(FString& Message) const;
#endif
};

// Templates & inline implementations:

template <typename TFlowPinType>
EFlowDataPinResolveResult UFlowNodeBase::TryResolveDataPinValue(const FName& PinName, typename TFlowPinType::ValueType& OutValue, EFlowSingleFromArray SingleFromArray /*= EFlowSingleFromArray::LastValue*/) const
{
	const FFlowDataPinResult DataPinResult = TryResolveDataPin(PinName);
	return FlowPinType::TryExtractValue<TFlowPinType>(DataPinResult, OutValue, SingleFromArray);
}

template <typename TFlowPinType>
EFlowDataPinResolveResult UFlowNodeBase::TryResolveDataPinValues(const FName& PinName, TArray<typename TFlowPinType::ValueType>& OutValues) const
{
	const FFlowDataPinResult DataPinResult = TryResolveDataPin(PinName);
	return FlowPinType::TryExtractValues<TFlowPinType>(DataPinResult, OutValues);
}

template <typename TFlowPinType>
EFlowDataPinResolveResult UFlowNodeBase::TryResolveDataPinValue(const FName& PinName, FName& OutEnumValue, UEnum*& OutEnumClass, EFlowSingleFromArray SingleFromArray /*= EFlowSingleFromArray::LastValue*/) const
{
	const FFlowDataPinResult DataPinResult = TryResolveDataPin(PinName);
	if (!FlowPinType::IsSuccess(DataPinResult.Result))
	{
		return DataPinResult.Result;
	}

	return FlowPinType::TryExtractValue<TFlowPinType>(DataPinResult, OutEnumValue, OutEnumClass, SingleFromArray);
}

template <typename TFlowPinType>
EFlowDataPinResolveResult UFlowNodeBase::TryResolveDataPinValues(const FName& PinName, TArray<FName>& OutEnumValues, UEnum*& OutEnumClass) const
{
	const FFlowDataPinResult DataPinResult = TryResolveDataPin(PinName);
	if (!FlowPinType::IsSuccess(DataPinResult.Result))
	{
		return DataPinResult.Result;
	}

	return FlowPinType::TryExtractValues<TFlowPinType>(DataPinResult, OutEnumValues, OutEnumClass);
}

template <typename TEnumType> requires std::is_enum_v<TEnumType>
EFlowDataPinResolveResult UFlowNodeBase::TryResolveDataPinValue(const FName& PinName, TEnumType& OutValue, EFlowSingleFromArray SingleFromArray /*= EFlowSingleFromArray::LastValue*/) const
{
	const FFlowDataPinResult DataPinResult = TryResolveDataPin(PinName);
	return FlowPinType::TryExtractValue<TEnumType>(DataPinResult, OutValue, SingleFromArray);
}

template <typename TEnumType> requires std::is_enum_v<TEnumType>
EFlowDataPinResolveResult UFlowNodeBase::TryResolveDataPinValues(const FName& PinName, TArray<TEnumType>& OutValues) const
{
	const FFlowDataPinResult DataPinResult = TryResolveDataPin(PinName);
	return FlowPinType::TryExtractValues<TEnumType>(DataPinResult, OutValues);
}
