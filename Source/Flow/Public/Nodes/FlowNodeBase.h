// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Templates/SubclassOf.h"

#include "Interfaces/FlowCoreExecutableInterface.h"
#include "Interfaces/FlowContextPinSupplierInterface.h"
#include "FlowMessageLog.h"
#include "FlowTags.h"
#include "FlowTypes.h"
#include "Types/FlowDataPinResults.h"
#include "NativeGameplayTags.h"

#include "FlowNodeBase.generated.h"

class UFlowAsset;
class UFlowNode;
class UFlowNodeAddOn;
class UFlowSubsystem;
class UEdGraphNode;
class IFlowOwnerInterface;
class IFlowDataPinValueSupplierInterface;
struct FFlowPin;

#if WITH_EDITOR
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

// Helper template to reduce (some) of the boilerplate in TryResolveDataPinAs...() functions
template <typename TFlowDataPinResultType, EFlowPinType PinType>
struct TResolveDataPinWorkingData
{
	bool TrySetupWorkingData(const FName& PinName, const UFlowNodeBase& FlowNodeBase);

	TFlowDataPinResultType DataPinResult;
	const UFlowNode* FlowNode = nullptr;
	const FFlowPin* FlowPin = nullptr;
	
	TArray<FFlowPinValueSupplierData> PinValueSupplierDatas;

	static constexpr bool bCheckDefaultProperties = true;
};

/**
 * The base class for UFlowNode and UFlowNodeAddOn, with their shared functionality
 */
UCLASS(Abstract, BlueprintType, HideCategories = Object)
class FLOW_API UFlowNodeBase
	: public UObject
	, public IFlowCoreExecutableInterface
	, public IFlowContextPinSupplierInterface
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

	// Returns the IFlowOwnerInterface for the owner object (if implemented)
	//  NOTE - will consider a UActorComponent owner's owning actor if appropriate
	IFlowOwnerInterface* GetFlowOwnerInterface() const;

protected:
	// Helper functions for GetFlowOwnerInterface()
	static IFlowOwnerInterface* TryGetFlowOwnerInterfaceFromRootFlowOwner(UObject& RootFlowOwner, const UClass& ExpectedOwnerClass);
	static IFlowOwnerInterface* TryGetFlowOwnerInterfaceActor(UObject& RootFlowOwner, const UClass& ExpectedOwnerClass);

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

	// Call a function for all of this object's AddOns (recursively iterating AddOns inside AddOn)
	EFlowForEachAddOnFunctionReturnValue ForEachAddOnConst(const FConstFlowNodeAddOnFunction& Function) const;
	EFlowForEachAddOnFunctionReturnValue ForEachAddOn(const FFlowNodeAddOnFunction& Function) const;

	template <typename TInterfaceOrClass>
	EFlowForEachAddOnFunctionReturnValue ForEachAddOnForClassConst(const FConstFlowNodeAddOnFunction Function) const
	{
		return ForEachAddOnForClassConst(*TInterfaceOrClass::StaticClass(), Function);
	}

	EFlowForEachAddOnFunctionReturnValue ForEachAddOnForClassConst(const UClass& InterfaceOrClass, const FConstFlowNodeAddOnFunction& Function) const;

	template <typename TInterfaceOrClass>
	EFlowForEachAddOnFunctionReturnValue ForEachAddOnForClass(const FFlowNodeAddOnFunction Function) const
	{
		return ForEachAddOnForClass(*TInterfaceOrClass::StaticClass(), Function);
	}

	EFlowForEachAddOnFunctionReturnValue ForEachAddOnForClass(const UClass& InterfaceOrClass, const FFlowNodeAddOnFunction& Function) const;

public:

//////////////////////////////////////////////////////////////////////////
// Data Pins

	// Must implement TryResolveDataAs... for every EFlowPinType
	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Bool")
	FFlowDataPinResult_Bool TryResolveDataPinAsBool(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Int")
	FFlowDataPinResult_Int TryResolveDataPinAsInt(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Float")
	FFlowDataPinResult_Float TryResolveDataPinAsFloat(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Name")
	FFlowDataPinResult_Name TryResolveDataPinAsName(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As String")
	FFlowDataPinResult_String TryResolveDataPinAsString(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Text")
	FFlowDataPinResult_Text TryResolveDataPinAsText(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Enum")
	FFlowDataPinResult_Enum TryResolveDataPinAsEnum(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Vector")
	FFlowDataPinResult_Vector TryResolveDataPinAsVector(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Rotator")
	FFlowDataPinResult_Rotator TryResolveDataPinAsRotator(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Transform")
	FFlowDataPinResult_Transform TryResolveDataPinAsTransform(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As GameplayTag")
	FFlowDataPinResult_GameplayTag TryResolveDataPinAsGameplayTag(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As GameplayTagContainer")
	FFlowDataPinResult_GameplayTagContainer TryResolveDataPinAsGameplayTagContainer(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As InstancedStruct")
	FFlowDataPinResult_InstancedStruct TryResolveDataPinAsInstancedStruct(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Object")
	FFlowDataPinResult_Object TryResolveDataPinAsObject(const FName& PinName) const;

	UFUNCTION(BlueprintCallable, Category = DataPins, DisplayName = "Try Resolve DataPin As Class")
	FFlowDataPinResult_Class TryResolveDataPinAsClass(const FName& PinName) const;

	// Public only for for TResolveDataPinWorkingData's use
	EFlowDataPinResolveResult TryResolveDataPinPrerequisites(const FName& PinName, const UFlowNode*& FlowNode, const FFlowPin*& FlowPin, EFlowPinType PinType) const;

public:

//////////////////////////////////////////////////////////////////////////
// Editor
// (some editor symbols exposed to enabled creation of non-editor tooling)

	UPROPERTY()
	TObjectPtr<UEdGraphNode> GraphNode;
	
#if WITH_EDITORONLY_DATA
protected:
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
	FFlowMessageLog ValidationLog;
#endif // WITH_EDITORONLY_DATA

public:
	UEdGraphNode* GetGraphNode() const { return GraphNode; }

	virtual void PostLoad() override;

#if WITH_EDITOR
	void SetGraphNode(UEdGraphNode* NewGraphNode);

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
	virtual FString GetNodeCategory() const;

	const FGameplayTag& GetNodeDisplayStyle() const { return NodeDisplayStyle; }

	// This method allows to have different for every node instance, i.e. Red if node represents enemy, Green if node represents a friend
	virtual bool GetDynamicTitleColor(FLinearColor& OutColor) const;
	
	virtual FText GetNodeTitle() const;
	virtual FText GetNodeToolTip() const;
	virtual FText GetNodeConfigText() const;
	FText GetGeneratedDisplayName() const;

protected:
	void EnsureNodeDisplayStyle();
#endif

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
