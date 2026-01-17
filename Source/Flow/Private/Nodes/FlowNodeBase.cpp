// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/FlowNodeBase.h"

#include "FlowAsset.h"
#include "FlowLogChannels.h"
#include "FlowSubsystem.h"
#include "FlowTypes.h"
#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowDataPinValueSupplierInterface.h"
#include "Interfaces/FlowNamedPropertiesSupplierInterface.h"
#include "Types/FlowArray.h"
#include "Types/FlowDataPinResults.h"
#include "Types/FlowPinTypesStandard.h"
#include "Types/FlowNamedDataPinProperty.h"

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
#if WITH_EDITORONLY_DATA
	, GraphNode(nullptr)
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
	// To ensure that Nodes and AddOns only get the input pins signaled that they expect,
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
	return FlowPins.FindByPredicate([&PinName](const FFlowPin& FlowPin)
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

TArray<UFlowNodeBase*> UFlowNodeBase::BuildFlowNodeBaseAncestorChain(UFlowNodeBase& FromFlowNodeBase, bool bIncludeFromFlowNodeBase)
{
	TArray<UFlowNodeBase*> AncestorChain;

	UFlowNodeBase* CurOuter = Cast<UFlowNodeBase>(FromFlowNodeBase.GetOuter());
	while (IsValid(CurOuter))
	{
		AncestorChain.Add(CurOuter);

		CurOuter = Cast<UFlowNodeBase>(CurOuter->GetOuter());
	}

	FlowArray::ReverseArray<UFlowNodeBase*, FDefaultAllocator>(AncestorChain);

	if (bIncludeFromFlowNodeBase)
	{
		AncestorChain.Add(&FromFlowNodeBase);
	}

	return AncestorChain;
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

EFlowForEachAddOnFunctionReturnValue UFlowNodeBase::ForEachAddOnConst(
	const FConstFlowNodeAddOnFunction& Function,
	EFlowForEachAddOnChildRule AddOnChildRule) const
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

		FLOW_ASSERT_ENUM_MAX(EFlowForEachAddOnChildRule, 2);
		if (AddOnChildRule == EFlowForEachAddOnChildRule::AllChildren)
		{
			ReturnValue = AddOn->ForEachAddOnConst(Function);

			if (!ShouldContinueForEach(ReturnValue))
			{
				break;
			}
		}
	}

	return ReturnValue;
}

EFlowForEachAddOnFunctionReturnValue UFlowNodeBase::ForEachAddOn(
	const FFlowNodeAddOnFunction& Function,
	EFlowForEachAddOnChildRule AddOnChildRule) const
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

		FLOW_ASSERT_ENUM_MAX(EFlowForEachAddOnChildRule, 2);
		if (AddOnChildRule == EFlowForEachAddOnChildRule::AllChildren)
		{
			ReturnValue = AddOn->ForEachAddOn(Function);

			if (!ShouldContinueForEach(ReturnValue))
			{
				break;
			}
		}
	}

	return ReturnValue;
}

EFlowForEachAddOnFunctionReturnValue UFlowNodeBase::ForEachAddOnForClassConst(
	const UClass& InterfaceOrClass,
	const FConstFlowNodeAddOnFunction& Function,
	EFlowForEachAddOnChildRule AddOnChildRule) const
{
	FLOW_ASSERT_ENUM_MAX(EFlowForEachAddOnFunctionReturnValue, 3);

	EFlowForEachAddOnFunctionReturnValue ReturnValue = EFlowForEachAddOnFunctionReturnValue::Continue;

	for (const UFlowNodeAddOn* AddOn : AddOns)
	{
		if (!IsValid(AddOn))
		{
			continue;
		}

		if (AddOn->IsClassOrImplementsInterface(InterfaceOrClass))
		{
			ReturnValue = Function(*AddOn);

			if (!ShouldContinueForEach(ReturnValue))
			{
				break;
			}
		}

		FLOW_ASSERT_ENUM_MAX(EFlowForEachAddOnChildRule, 2);
		if (AddOnChildRule == EFlowForEachAddOnChildRule::AllChildren)
		{
			ReturnValue = AddOn->ForEachAddOnForClassConst(InterfaceOrClass, Function);

			if (!ShouldContinueForEach(ReturnValue))
			{
				break;
			}
		}
	}

	return ReturnValue;
}

EFlowForEachAddOnFunctionReturnValue UFlowNodeBase::ForEachAddOnForClass(
	const UClass& InterfaceOrClass,
	const FFlowNodeAddOnFunction& Function,
	EFlowForEachAddOnChildRule AddOnChildRule) const
{
	FLOW_ASSERT_ENUM_MAX(EFlowForEachAddOnFunctionReturnValue, 3);

	EFlowForEachAddOnFunctionReturnValue ReturnValue = EFlowForEachAddOnFunctionReturnValue::Continue;

	for (UFlowNodeAddOn* AddOn : AddOns)
	{
		if (!IsValid(AddOn))
		{
			continue;
		}

		if (AddOn->IsClassOrImplementsInterface(InterfaceOrClass))
		{
			ReturnValue = Function(*AddOn);

			if (!ShouldContinueForEach(ReturnValue))
			{
				break;
			}
		}

		FLOW_ASSERT_ENUM_MAX(EFlowForEachAddOnChildRule, 2);
		if (AddOnChildRule == EFlowForEachAddOnChildRule::AllChildren)
		{
			ReturnValue = AddOn->ForEachAddOnForClass(InterfaceOrClass, Function);

			if (!ShouldContinueForEach(ReturnValue))
			{
				break;
			}
		}
	}

	return ReturnValue;
}

#if WITH_EDITOR
void UFlowNodeBase::PostLoad()
{
	Super::PostLoad();

	EnsureNodeDisplayStyle();
}

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
	// todo: remove in Flow 2.2

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

bool UFlowNodeBase::CanModifyFlowDataPinType() const
{
	return !IsPlacedInFlowAsset() || IsFlowNamedPropertiesSupplier();
}

bool UFlowNodeBase::ShowFlowDataPinValueInputPinCheckbox() const
{
	const bool bIsPlacedInFlowAsset = IsPlacedInFlowAsset();
	return !bIsPlacedInFlowAsset;
}

bool UFlowNodeBase::ShowFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const
{
	const bool bIsPlacedInFlowAsset = IsPlacedInFlowAsset();
	const bool bIsFlowNamedPropertiesSupplier = IsFlowNamedPropertiesSupplier();
	return !bIsPlacedInFlowAsset || bIsFlowNamedPropertiesSupplier;
}

bool UFlowNodeBase::CanEditFlowDataPinValueClassFilter(const FFlowDataPinValue* Value) const
{
	const bool bIsPlacedInFlowAsset = IsPlacedInFlowAsset();
	const bool bIsFlowNamedPropertiesSupplier = IsFlowNamedPropertiesSupplier();
	return !bIsPlacedInFlowAsset || bIsFlowNamedPropertiesSupplier;
}

bool UFlowNodeBase::IsPlacedInFlowAsset() const
{
	return GetFlowAsset() != nullptr;
}

bool UFlowNodeBase::IsFlowNamedPropertiesSupplier() const
{
	return Implements<UFlowNamedPropertiesSupplierInterface>();
}

#endif

FText UFlowNodeBase::K2_GetNodeTitle_Implementation() const
{
#if WITH_EDITOR
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
#else
	return FText::GetEmpty();
#endif
}

FText UFlowNodeBase::K2_GetNodeToolTip_Implementation() const
{
#if WITH_EDITOR
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
#else
	return FText::GetEmpty();
#endif
}

FText UFlowNodeBase::GetNodeConfigText() const
{
#if WITH_EDITORONLY_DATA
	return DevNodeConfigText;
#else
	return FText::GetEmpty();
#endif // WITH_EDITORONLY_DATA
}

void UFlowNodeBase::SetNodeConfigText(const FText& NodeConfigText)
{
#if WITH_EDITOR
	if (!NodeConfigText.EqualTo(DevNodeConfigText))
	{
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
			if (UWorld* World = GetWorld())
			{
				if (UViewportStatsSubsystem* StatsSubsystem = World->GetSubsystem<UViewportStatsSubsystem>())
				{
					StatsSubsystem->AddDisplayDelegate([WeakThis = TWeakObjectPtr<const UFlowNodeBase>(this), Message](FText& OutText, FLinearColor& OutColor)
					{
						const UFlowNodeBase* ThisPtr = WeakThis.Get();
						if (ThisPtr && ThisPtr->GetFlowNodeSelfOrOwner()->GetActivationState() != EFlowNodeState::NeverActivated)
						{
							OutText = FText::FromString(Message);
							OutColor = FLinearColor::Red;
							return true;
						}

						return false;
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

#if WITH_EDITOR
		if (GEditor)
		{
			// Message Log
			GetFlowAsset()->GetTemplateAsset()->LogError(Message, this);
		}
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

#if WITH_EDITOR
		if (GEditor)
		{
			// Message Log
			GetFlowAsset()->GetTemplateAsset()->LogWarning(Message, this);
		}
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

#if WITH_EDITOR
		if (GEditor)
		{
			// Message Log
			GetFlowAsset()->GetTemplateAsset()->LogNote(Message, this);
		}
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
	const UFlowAsset* FlowAsset = GetFlowAsset();
	if (FlowAsset && FlowAsset->GetTemplateAsset()) // this is runtime log which is should be only called on runtime instances of asset
	{
		const FString TemplatePath = FlowAsset->GetTemplateAsset()->GetPathName();
		Message.Append(TEXT(" --- node ")).Append(GetName()).Append(TEXT(", asset ")).Append(FPaths::GetPath(TemplatePath) / FPaths::GetBaseFilename(TemplatePath));

		return true;
	}

	return false;
}
#endif

bool UFlowNodeBase::TryAddValueToFormatNamedArguments(const FFlowNamedDataPinProperty& NamedDataPinProperty, FFormatNamedArguments& InOutArguments) const
{
	const FFlowDataPinValue& DataPinValue = NamedDataPinProperty.DataPinValue.Get();

	const FFlowPinTypeName PinTypeName = DataPinValue.GetPinTypeName();
	if (PinTypeName.IsNone())
	{
		return false;
	}

	const FFlowPinType* PinType = FFlowPinType::LookupPinType(PinTypeName);
	if (!PinType)
	{
		return false;
	}

	FFormatArgumentValue FormatValue;
	if (PinType->ResolveAndFormatPinValue(*this, NamedDataPinProperty.Name, FormatValue))
	{
		InOutArguments.Add(NamedDataPinProperty.Name.ToString(), FormatValue);
		return true;
	}

	return false;
}

FFlowDataPinResult UFlowNodeBase::TryResolveDataPin(FName PinName) const
{
	FFlowDataPinResult DataPinResult(EFlowDataPinResolveResult::Success);

	const UFlowNode* FlowNode = GetFlowNodeSelfOrOwner();
	UFlowNode::TFlowPinValueSupplierDataArray PinValueSupplierDatas;
	if (!FlowNode->TryGetFlowDataPinSupplierDatasForPinName(PinName, PinValueSupplierDatas))
	{
		// If we could not build the PinValueDataSuppliers array, 
		// then the pin must be disconnected and have no default value available.
		DataPinResult.Result = EFlowDataPinResolveResult::FailedWithError;

		LogError(FString::Printf(TEXT("DataPin named '%s' could not be supplied with a value."), *PinName.ToString()), EFlowOnScreenMessageType::Temporary);

		return DataPinResult;
	}

	// Iterate over the suppliers in inverse order
	for (int32 Index = PinValueSupplierDatas.Num() - 1; Index >= 0; --Index)
	{
		const FFlowPinValueSupplierData& SupplierData = PinValueSupplierDatas[Index];

		DataPinResult = IFlowDataPinValueSupplierInterface::Execute_TrySupplyDataPin(CastChecked<UObject>(SupplierData.PinValueSupplier), SupplierData.SupplierPinName);

		if (FlowPinType::IsSuccess(DataPinResult.Result))
		{
			return DataPinResult;
		}
	}

	return DataPinResult;
}

// #FlowDataPinLegacy
FFlowDataPinResult_Bool UFlowNodeBase::TryResolveDataPinAsBool(const FName& PinName) const
{
	FFlowDataPinResult_Bool BoolResolveResult;
	BoolResolveResult.Result = TryResolveDataPinValue<FFlowPinType_Bool>(PinName, BoolResolveResult.Value);
	return BoolResolveResult;
}

FFlowDataPinResult_Int UFlowNodeBase::TryResolveDataPinAsInt(const FName& PinName) const
{
	FFlowDataPinResult_Int ResolveResult;
	int32 Value = 0;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_Int>(PinName, Value);
	ResolveResult.Value = Value;
	return ResolveResult;
}

FFlowDataPinResult_Float UFlowNodeBase::TryResolveDataPinAsFloat(const FName& PinName) const
{
	FFlowDataPinResult_Float ResolveResult;
	float Value = 0.0f;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_Float>(PinName, Value);
	ResolveResult.Value = Value;
	return ResolveResult;
}

FFlowDataPinResult_Name UFlowNodeBase::TryResolveDataPinAsName(const FName& PinName) const
{
	FFlowDataPinResult_Name ResolveResult;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_Name>(PinName, ResolveResult.Value);
	return ResolveResult;
}

FFlowDataPinResult_String UFlowNodeBase::TryResolveDataPinAsString(const FName& PinName) const
{
	FFlowDataPinResult_String ResolveResult;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_String>(PinName, ResolveResult.Value);
	return ResolveResult;
}

FFlowDataPinResult_Text UFlowNodeBase::TryResolveDataPinAsText(const FName& PinName) const
{
	FFlowDataPinResult_Text ResolveResult;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_Text>(PinName, ResolveResult.Value);
	return ResolveResult;
}

FFlowDataPinResult_Enum UFlowNodeBase::TryResolveDataPinAsEnum(const FName& PinName) const
{
	const FFlowDataPinResult DataPinResult = TryResolveDataPin(PinName);
	if (!FlowPinType::IsSuccess(DataPinResult.Result))
	{
		return FFlowDataPinResult_Enum(DataPinResult.Result);
	}

	const FFlowDataPinValue_Enum& Wrapper = DataPinResult.ResultValue.Get<FFlowDataPinValue_Enum>();

	if (Wrapper.Values.IsEmpty())
	{
		return FFlowDataPinResult_Enum(EFlowDataPinResolveResult::FailedInsufficientValues);
	}

	const FFlowDataPinResult_Enum ResolveResult(Wrapper.Values[0], Wrapper.EnumClass.LoadSynchronous());
	return ResolveResult;
}

FFlowDataPinResult_Vector UFlowNodeBase::TryResolveDataPinAsVector(const FName& PinName) const
{
	FFlowDataPinResult_Vector ResolveResult;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_Vector>(PinName, ResolveResult.Value);
	return ResolveResult;
}

FFlowDataPinResult_Rotator UFlowNodeBase::TryResolveDataPinAsRotator(const FName& PinName) const
{
	FFlowDataPinResult_Rotator ResolveResult;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_Rotator>(PinName, ResolveResult.Value);
	return ResolveResult;
}

FFlowDataPinResult_Transform UFlowNodeBase::TryResolveDataPinAsTransform(const FName& PinName) const
{
	FFlowDataPinResult_Transform ResolveResult;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_Transform>(PinName, ResolveResult.Value);
	return ResolveResult;
}

FFlowDataPinResult_GameplayTag UFlowNodeBase::TryResolveDataPinAsGameplayTag(const FName& PinName) const
{
	FFlowDataPinResult_GameplayTag ResolveResult;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_GameplayTag>(PinName, ResolveResult.Value);
	return ResolveResult;
}

FFlowDataPinResult_GameplayTagContainer UFlowNodeBase::TryResolveDataPinAsGameplayTagContainer(const FName& PinName) const
{
	FFlowDataPinResult_GameplayTagContainer ResolveResult;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_GameplayTagContainer>(PinName, ResolveResult.Value);
	return ResolveResult;
}

FFlowDataPinResult_InstancedStruct UFlowNodeBase::TryResolveDataPinAsInstancedStruct(const FName& PinName) const
{
	FFlowDataPinResult_InstancedStruct ResolveResult;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_InstancedStruct>(PinName, ResolveResult.Value);
	return ResolveResult;
}

FFlowDataPinResult_Object UFlowNodeBase::TryResolveDataPinAsObject(const FName& PinName) const
{
	FFlowDataPinResult_Object ResolveResult;
	TObjectPtr<UObject> Value = nullptr;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_Object>(PinName, Value);
	ResolveResult.Value = Value;
	return ResolveResult;
}

FFlowDataPinResult_Class UFlowNodeBase::TryResolveDataPinAsClass(const FName& PinName) const
{
	FFlowDataPinResult_Class ResolveResult;
	TObjectPtr<UClass> Value = nullptr;
	ResolveResult.Result = TryResolveDataPinValue<FFlowPinType_Class>(PinName, Value);
	ResolveResult.SetValueFromObjectPtr(Value);
	return ResolveResult;
}
// --