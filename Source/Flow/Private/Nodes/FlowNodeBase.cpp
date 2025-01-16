// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/FlowNodeBase.h"

#include "AddOns/FlowNodeAddOn.h"
#include "FlowAsset.h"
#include "FlowLogChannels.h"
#include "FlowSubsystem.h"
#include "FlowTypes.h"
#include "Interfaces/FlowDataPinValueSupplierInterface.h"

#include "Components/ActorComponent.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/ViewportStatsSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeBase)

using namespace EFlowForEachAddOnFunctionReturnValue_Classifiers;

UFlowNodeBase::UFlowNodeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, GraphNode(nullptr)
#if WITH_EDITORONLY_DATA
	, bDisplayNodeTitleWithoutPrefix(true)
	, bCanDelete(true)
	, bCanDuplicate(true)
	, bNodeDeprecated(false)
	, NodeDisplayStyle(FlowNodeStyle::Node)
	, NodeStyle(EFlowNodeStyle::Invalid)
	, NodeColor(FLinearColor::Black)
#endif
{
}

UWorld* UFlowNodeBase::GetWorld() const
{
	if (const UFlowAsset* FlowAsset = GetFlowAsset())
	{
		if (const UObject* FlowAssetOwner = FlowAsset->GetOwner())
		{
			return FlowAssetOwner->GetWorld();
		}
	}

	if (const UFlowSubsystem* FlowSubsystem = GetFlowSubsystem())
	{
		return FlowSubsystem->GetWorld();
	}

	return nullptr;
}

void UFlowNodeBase::InitializeInstance()
{
	IFlowCoreExecutableInterface::InitializeInstance();

	if (!AddOns.IsEmpty())
	{
		TArray<UFlowNodeAddOn*> SourceAddOns = AddOns;
		AddOns.Reset();

		for (UFlowNodeAddOn* SourceAddOn : SourceAddOns)
		{
			// Create a new instance of each AddOn
			if (IsValid(SourceAddOn))
			{
				UFlowNodeAddOn* NewAddOnInstance = NewObject<UFlowNodeAddOn>(this, SourceAddOn->GetClass(), NAME_None, RF_Transient, SourceAddOn, false, nullptr);
				AddOns.Add(NewAddOnInstance);
			}
			else
			{
				LogError(FString::Printf(TEXT("Null AddOn found in node %s"), *GetName()), EFlowOnScreenMessageType::Permanent);
			}
		}

		for (UFlowNodeAddOn* AddOn : AddOns)
		{
			// Initialize all the AddOn instances after they are all allocated
			AddOn->InitializeInstance();
		}
	}
}

void UFlowNodeBase::DeinitializeInstance()
{
	for (UFlowNodeAddOn* AddOn : AddOns)
	{
		AddOn->DeinitializeInstance();
	}

	IFlowCoreExecutableInterface::DeinitializeInstance();
}

void UFlowNodeBase::PreloadContent()
{
	IFlowCoreExecutableInterface::PreloadContent();

	for (UFlowNodeAddOn* AddOn : AddOns)
	{
		AddOn->PreloadContent();
	}
}

void UFlowNodeBase::FlushContent()
{
	for (UFlowNodeAddOn* AddOn : AddOns)
	{
		AddOn->FlushContent();
	}

	IFlowCoreExecutableInterface::FlushContent();
}

void UFlowNodeBase::OnActivate()
{
	IFlowCoreExecutableInterface::OnActivate();

	for (UFlowNodeAddOn* AddOn : AddOns)
	{
		AddOn->OnActivate();
	}
}

void UFlowNodeBase::ExecuteInputForSelfAndAddOns(const FName& PinName)
{
	// AddOns can introduce input pins to Nodes without the Node being aware of the addition.
	// To ensure that Nodes and AddOns only get the input pins signalled that they expect,
	// we are filtering the PinName vs. the expected InputPins before carrying on with the ExecuteInput

	if (IsSupportedInputPinName(PinName))
	{
		ExecuteInput(PinName);
	}

	for (UFlowNodeAddOn* AddOn : AddOns)
	{
		AddOn->ExecuteInputForSelfAndAddOns(PinName);
	}
}

void UFlowNodeBase::ExecuteInput(const FName& PinName)
{
	IFlowCoreExecutableInterface::ExecuteInput(PinName);
}

void UFlowNodeBase::ForceFinishNode()
{
	for (UFlowNodeAddOn* AddOn : AddOns)
	{
		AddOn->ForceFinishNode();
	}

	IFlowCoreExecutableInterface::ForceFinishNode();
}

void UFlowNodeBase::Cleanup()
{
	for (UFlowNodeAddOn* AddOn : AddOns)
	{
		AddOn->Cleanup();
	}

	IFlowCoreExecutableInterface::Cleanup();
}

void UFlowNodeBase::TriggerOutputPin(const FFlowOutputPinHandle Pin, const bool bFinish, const EFlowPinActivationType ActivationType)
{
	TriggerOutput(Pin.PinName, bFinish, ActivationType);
}

void UFlowNodeBase::TriggerOutput(const FString& PinName, const bool bFinish)
{
	TriggerOutput(FName(PinName), bFinish);
}

void UFlowNodeBase::TriggerOutput(const FText& PinName, const bool bFinish)
{
	TriggerOutput(FName(PinName.ToString()), bFinish);
}

void UFlowNodeBase::TriggerOutput(const TCHAR* PinName, const bool bFinish)
{
	TriggerOutput(FName(PinName), bFinish);
}

const FFlowPin* UFlowNodeBase::FindFlowPinByName(const FName& PinName, const TArray<FFlowPin>& FlowPins)
{
	return FlowPins.FindByPredicate([&PinName](const FFlowPin& FlowPin)
	{
		return FlowPin.PinName == PinName;
	});
}

FFlowPin* UFlowNodeBase::FindFlowPinByName(const FName& PinName, TArray<FFlowPin>& FlowPins)
{
	return FlowPins.FindByPredicate([&PinName](FFlowPin& FlowPin)
	{
		return FlowPin.PinName == PinName;
	});
}

#if WITH_EDITOR
TArray<FFlowPin> UFlowNodeBase::GetContextInputs() const
{
	TArray<FFlowPin> ContextInputs = IFlowContextPinSupplierInterface::GetContextInputs();
	TArray<FFlowPin> AddOnInputs;

	for (const UFlowNodeAddOn* AddOn : AddOns)
	{
		if (IsValid(AddOn))
		{
			AddOnInputs.Append(AddOn->GetContextInputs());
		}
	}

	if (!AddOnInputs.IsEmpty())
	{
		for (const FFlowPin& FlowPin : AddOnInputs)
		{
			ContextInputs.AddUnique(FlowPin);
		}
	}

	return ContextInputs;
}

TArray<FFlowPin> UFlowNodeBase::GetContextOutputs() const
{
	TArray<FFlowPin> ContextOutputs = IFlowContextPinSupplierInterface::GetContextOutputs();
	TArray<FFlowPin> AddOnOutputs;

	for (const UFlowNodeAddOn* AddOn : AddOns)
	{
		if (IsValid(AddOn))
		{
			AddOnOutputs.Append(AddOn->GetContextOutputs());
		}
	}

	if (!AddOnOutputs.IsEmpty())
	{
		for (const FFlowPin& FlowPin : AddOnOutputs)
		{
			ContextOutputs.AddUnique(FlowPin);
		}
	}

	return ContextOutputs;
}

FString UFlowNodeBase::GetStatusString() const
{
	return K2_GetStatusString();
}
#endif // WITH_EDITOR

UFlowAsset* UFlowNodeBase::GetFlowAsset() const
{
	// In the case of an AddOn, we want our containing FlowNode's Outer, not our own
	const UFlowNode* FlowNode = GetFlowNodeSelfOrOwner();
	return FlowNode && FlowNode->GetOuter() ? Cast<UFlowAsset>(FlowNode->GetOuter()) : Cast<UFlowAsset>(GetOuter());
}

const UFlowNode* UFlowNodeBase::GetFlowNodeSelfOrOwner() const
{
	return const_cast<UFlowNodeBase*>(this)->GetFlowNodeSelfOrOwner();
}

UFlowSubsystem* UFlowNodeBase::GetFlowSubsystem() const
{
	return GetFlowAsset() ? GetFlowAsset()->GetFlowSubsystem() : nullptr;
}

AActor* UFlowNodeBase::TryGetRootFlowActorOwner() const
{
	AActor* OwningActor = nullptr;

	UObject* RootFlowOwner = TryGetRootFlowObjectOwner();

	if (IsValid(RootFlowOwner))
	{
		// Check if the immediate parent is an AActor
		OwningActor = Cast<AActor>(RootFlowOwner);

		if (!IsValid(OwningActor))
		{
			// Check if the immediate parent is an UActorComponent and return that Component's Owning actor
			if (const UActorComponent* OwningComponent = Cast<UActorComponent>(RootFlowOwner))
			{
				OwningActor = OwningComponent->GetOwner();
			}
		}
	}

	return OwningActor;
}

UObject* UFlowNodeBase::TryGetRootFlowObjectOwner() const
{
	const UFlowAsset* FlowAsset = GetFlowAsset();

	if (IsValid(FlowAsset))
	{
		return FlowAsset->GetOwner();
	}

	return nullptr;
}

IFlowOwnerInterface* UFlowNodeBase::GetFlowOwnerInterface() const
{
	const UFlowAsset* FlowAsset = GetFlowAsset();
	if (!IsValid(FlowAsset))
	{
		return nullptr;
	}

	const UClass* ExpectedOwnerClass = FlowAsset->GetExpectedOwnerClass();
	if (!IsValid(ExpectedOwnerClass))
	{
		return nullptr;
	}

	UObject* RootFlowOwner = FlowAsset->GetOwner();
	if (!IsValid(RootFlowOwner))
	{
		return nullptr;
	}

	if (IFlowOwnerInterface* FlowOwnerInterface = TryGetFlowOwnerInterfaceFromRootFlowOwner(*RootFlowOwner, *ExpectedOwnerClass))
	{
		return FlowOwnerInterface;
	}

	if (IFlowOwnerInterface* FlowOwnerInterface = TryGetFlowOwnerInterfaceActor(*RootFlowOwner, *ExpectedOwnerClass))
	{
		return FlowOwnerInterface;
	}

	return nullptr;
}

IFlowOwnerInterface* UFlowNodeBase::TryGetFlowOwnerInterfaceFromRootFlowOwner(UObject& RootFlowOwner, const UClass& ExpectedOwnerClass)
{
	const UClass* RootFlowOwnerClass = RootFlowOwner.GetClass();
	if (!IsValid(RootFlowOwnerClass))
	{
		return nullptr;
	}

	if (!RootFlowOwnerClass->IsChildOf(&ExpectedOwnerClass))
	{
		return nullptr;
	}

	// If the immediate owner is the expected class type, return its FlowOwnerInterface
	return CastChecked<IFlowOwnerInterface>(&RootFlowOwner);
}

IFlowOwnerInterface* UFlowNodeBase::TryGetFlowOwnerInterfaceActor(UObject& RootFlowOwner, const UClass& ExpectedOwnerClass)
{
	// Special case if the immediate owner is a component, also consider the component's owning actor
	const UActorComponent* FlowComponent = Cast<UActorComponent>(&RootFlowOwner);
	if (!IsValid(FlowComponent))
	{
		return nullptr;
	}

	AActor* ActorOwner = FlowComponent->GetOwner();
	if (!IsValid(ActorOwner))
	{
		return nullptr;
	}

	const UClass* ActorOwnerClass = ActorOwner->GetClass();
	if (!ActorOwnerClass->IsChildOf(&ExpectedOwnerClass))
	{
		return nullptr;
	}

	return CastChecked<IFlowOwnerInterface>(ActorOwner);
}

EFlowAddOnAcceptResult UFlowNodeBase::AcceptFlowNodeAddOnChild_Implementation(
	const UFlowNodeAddOn* AddOnTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	// Subclasses may override this function to allow AddOn children classes
	return EFlowAddOnAcceptResult::Undetermined;
}

#if WITH_EDITOR
EFlowAddOnAcceptResult UFlowNodeBase::CheckAcceptFlowNodeAddOnChild(
	const UFlowNodeAddOn* AddOnTemplate,
	const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (!IsValid(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::Reject;
	}

	FLOW_ASSERT_ENUM_MAX(EFlowAddOnAcceptResult, 3);

	EFlowAddOnAcceptResult CombinedResult = EFlowAddOnAcceptResult::Undetermined;

	// Potential parents of AddOns are allowed to decide their eligible AddOn children
	const EFlowAddOnAcceptResult AsChildResult = AcceptFlowNodeAddOnChild(AddOnTemplate, AdditionalAddOnsToAssumeAreChildren);
	CombinedResult = CombineFlowAddOnAcceptResult(AsChildResult, CombinedResult);

	if (CombinedResult == EFlowAddOnAcceptResult::Reject)
	{
		return EFlowAddOnAcceptResult::Reject;
	}

	// FlowNodeAddOns are allowed to opt in to their parent
	const EFlowAddOnAcceptResult AsParentResult = AddOnTemplate->AcceptFlowNodeAddOnParent(this, AdditionalAddOnsToAssumeAreChildren);

	if (AsParentResult != EFlowAddOnAcceptResult::Reject &&
		AddOnTemplate->IsA<UFlowNode>())
	{
		const FString Message = FString::Printf(TEXT("%s::AcceptFlowNodeAddOnParent must always Reject for UFlowNode subclasses"), *GetClass()->GetName());
		GetFlowAsset()->GetTemplateAsset()->LogError(Message, this);

		return EFlowAddOnAcceptResult::Reject;
	}

	CombinedResult = CombineFlowAddOnAcceptResult(AsParentResult, CombinedResult);

	return CombinedResult;
}
#endif // WITH_EDITOR

EFlowForEachAddOnFunctionReturnValue UFlowNodeBase::ForEachAddOnConst(const FConstFlowNodeAddOnFunction& Function) const
{
	FLOW_ASSERT_ENUM_MAX(EFlowForEachAddOnFunctionReturnValue, 3);

	EFlowForEachAddOnFunctionReturnValue ReturnValue = EFlowForEachAddOnFunctionReturnValue::Continue;

	for (const UFlowNodeAddOn* AddOn : AddOns)
	{
		if (!IsValid(AddOn))
		{
			continue;
		}

		ReturnValue = Function(*AddOn);

		if (!ShouldContinueForEach(ReturnValue))
		{
			break;
		}

		ReturnValue = AddOn->ForEachAddOnConst(Function);

		if (!ShouldContinueForEach(ReturnValue))
		{
			break;
		}
	}

	return ReturnValue;
}

EFlowForEachAddOnFunctionReturnValue UFlowNodeBase::ForEachAddOn(const FFlowNodeAddOnFunction& Function) const
{
	FLOW_ASSERT_ENUM_MAX(EFlowForEachAddOnFunctionReturnValue, 3);

	EFlowForEachAddOnFunctionReturnValue ReturnValue = EFlowForEachAddOnFunctionReturnValue::Continue;

	for (UFlowNodeAddOn* AddOn : AddOns)
	{
		if (!IsValid(AddOn))
		{
			continue;
		}

		ReturnValue = Function(*AddOn);

		if (!ShouldContinueForEach(ReturnValue))
		{
			break;
		}

		ReturnValue = AddOn->ForEachAddOn(Function);

		if (!ShouldContinueForEach(ReturnValue))
		{
			break;
		}
	}

	return ReturnValue;
}

EFlowForEachAddOnFunctionReturnValue UFlowNodeBase::ForEachAddOnForClassConst(const UClass& InterfaceOrClass, const FConstFlowNodeAddOnFunction& Function) const
{
	FLOW_ASSERT_ENUM_MAX(EFlowForEachAddOnFunctionReturnValue, 3);

	EFlowForEachAddOnFunctionReturnValue ReturnValue = EFlowForEachAddOnFunctionReturnValue::Continue;

	for (const UFlowNodeAddOn* AddOn : AddOns)
	{
		if (!IsValid(AddOn))
		{
			continue;
		}

		// InterfaceOrClass can either be the AddOn's UClass (or its superclass)
		// or an interface (the UClass version) that its UClass implements 
		if (AddOn->IsA(&InterfaceOrClass) || AddOn->GetClass()->ImplementsInterface(&InterfaceOrClass))
		{
			ReturnValue = Function(*AddOn);

			if (!ShouldContinueForEach(ReturnValue))
			{
				break;
			}
		}

		ReturnValue = AddOn->ForEachAddOnForClassConst(InterfaceOrClass, Function);

		if (!ShouldContinueForEach(ReturnValue))
		{
			break;
		}
	}

	return ReturnValue;
}

EFlowForEachAddOnFunctionReturnValue UFlowNodeBase::ForEachAddOnForClass(const UClass& InterfaceOrClass, const FFlowNodeAddOnFunction& Function) const
{
	FLOW_ASSERT_ENUM_MAX(EFlowForEachAddOnFunctionReturnValue, 3);

	EFlowForEachAddOnFunctionReturnValue ReturnValue = EFlowForEachAddOnFunctionReturnValue::Continue;

	for (UFlowNodeAddOn* AddOn : AddOns)
	{
		if (!IsValid(AddOn))
		{
			continue;
		}

		// InterfaceOrClass can either be the AddOn's UClass (or its superclass)
		// or an interface (the UClass version) that its UClass implements 
		if (AddOn->IsA(&InterfaceOrClass) || AddOn->GetClass()->ImplementsInterface(&InterfaceOrClass))
		{
			ReturnValue = Function(*AddOn);

			if (!ShouldContinueForEach(ReturnValue))
			{
				break;
			}
		}

		ReturnValue = AddOn->ForEachAddOnForClass(InterfaceOrClass, Function);

		if (!ShouldContinueForEach(ReturnValue))
		{
			break;
		}
	}

	return ReturnValue;
}

void UFlowNodeBase::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	EnsureNodeDisplayStyle();
#endif
}

#if WITH_EDITOR

void UFlowNodeBase::SetGraphNode(UEdGraphNode* NewGraphNode)
{
	GraphNode = NewGraphNode;

	UpdateNodeConfigText();
}

void UFlowNodeBase::SetupForEditing(UEdGraphNode& EdGraphNode)
{
	SetGraphNode(&EdGraphNode);

	// Refresh the Config text when setting up this FlowNodeBase for editing
	UpdateNodeConfigText();
}

void UFlowNodeBase::FixNode(UEdGraphNode* NewGraphNode)
{
	// Fix any node pointers that may be out of date
	if (NewGraphNode)
	{
		GraphNode = NewGraphNode;
	}
}

void UFlowNodeBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UFlowNode, AddOns))
	{
		// Potentially need to rebuild the pins from the AddOns of this node
		OnReconstructionRequested.ExecuteIfBound();
	}

	UpdateNodeConfigText();
}

FString UFlowNodeBase::GetNodeCategory() const
{
	if (GetClass()->ClassGeneratedBy)
	{
		const FString& BlueprintCategory = Cast<UBlueprint>(GetClass()->ClassGeneratedBy)->BlueprintCategory;
		if (!BlueprintCategory.IsEmpty())
		{
			return BlueprintCategory;
		}
	}

	return Category;
}

bool UFlowNodeBase::GetDynamicTitleColor(FLinearColor& OutColor) const
{
	// Legacy asset support for NodeStyle == EFlowNodeStyle::Custom
	if (NodeDisplayStyle == FlowNodeStyle::Custom || NodeStyle == EFlowNodeStyle::Custom)
	{
		OutColor = NodeColor;
		return true;
	}

	return false;
}

FText UFlowNodeBase::GetNodeTitle() const
{
	if (GetClass()->ClassGeneratedBy)
	{
		const FString& BlueprintTitle = Cast<UBlueprint>(GetClass()->ClassGeneratedBy)->BlueprintDisplayName;
		if (!BlueprintTitle.IsEmpty())
		{
			return FText::FromString(BlueprintTitle);
		}
	}

	static const FName NAME_DisplayName(TEXT("DisplayName"));
	if (bDisplayNodeTitleWithoutPrefix && !GetClass()->HasMetaData(NAME_DisplayName))
	{
		return GetGeneratedDisplayName();
	}

	return GetClass()->GetDisplayNameText();
}

FText UFlowNodeBase::GetNodeToolTip() const
{
	if (GetClass()->ClassGeneratedBy)
	{
		const FString& BlueprintToolTip = Cast<UBlueprint>(GetClass()->ClassGeneratedBy)->BlueprintDescription;
		if (!BlueprintToolTip.IsEmpty())
		{
			return FText::FromString(BlueprintToolTip);
		}
	}

	static const FName NAME_Tooltip(TEXT("Tooltip"));
	if (bDisplayNodeTitleWithoutPrefix && !GetClass()->HasMetaData(NAME_Tooltip))
	{
		return GetGeneratedDisplayName();
	}

	// GetClass()->GetToolTipText() can return meta = (DisplayName = ... ), but ignore BlueprintDisplayName even if it is BP Node
	if (GetClass()->ClassGeneratedBy)
	{
		const FString& BlueprintTitle = Cast<UBlueprint>(GetClass()->ClassGeneratedBy)->BlueprintDisplayName;
		if (!BlueprintTitle.IsEmpty())
		{
			return FText::FromString(BlueprintTitle);
		}
	}
	

	return GetClass()->GetToolTipText();
}

FText UFlowNodeBase::GetNodeConfigText() const
{
	return DevNodeConfigText;
}

FText UFlowNodeBase::GetGeneratedDisplayName() const
{
	static const FName NAME_GeneratedDisplayName(TEXT("GeneratedDisplayName"));
	
	if (GetClass()->ClassGeneratedBy)
	{
		UClass* Class = Cast<UBlueprint>(GetClass()->ClassGeneratedBy)->GeneratedClass;
		return Class->GetMetaDataText(NAME_GeneratedDisplayName);
	}
	
	return GetClass()->GetMetaDataText(NAME_GeneratedDisplayName);
}

void UFlowNodeBase::EnsureNodeDisplayStyle()
{
	// todo: remove in Flow 2.1
	
	// Backward compatibility update to convert NodeStyle to NodeDisplayStyle
	FLOW_ASSERT_ENUM_MAX(EFlowNodeStyle, 7);

	const FGameplayTag NodeDisplayStylePrev = NodeDisplayStyle;

	switch (NodeStyle)
	{
		case EFlowNodeStyle::Condition:
			{
				NodeDisplayStyle = FlowNodeStyle::Condition;
			}
			break;
		case EFlowNodeStyle::Default:
			{
				NodeDisplayStyle = FlowNodeStyle::Default;
			}
			break;
		case EFlowNodeStyle::InOut:
			{
				NodeDisplayStyle = FlowNodeStyle::InOut;
			}
			break;
		case EFlowNodeStyle::Latent:
			{
				NodeDisplayStyle = FlowNodeStyle::Latent;
			}
			break;
		case EFlowNodeStyle::Logic:
			{
				NodeDisplayStyle = FlowNodeStyle::Logic;
			}
			break;
		case EFlowNodeStyle::SubGraph:
			{
				NodeDisplayStyle = FlowNodeStyle::SubGraph;
			}
			break;
		case EFlowNodeStyle::Custom:
			{
				NodeDisplayStyle = FlowNodeStyle::Custom;
			}
			break;
		default: break;
	}

	if (GEditor != nullptr && NodeDisplayStyle != NodeDisplayStylePrev)
	{
		NodeStyle = EFlowNodeStyle::Invalid;
		Modify();
	}
}

FString UFlowNodeBase::GetNodeDescription() const
{
	return K2_GetNodeDescription();
}
#endif

void UFlowNodeBase::SetNodeConfigText(const FText& NodeConfigText)
{
#if WITH_EDITOR
	if (!NodeConfigText.EqualTo(DevNodeConfigText))
	{
		Modify();

		DevNodeConfigText = NodeConfigText;
	}
#endif // WITH_EDITOR
}

void UFlowNodeBase::UpdateNodeConfigText_Implementation()
{
}

void UFlowNodeBase::LogError(FString Message, const EFlowOnScreenMessageType OnScreenMessageType) const
{
#if !UE_BUILD_SHIPPING
	if (BuildMessage(Message))
	{
		// OnScreen Message
		if (OnScreenMessageType == EFlowOnScreenMessageType::Permanent)
		{
			if (GetWorld())
			{
				if (UViewportStatsSubsystem* StatsSubsystem = GetWorld()->GetSubsystem<UViewportStatsSubsystem>())
				{
					StatsSubsystem->AddDisplayDelegate([this, Message](FText& OutText, FLinearColor& OutColor)
					{
						OutText = FText::FromString(Message);
						OutColor = FLinearColor::Red;

						return IsValid(this) && GetFlowNodeSelfOrOwner()->GetActivationState() != EFlowNodeState::NeverActivated;
					});
				}
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, Message);
		}

		// Output Log
		UE_LOG(LogFlow, Error, TEXT("%s"), *Message);

		// Message Log
#if WITH_EDITOR
		GetFlowAsset()->GetTemplateAsset()->LogError(Message, this);
#endif
	}
#endif
}

void UFlowNodeBase::LogWarning(FString Message) const
{
#if !UE_BUILD_SHIPPING
	if (BuildMessage(Message))
	{
		// Output Log
		UE_LOG(LogFlow, Warning, TEXT("%s"), *Message);

		// Message Log
#if WITH_EDITOR
		GetFlowAsset()->GetTemplateAsset()->LogWarning(Message, this);
#endif
	}
#endif
}

void UFlowNodeBase::LogNote(FString Message) const
{
#if !UE_BUILD_SHIPPING
	if (BuildMessage(Message))
	{
		// Output Log
		UE_LOG(LogFlow, Log, TEXT("%s"), *Message);

		// Message Log
#if WITH_EDITOR
		GetFlowAsset()->GetTemplateAsset()->LogNote(Message, this);
#endif
	}
#endif
}

void UFlowNodeBase::LogVerbose(FString Message) const
{
#if !UE_BUILD_SHIPPING
	if (BuildMessage(Message))
	{
		// Output Log
		UE_LOG(LogFlow, Verbose, TEXT("%s"), *Message);
	}
#endif
}

#if !UE_BUILD_SHIPPING
bool UFlowNodeBase::BuildMessage(FString& Message) const
{
	UFlowAsset* FlowAsset = GetFlowAsset();
	if (FlowAsset && FlowAsset->GetTemplateAsset()) // this is runtime log which is should be only called on runtime instances of asset
	{
		const FString TemplatePath = FlowAsset->GetTemplateAsset()->GetPathName();
		Message.Append(TEXT(" --- node ")).Append(GetName()).Append(TEXT(", asset ")).Append(FPaths::GetPath(TemplatePath) / FPaths::GetBaseFilename(TemplatePath));

		return true;
	}

	return false;
}
#endif

EFlowDataPinResolveResult UFlowNodeBase::TryResolveDataPinPrerequisites(const FName& PinName, const UFlowNode*& FlowNode, const FFlowPin*& FlowPin, EFlowPinType PinType) const
{
	FlowNode = GetFlowNodeSelfOrOwner();

	if (!IsValid(FlowNode))
	{
		LogError(FString::Printf(TEXT("Unexpected for %s to not have an associated FlowNode"), *GetName()), EFlowOnScreenMessageType::Temporary);

		return EFlowDataPinResolveResult::FailedWithError;
	}

	FlowPin = FindFlowPinByName(PinName, FlowNode->GetInputPins());
	if (!FlowPin)
	{
		return EFlowDataPinResolveResult::FailedMissingPin;
	}

	if (FlowPin->GetPinType() != PinType)
	{
		return EFlowDataPinResolveResult::FailedMismatchedType;
	}

	return EFlowDataPinResolveResult::Success;
}

// Must implement TryResolveDataPinAs...() for every EFlowPinType
FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

template <typename TFlowDataPinResultType, EFlowPinType PinType>
bool TResolveDataPinWorkingData<TFlowDataPinResultType, PinType>::TrySetupWorkingData(const FName& PinName, const UFlowNodeBase& FlowNodeBase)
{
	DataPinResult.Result = FlowNodeBase.TryResolveDataPinPrerequisites(PinName, FlowNode, FlowPin, PinType);
	if (DataPinResult.Result != EFlowDataPinResolveResult::Success)
	{
		return false;
	}

	if (!FlowNode->TryGetFlowDataPinSupplierDatasForPinName(FlowPin->PinName, PinValueSupplierDatas))
	{
		return false;
	}

	// If we could not build the PinValueDataSuppliers array, 
	// then the pin must be disconnected and have no default value available.
	DataPinResult.Result = EFlowDataPinResolveResult::FailedUnconnected;

	return true;
}

FFlowDataPinResult_Bool UFlowNodeBase::TryResolveDataPinAsBool(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Bool, EFlowPinType::Bool> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsBool(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_Int UFlowNodeBase::TryResolveDataPinAsInt(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Int, EFlowPinType::Int> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsInt(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_Float UFlowNodeBase::TryResolveDataPinAsFloat(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Float, EFlowPinType::Float> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsFloat(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_Name UFlowNodeBase::TryResolveDataPinAsName(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Name, EFlowPinType::Name> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsName(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_String UFlowNodeBase::TryResolveDataPinAsString(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_String, EFlowPinType::String> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsString(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_Text UFlowNodeBase::TryResolveDataPinAsText(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Text, EFlowPinType::Text> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsText(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_Enum UFlowNodeBase::TryResolveDataPinAsEnum(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Enum, EFlowPinType::Enum> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsEnum(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_Vector UFlowNodeBase::TryResolveDataPinAsVector(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Vector, EFlowPinType::Vector> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsVector(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_Rotator UFlowNodeBase::TryResolveDataPinAsRotator(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Rotator, EFlowPinType::Rotator> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsRotator(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_Transform UFlowNodeBase::TryResolveDataPinAsTransform(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Transform, EFlowPinType::Transform> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsTransform(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_GameplayTag UFlowNodeBase::TryResolveDataPinAsGameplayTag(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_GameplayTag, EFlowPinType::GameplayTag> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsGameplayTag(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_GameplayTagContainer UFlowNodeBase::TryResolveDataPinAsGameplayTagContainer(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_GameplayTagContainer, EFlowPinType::GameplayTagContainer> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsGameplayTagContainer(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_InstancedStruct UFlowNodeBase::TryResolveDataPinAsInstancedStruct(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_InstancedStruct, EFlowPinType::InstancedStruct> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsInstancedStruct(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_Object UFlowNodeBase::TryResolveDataPinAsObject(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Object, EFlowPinType::Object> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsObject(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}

FFlowDataPinResult_Class UFlowNodeBase::TryResolveDataPinAsClass(const FName& PinName) const
{
	TResolveDataPinWorkingData<FFlowDataPinResult_Class, EFlowPinType::Class> WorkData;
	if (!WorkData.TrySetupWorkingData(PinName, *this))
	{
		return WorkData.DataPinResult;
	}

	for (const FFlowPinValueSupplierData& SupplierData : WorkData.PinValueSupplierDatas)
	{
		WorkData.DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPinAsClass(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (WorkData.DataPinResult.Result == EFlowDataPinResolveResult::Success)
		{
			return WorkData.DataPinResult;
		}
	}

	return WorkData.DataPinResult;
}
