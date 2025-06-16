// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/FlowNode.h"
#include "AddOns/FlowNodeAddOn.h"

#include "FlowAsset.h"
#include "FlowSettings.h"
#include "Interfaces/FlowNodeWithExternalDataPinSupplierInterface.h"
#include "Types/FlowDataPinProperties.h"

#include "Components/ActorComponent.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/Actor.h"
#include "Misc/App.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

FFlowPin UFlowNode::DefaultInputPin(TEXT("In"));
FFlowPin UFlowNode::DefaultOutputPin(TEXT("Out"));

FString UFlowNode::MissingIdentityTag = TEXT("Missing Identity Tag");
FString UFlowNode::MissingNotifyTag = TEXT("Missing Notify Tag");
FString UFlowNode::MissingClass = TEXT("Missing class");
FString UFlowNode::NoActorsFound = TEXT("No actors found");

UFlowNode::UFlowNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, AllowedSignalModes({EFlowSignalMode::Enabled, EFlowSignalMode::Disabled, EFlowSignalMode::PassThrough})
	, SignalMode(EFlowSignalMode::Enabled)
	, bPreloaded(false)
	, ActivationState(EFlowNodeState::NeverActivated)
{
#if WITH_EDITOR
	Category = TEXT("Uncategorized");
	NodeDisplayStyle = FlowNodeStyle::Default;
#endif

	InputPins = {DefaultInputPin};
	OutputPins = {DefaultOutputPin};
}

#if WITH_EDITOR

void UFlowNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UFlowNode, InputPins) || PropertyName == GET_MEMBER_NAME_CHECKED(UFlowNode, OutputPins)
		|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UFlowNode, InputPins) || MemberPropertyName == GET_MEMBER_NAME_CHECKED(UFlowNode, OutputPins))
	{
		// Potentially need to rebuild the pins from the this node
		OnReconstructionRequested.ExecuteIfBound();
	}
}

void UFlowNode::PostLoad()
{
	Super::PostLoad();

	// fix Class Default Object
	FixNode(nullptr);
}

#endif

bool UFlowNode::IsSupportedInputPinName(const FName& PinName) const
{
	const FFlowPin* InputPin = FindFlowPinByName(PinName, InputPins);

	if (AddOns.IsEmpty())
	{
		checkf(InputPin, TEXT("Only AddOns should introduce unknown Pins to a FlowNode, so if we have no AddOns, we should have no unknown pins"));
		return true;
	}

	return (InputPin != nullptr);
}

void UFlowNode::AddInputPins(const TArray<FFlowPin>& Pins)
{
	for (const FFlowPin& Pin : Pins)
	{
		InputPins.AddUnique(Pin);
	}
}

void UFlowNode::AddOutputPins(const TArray<FFlowPin>& Pins)
{
	for (const FFlowPin& Pin : Pins)
	{
		OutputPins.AddUnique(Pin);
	}
}

#if WITH_EDITOR

bool UFlowNode::RebuildPinArray(const TArray<FName>& NewPinNames, TArray<FFlowPin>& InOutPins, const FFlowPin& DefaultPin)
{
	bool bIsChanged;

	TArray<FFlowPin> NewPins;

	if (NewPinNames.Num() == 0)
	{
		bIsChanged = true;

		NewPins.Reserve(1);

		NewPins.Add(DefaultPin);
	}
	else
	{
		const bool bIsSameNum = (NewPinNames.Num() == InOutPins.Num());

		bIsChanged = !bIsSameNum;

		NewPins.Reserve(NewPinNames.Num());

		for (int32 NewPinIndex = 0; NewPinIndex < NewPinNames.Num(); ++NewPinIndex)
		{
			const FName& NewPinName = NewPinNames[NewPinIndex];
			NewPins.Add(FFlowPin(NewPinName));

			if (bIsSameNum)
			{
				bIsChanged = bIsChanged || (NewPinName != InOutPins[NewPinIndex].PinName);
			}
		}
	}

	if (bIsChanged)
	{
		InOutPins.Reset();

		check(NewPins.Num() > 0);

		if (&InOutPins == &InputPins)
		{
			AddInputPins(NewPins);
		}
		else
		{
			checkf(&InOutPins == &OutputPins, TEXT("Only expected to be called with one or the other of the pin arrays"));

			AddOutputPins(NewPins);
		}
	}

	return bIsChanged;
}

bool UFlowNode::RebuildPinArray(const TArray<FFlowPin>& NewPins, TArray<FFlowPin>& InOutPins, const FFlowPin& DefaultPin)
{
	TArray<FName> NewPinNames;
	NewPinNames.Reserve(NewPins.Num());

	for (const FFlowPin& NewPin : NewPins)
	{
		NewPinNames.Add(NewPin.PinName);
	}

	return RebuildPinArray(NewPinNames, InOutPins, DefaultPin);
}

#endif // WITH_EDITOR

void UFlowNode::SetNumberedInputPins(const uint8 FirstNumber, const uint8 LastNumber)
{
	InputPins.Empty();

	for (uint8 i = FirstNumber; i <= LastNumber; i++)
	{
		InputPins.Emplace(i);
	}
}

void UFlowNode::SetNumberedOutputPins(const uint8 FirstNumber /*= 0*/, const uint8 LastNumber /*= 1*/)
{
	OutputPins.Empty();

	for (uint8 i = FirstNumber; i <= LastNumber; i++)
	{
		OutputPins.Emplace(i);
	}
}

uint8 UFlowNode::CountNumberedInputs() const
{
	uint8 Result = 0;
	for (const FFlowPin& Pin : InputPins)
	{
		if (Pin.PinName.ToString().IsNumeric())
		{
			Result++;
		}
	}
	return Result;
}

uint8 UFlowNode::CountNumberedOutputs() const
{
	uint8 Result = 0;
	for (const FFlowPin& Pin : OutputPins)
	{
		if (Pin.PinName.ToString().IsNumeric())
		{
			Result++;
		}
	}
	return Result;
}

TArray<FName> UFlowNode::GetInputNames() const
{
	TArray<FName> Result;
	for (const FFlowPin& Pin : InputPins)
	{
		if (!Pin.PinName.IsNone())
		{
			Result.Emplace(Pin.PinName);
		}
	}
	return Result;
}

TArray<FName> UFlowNode::GetOutputNames() const
{
	TArray<FName> Result;
	for (const FFlowPin& Pin : OutputPins)
	{
		if (!Pin.PinName.IsNone())
		{
			Result.Emplace(Pin.PinName);
		}
	}
	return Result;
}

#if WITH_EDITOR

bool UFlowNode::SupportsContextPins() const
{
	if (Super::SupportsContextPins())
	{
		return true;
	}

	if (!GetAutoInputDataPins().IsEmpty() || !GetAutoOutputDataPins().IsEmpty())
	{
		return true;
	}

	for (const UFlowNodeAddOn* AddOn : AddOns)
	{
		if (IsValid(AddOn) && AddOn->SupportsContextPins())
		{
			return true;
		}
	}

	return false;
}

TArray<FFlowPin> UFlowNode::GetContextInputs() const
{
	TArray<FFlowPin> ContextOutputs = Super::GetContextInputs();

	// Add the Auto-Generated DataPins as GetContextInputs
	for (const FFlowPin& AutoGeneratedDataPin : GetAutoInputDataPins())
	{
		ContextOutputs.AddUnique(AutoGeneratedDataPin);
	}

	return ContextOutputs;
}

TArray<FFlowPin> UFlowNode::GetContextOutputs() const
{
	TArray<FFlowPin> ContextOutputs = Super::GetContextOutputs();

	// Add the Auto-Generated DataPins as ContextOutputs
	for (const FFlowPin& AutoGeneratedDataPin : GetAutoOutputDataPins())
	{
		ContextOutputs.AddUnique(AutoGeneratedDataPin);
	}

	return ContextOutputs;
}

bool UFlowNode::CanUserAddInput() const
{
	return K2_CanUserAddInput();
}

bool UFlowNode::CanUserAddOutput() const
{
	return K2_CanUserAddOutput();
}

void UFlowNode::RemoveUserInput(const FName& PinName)
{
	Modify();

	int32 RemovedPinIndex = INDEX_NONE;
	for (int32 i = 0; i < InputPins.Num(); i++)
	{
		if (InputPins[i].PinName == PinName)
		{
			InputPins.RemoveAt(i);
			RemovedPinIndex = i;
			break;
		}
	}

	// update remaining pins
	if (RemovedPinIndex > INDEX_NONE)
	{
		for (int32 i = RemovedPinIndex; i < InputPins.Num(); ++i)
		{
			if (InputPins[i].PinName.ToString().IsNumeric())
			{
				InputPins[i].PinName = *FString::FromInt(i);
			}
		}
	}
}

void UFlowNode::RemoveUserOutput(const FName& PinName)
{
	Modify();

	int32 RemovedPinIndex = INDEX_NONE;
	for (int32 i = 0; i < OutputPins.Num(); i++)
	{
		if (OutputPins[i].PinName == PinName)
		{
			OutputPins.RemoveAt(i);
			RemovedPinIndex = i;
			break;
		}
	}

	// update remaining pins
	if (RemovedPinIndex > INDEX_NONE)
	{
		for (int32 i = RemovedPinIndex; i < OutputPins.Num(); ++i)
		{
			if (OutputPins[i].PinName.ToString().IsNumeric())
			{
				OutputPins[i].PinName = *FString::FromInt(i);
			}
		}
	}
}

void UFlowNode::SetPinNameToBoundPropertyNameMap(const TMap<FName, FName>& Map)
{
	PinNameToBoundPropertyNameMap = Map;
}

void UFlowNode::SetAutoInputDataPins(const TArray<FFlowPin>& AutoInputPins)
{
	AutoInputDataPins = AutoInputPins;
}

void UFlowNode::SetAutoOutputDataPins(const TArray<FFlowPin>& AutoOutputPins)
{
	AutoOutputDataPins = AutoOutputPins;
}

#endif // WITH_EDITOR

bool UFlowNode::CanSupplyDataPinValues_Implementation() const
{
	if (!PinNameToBoundPropertyNameMap.IsEmpty())
	{
		return true;
	}

	return false;
}

bool UFlowNode::TryGetFlowDataPinSupplierDatasForPinName(
	const FName& PinName,
	TArray<FFlowPinValueSupplierData>& InOutPinValueSupplierDatas) const
{
	const IFlowDataPinValueSupplierInterface* ThisAsPinValueSupplier = Cast<IFlowDataPinValueSupplierInterface>(this);

	// This function will build the priority-ordered array of data suppliers for a given PinName.
	// It works in two modes:
	// - Standard case - Add a connected node as the priority supplier, and this node as the default value supplier
	// - Exception case - for External data supplied nodes, we recurse (below) to crawl further and add the supplier
	//   for the external supplier's node.  In practice, this is a node (A) connected to a Start node, which is 
	//   supplied by its outer SubGraph node, which sources its values from the nodes tha are connected to the external inputs
	//   that the subgraph node added as inputs for its instanced subgraph).  The external supplier's value has top priority,
	//   then it falls to the standard case sources (as above).

	// Potentially add this current node as a default value supplier
	// (this will be pushed down the priority queue as higher priority suppliers are found)
	if (ThisAsPinValueSupplier && IFlowDataPinValueSupplierInterface::Execute_CanSupplyDataPinValues(this))
	{
		FFlowPinValueSupplierData NewPinValueSupplier;
		NewPinValueSupplier.PinValueSupplier = ThisAsPinValueSupplier;
		NewPinValueSupplier.SupplierPinName = PinName;

		// Put this node as the backup supplier
		InOutPinValueSupplierDatas.Insert(NewPinValueSupplier, 0);
	}

	// If the pin is connected, try to add the connected node as the priority supplier
	FFlowPinValueSupplierData ConnectedPinValueSupplier;
	FGuid ConnectedNodeGuid;

	if (FindConnectedNodeForPinFast(PinName, &ConnectedNodeGuid, &ConnectedPinValueSupplier.SupplierPinName))
	{
		if (const UFlowAsset* FlowAsset = GetFlowAsset())
		{
			const UFlowNode* SupplierFlowNode = FlowAsset->GetNode(ConnectedNodeGuid);

			// If the connected node can supply data pin values, insert it into the top of the priority queue
			const IFlowDataPinValueSupplierInterface* SupplierFlowNodeAsInterface = Cast<IFlowDataPinValueSupplierInterface>(SupplierFlowNode);
			if (SupplierFlowNodeAsInterface && IFlowDataPinValueSupplierInterface::Execute_CanSupplyDataPinValues(SupplierFlowNode))
			{
				ConnectedPinValueSupplier.PinValueSupplier = SupplierFlowNodeAsInterface;

				InOutPinValueSupplierDatas.Insert(ConnectedPinValueSupplier, 0);
			}

			// Exception case for nodes with external suppliers, recurse here to crawl further 
			// to the external supplier's connected pin as our most preferred source (see block comment above).
			if (const IFlowNodeWithExternalDataPinSupplierInterface* HasExternalPinSupplierInterface = Cast<IFlowNodeWithExternalDataPinSupplierInterface>(SupplierFlowNode))
			{
				if (const UFlowNode* ExternalDataPinSupplierFlowNode = Cast<UFlowNode>(HasExternalPinSupplierInterface->GetExternalDataPinSupplier()))
				{
					return ExternalDataPinSupplierFlowNode->TryGetFlowDataPinSupplierDatasForPinName(ConnectedPinValueSupplier.SupplierPinName, InOutPinValueSupplierDatas);
				}
			}
		}
	}

	return !InOutPinValueSupplierDatas.IsEmpty();
}

bool UFlowNode::TryFindPropertyByPinName(
	const FName& PinName,
	const FProperty*& OutFoundProperty,
	TInstancedStruct<FFlowDataPinProperty>& OutFoundInstancedStruct,
	EFlowDataPinResolveResult& InOutResult) const
{
	const FName* RemappedPinName = PinNameToBoundPropertyNameMap.Find(PinName);
	if (!RemappedPinName)
	{
		InOutResult = EFlowDataPinResolveResult::FailedUnknownPin;

		return false;
	}

	if (!TryFindPropertyByRemappedPinName(*RemappedPinName, OutFoundProperty, OutFoundInstancedStruct, InOutResult))
	{
		return false;
	}

	return true;
}

bool UFlowNode::TryFindPropertyByRemappedPinName(
	const FName& RemappedPinName,
	const FProperty*& OutFoundProperty,
	TInstancedStruct<FFlowDataPinProperty>& OutFoundInstancedStruct,
	EFlowDataPinResolveResult& InOutResult) const
{
	const UClass* ThisClass = GetClass();
	OutFoundProperty = ThisClass->FindPropertyByName(RemappedPinName);

	if (!OutFoundProperty)
	{
		LogError(FString::Printf(TEXT("Could not find property %s, but expected to"), *RemappedPinName.ToString()), EFlowOnScreenMessageType::Temporary);

		InOutResult = EFlowDataPinResolveResult::FailedWithError;

		return false;
	}

	return true;
}

TSet<UFlowNode*> UFlowNode::GatherConnectedNodes() const
{
	TSet<UFlowNode*> Result;
	for (const TPair<FName, FConnectedPin>& Connection : Connections)
	{
		Result.Emplace(GetFlowAsset()->GetNode(Connection.Value.NodeGuid));
	}

	return Result;
}

FName UFlowNode::GetPinConnectedToNode(const FGuid& OtherNodeGuid)
{
	for (const TPair<FName, FConnectedPin>& Connection : Connections)
	{
		if (Connection.Value.NodeGuid == OtherNodeGuid)
		{
			return Connection.Key;
		}
	}

	return NAME_None;
}

bool UFlowNode::IsInputConnected(const FName& PinName, bool bErrorIfPinNotFound) const
{
	if (const FFlowPin* FlowPin = FindFlowPinByName(PinName, InputPins))
	{
		return IsInputConnected(*FlowPin);
	}

	if (bErrorIfPinNotFound)
	{
		LogError(FString::Printf(TEXT("Unknown pin %s"), *PinName.ToString()), EFlowOnScreenMessageType::Temporary);
	}

	return false;
}

bool UFlowNode::IsOutputConnected(const FName& PinName, bool bErrorIfPinNotFound) const
{
	if (const FFlowPin* FlowPin = FindFlowPinByName(PinName, OutputPins))
	{
		return IsOutputConnected(*FlowPin);
	}

	if (bErrorIfPinNotFound)
	{
		LogError(FString::Printf(TEXT("Unknown pin %s"), *PinName.ToString()), EFlowOnScreenMessageType::Temporary);
	}

	return false;
}

FFlowPin* UFlowNode::FindInputPinByName(const FName& PinName)
{
	if (FFlowPin* FlowPin = FindFlowPinByName(PinName, InputPins))
	{
		return FlowPin;
	}

	return nullptr;
}

FFlowPin* UFlowNode::FindOutputPinByName(const FName& PinName)
{
	if (FFlowPin* FlowPin = FindFlowPinByName(PinName, OutputPins))
	{
		return FlowPin;
	}

	return nullptr;
}

bool UFlowNode::IsInputConnected(const FFlowPin& FlowPin) const
{
	if (!InputPins.Contains(FlowPin.PinName))
	{
		return false;
	}

	if (FlowPin.IsDataPin())
	{
		return FindConnectedNodeForPinFast(FlowPin.PinName);
	}
	else
	{
		// We don't cache the input exec pins for fast lookup in Connections, so use the slow path for them:

		return FindConnectedNodeForPinSlow(FlowPin.PinName);
	}
}

bool UFlowNode::IsOutputConnected(const FFlowPin& FlowPin) const
{
	if (!OutputPins.Contains(FlowPin.PinName))
	{
		return false;
	}

	if (FlowPin.IsExecPin())
	{
		return FindConnectedNodeForPinFast(FlowPin.PinName);
	}
	else
	{
		// We don't cache the input data pins for fast lookup in Connections, so use the slow path for them:

		return FindConnectedNodeForPinSlow(FlowPin.PinName);
	}
}

bool UFlowNode::FindConnectedNodeForPinFast(const FName& PinName, FGuid* OutGuid, FName* OutConnectedPinName) const
{
	const FConnectedPin* FoundConnectedPin = Connections.Find(PinName);
	if (FoundConnectedPin)
	{
		if (OutGuid)
		{
			*OutGuid = FoundConnectedPin->NodeGuid;
		}

		if (OutConnectedPinName)
		{
			*OutConnectedPinName = FoundConnectedPin->PinName;
		}
	}

	return FoundConnectedPin != nullptr;
}

bool UFlowNode::FindConnectedNodeForPinSlow(const FName& PinName, FGuid* OutGuid, FName* OutConnectedPinName) const
{
	const UFlowAsset* FlowAsset = GetFlowAsset();

	if (!IsValid(FlowAsset))
	{
		return false;
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : ObjectPtrDecay(FlowAsset->Nodes))
	{
		const FGuid& ConnectedFromGuid = Pair.Key;
		const UFlowNode* ConnectedFromFlowNode = Pair.Value;

		if (!IsValid(ConnectedFromFlowNode))
		{
			continue;
		}

		for (const TPair<FName, FConnectedPin>& Connection : Pair.Value->Connections)
		{
			const FConnectedPin& ConnectedPinStruct = Connection.Value;

			if (ConnectedPinStruct.NodeGuid == NodeGuid && ConnectedPinStruct.PinName == PinName)
			{
				if (OutGuid)
				{
					*OutGuid = ConnectedFromGuid;
				}

				if (OutConnectedPinName)
				{
					*OutConnectedPinName = Connection.Key;
				}

				return true;
			}
		}
	}

	return false;
}

// Must implement TrySupplyDataPinAs... for every EFlowPinType 
FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

FFlowDataPinResult_Bool UFlowNode::TrySupplyDataPinAsBool_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsType<FFlowDataPinResult_Bool, FFlowDataPinOutputProperty_Bool, FBoolProperty>(PinName);
}

FFlowDataPinResult_Int UFlowNode::TrySupplyDataPinAsInt_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsNumericType<FFlowDataPinResult_Int, FFlowDataPinOutputProperty_Int64, FFlowDataPinOutputProperty_Int32>(PinName);
}

FFlowDataPinResult_Float UFlowNode::TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsNumericType<FFlowDataPinResult_Float, FFlowDataPinOutputProperty_Double, FFlowDataPinOutputProperty_Float>(PinName);
}

FFlowDataPinResult_Name UFlowNode::TrySupplyDataPinAsName_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsAnyTextType<FFlowDataPinResult_Name>(PinName);
}

FFlowDataPinResult_String UFlowNode::TrySupplyDataPinAsString_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsAnyTextType<FFlowDataPinResult_String>(PinName);
}

FFlowDataPinResult_Text UFlowNode::TrySupplyDataPinAsText_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsAnyTextType<FFlowDataPinResult_Text>(PinName);
}

FFlowDataPinResult_Enum UFlowNode::TrySupplyDataPinAsEnum_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsEnumType(PinName);
}

FFlowDataPinResult_Vector UFlowNode::TrySupplyDataPinAsVector_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsStructType<FFlowDataPinResult_Vector, FFlowDataPinOutputProperty_Vector, FVector>(PinName);
}

FFlowDataPinResult_Rotator UFlowNode::TrySupplyDataPinAsRotator_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsStructType<FFlowDataPinResult_Rotator, FFlowDataPinOutputProperty_Rotator, FRotator>(PinName);
}

FFlowDataPinResult_Transform UFlowNode::TrySupplyDataPinAsTransform_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsStructType<FFlowDataPinResult_Transform, FFlowDataPinOutputProperty_Transform, FTransform>(PinName);
}

FFlowDataPinResult_GameplayTag UFlowNode::TrySupplyDataPinAsGameplayTag_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsStructType<FFlowDataPinResult_GameplayTag, FFlowDataPinOutputProperty_GameplayTag, FGameplayTag>(PinName);
}

FFlowDataPinResult_GameplayTagContainer UFlowNode::TrySupplyDataPinAsGameplayTagContainer_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsStructType<FFlowDataPinResult_GameplayTagContainer, FFlowDataPinOutputProperty_GameplayTagContainer, FGameplayTagContainer>(PinName);
}

FFlowDataPinResult_InstancedStruct UFlowNode::TrySupplyDataPinAsInstancedStruct_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsStructType<FFlowDataPinResult_InstancedStruct, FFlowDataPinOutputProperty_InstancedStruct, FInstancedStruct>(PinName);
}

FFlowDataPinResult_Object UFlowNode::TrySupplyDataPinAsObject_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsUObjectType<FFlowDataPinResult_Object, FFlowDataPinOutputProperty_Object, UObject, FObjectProperty, FSoftObjectProperty, FWeakObjectProperty, FLazyObjectProperty>(PinName);
}

FFlowDataPinResult_Class UFlowNode::TrySupplyDataPinAsClass_Implementation(const FName& PinName) const
{
	return TrySupplyDataPinAsUClassType<FFlowDataPinResult_Class, FFlowDataPinOutputProperty_Class, UClass, FClassProperty, FSoftClassProperty>(PinName);
}

void UFlowNode::RecursiveFindNodesByClass(UFlowNode* Node, const TSubclassOf<UFlowNode> Class, uint8 Depth, TArray<UFlowNode*>& OutNodes)
{
	if (Node)
	{
		// Record the node if it is the desired type
		if (Node->GetClass() == Class)
		{
			OutNodes.AddUnique(Node);
		}

		if (OutNodes.Num() == Depth)
		{
			return;
		}

		// Recurse
		for (UFlowNode* ConnectedNode : Node->GatherConnectedNodes())
		{
			RecursiveFindNodesByClass(ConnectedNode, Class, Depth, OutNodes);
		}
	}
}

void UFlowNode::TriggerPreload()
{
	bPreloaded = true;
	PreloadContent();
}

void UFlowNode::TriggerFlush()
{
	bPreloaded = false;
	FlushContent();
}

void UFlowNode::TriggerInput(const FName& PinName, const EFlowPinActivationType ActivationType /*= Default*/)
{
	if (SignalMode == EFlowSignalMode::Disabled)
	{
		// entirely ignore any Input activation
	}

	if (InputPins.Contains(PinName))
	{
		if (SignalMode == EFlowSignalMode::Enabled)
		{
			const EFlowNodeState PreviousActivationState = ActivationState;
			if (PreviousActivationState != EFlowNodeState::Active)
			{
				OnActivate();
			}

			ActivationState = EFlowNodeState::Active;
		}

#if !UE_BUILD_SHIPPING
		// record for debugging
		TArray<FPinRecord>& Records = InputRecords.FindOrAdd(PinName);
		Records.Add(FPinRecord(FApp::GetCurrentTime(), ActivationType));

		if (const UFlowAsset* FlowAssetTemplate = GetFlowAsset()->GetTemplateAsset())
		{
			(void)FlowAssetTemplate->OnPinTriggered.ExecuteIfBound(NodeGuid, PinName);
		}
#endif
	}
#if !UE_BUILD_SHIPPING
	else
	{
		LogError(FString::Printf(TEXT("Input Pin name %s invalid"), *PinName.ToString()));
		return;
	}
#endif

	switch (SignalMode)
	{
		case EFlowSignalMode::Enabled:
			ExecuteInputForSelfAndAddOns(PinName);
			break;
		case EFlowSignalMode::Disabled:
			if (UFlowSettings::Get()->bLogOnSignalDisabled)
			{
				LogNote(FString::Printf(TEXT("Node disabled while triggering input %s"), *PinName.ToString()));
			}
			break;
		case EFlowSignalMode::PassThrough:
			if (UFlowSettings::Get()->bLogOnSignalPassthrough)
			{
				LogNote(FString::Printf(TEXT("Signal pass-through on triggering input %s"), *PinName.ToString()));
			}
			OnPassThrough();
			break;
		default: ;
	}
}

void UFlowNode::TriggerFirstOutput(const bool bFinish)
{
	if (OutputPins.Num() > 0)
	{
		TriggerOutput(OutputPins[0].PinName, bFinish);
	}
}

void UFlowNode::TriggerOutput(const FName PinName, const bool bFinish /*= false*/, const EFlowPinActivationType ActivationType /*= Default*/)
{
	if (ActivationState == EFlowNodeState::Completed || ActivationState == EFlowNodeState::Aborted)
	{
		// do not trigger output if node is already finished or aborted
		LogError(TEXT("Trying to TriggerOutput after finished or aborted"));
		return;
	}

	// clean up node, if needed
	if (bFinish)
	{
		Finish();
	}

#if !UE_BUILD_SHIPPING
	if (OutputPins.Contains(PinName))
	{
		// record for debugging, even if nothing is connected to this pin
		TArray<FPinRecord>& Records = OutputRecords.FindOrAdd(PinName);
		Records.Add(FPinRecord(FApp::GetCurrentTime(), ActivationType));

		if (const UFlowAsset* FlowAssetTemplate = GetFlowAsset()->GetTemplateAsset())
		{
			FlowAssetTemplate->OnPinTriggered.ExecuteIfBound(NodeGuid, PinName);
		}
	}
	else
	{
		LogError(FString::Printf(TEXT("Output Pin name %s invalid"), *PinName.ToString()));
	}
#endif

	// call the next node
	if (OutputPins.Contains(PinName) && Connections.Contains(PinName))
	{
		const FConnectedPin FlowPin = GetConnection(PinName);
		GetFlowAsset()->TriggerInput(FlowPin.NodeGuid, FlowPin.PinName);
	}
}

void UFlowNode::Finish()
{
	Deactivate();
	GetFlowAsset()->FinishNode(this);
}

void UFlowNode::Deactivate()
{
	if (SignalMode == EFlowSignalMode::PassThrough)
	{
		// there is nothing to deactivate, node was never active
		return;
	}

	if (GetFlowAsset()->FinishPolicy == EFlowFinishPolicy::Abort)
	{
		ActivationState = EFlowNodeState::Aborted;
	}
	else
	{
		ActivationState = EFlowNodeState::Completed;
	}

	Cleanup();
}

void UFlowNode::ResetRecords()
{
	ActivationState = EFlowNodeState::NeverActivated;

#if !UE_BUILD_SHIPPING
	InputRecords.Empty();
	OutputRecords.Empty();
#endif
}

void UFlowNode::SaveInstance(FFlowNodeSaveData& NodeRecord)
{
	NodeRecord.NodeGuid = NodeGuid;
	OnSave();

	FMemoryWriter MemoryWriter(NodeRecord.NodeData, true);
	FFlowArchive Ar(MemoryWriter);
	Serialize(Ar);
}

void UFlowNode::LoadInstance(const FFlowNodeSaveData& NodeRecord)
{
	FMemoryReader MemoryReader(NodeRecord.NodeData, true);
	FFlowArchive Ar(MemoryReader);
	Serialize(Ar);

	if (UFlowAsset* FlowAsset = GetFlowAsset())
	{
		FlowAsset->OnActivationStateLoaded(this);
	}

	switch (SignalMode)
	{
		case EFlowSignalMode::Enabled:
			OnLoad();
			break;
		case EFlowSignalMode::Disabled:
			// designer doesn't want to execute this node's logic at all, so we kill it
			LogNote(TEXT("Signal disabled while loading Flow Node from SaveGame"));
			Finish();
			break;
		case EFlowSignalMode::PassThrough:
			LogNote(TEXT("Signal pass-through on loading Flow Node from SaveGame"));
			OnPassThrough();
			break;
		default: ;
	}
}

void UFlowNode::OnSave_Implementation()
{
}

void UFlowNode::OnLoad_Implementation()
{
}

void UFlowNode::OnPassThrough_Implementation()
{
	// trigger all connected outputs
	// pin connections aren't serialized to the SaveGame, so users can safely change connections post game release
	for (const FFlowPin& OutputPin : OutputPins)
	{
		if (Connections.Contains(OutputPin.PinName))
		{
			TriggerOutput(OutputPin.PinName, false, EFlowPinActivationType::PassThrough);
		}
	}

	// deactivate node, so it doesn't get saved to a new SaveGame
	Finish();
}

#if WITH_EDITOR
TMap<uint8, FPinRecord> UFlowNode::GetWireRecords() const
{
	TMap<uint8, FPinRecord> Result;
	for (const TPair<FName, TArray<FPinRecord>>& Record : OutputRecords)
	{
		Result.Emplace(OutputPins.IndexOfByKey(Record.Key), Record.Value.Last());
	}
	return Result;
}

TArray<FPinRecord> UFlowNode::GetPinRecords(const FName& PinName, const EEdGraphPinDirection PinDirection) const
{
	switch (PinDirection)
	{
		case EGPD_Input:
			return InputRecords.FindRef(PinName);
		case EGPD_Output:
			return OutputRecords.FindRef(PinName);
		default:
			return TArray<FPinRecord>();
	}
}

#endif

FString UFlowNode::GetIdentityTagDescription(const FGameplayTag& Tag)
{
	return Tag.IsValid() ? Tag.ToString() : MissingIdentityTag;
}

FString UFlowNode::GetIdentityTagsDescription(const FGameplayTagContainer& Tags)
{
	return Tags.IsEmpty() ? MissingIdentityTag : FString::JoinBy(Tags, LINE_TERMINATOR, [](const FGameplayTag& Tag) { return Tag.ToString(); });
}

FString UFlowNode::GetNotifyTagsDescription(const FGameplayTagContainer& Tags)
{
	return Tags.IsEmpty() ? MissingNotifyTag : FString::JoinBy(Tags, LINE_TERMINATOR, [](const FGameplayTag& Tag) { return Tag.ToString(); });
}

FString UFlowNode::GetClassDescription(const TSubclassOf<UObject> Class)
{
	return Class ? Class->GetName() : MissingClass;
}

FString UFlowNode::GetProgressAsString(const float Value)
{
	return FString::Printf(TEXT("%.*f"), 2, Value);
}

#if WITH_EDITOR
UFlowNode* UFlowNode::GetInspectedInstance() const
{
	if (const UFlowAsset* FlowInstance = GetFlowAsset()->GetInspectedInstance())
	{
		return FlowInstance->GetNode(GetGuid());
	}

	return nullptr;
}

FString UFlowNode::GetStatusStringForNodeAndAddOns() const
{
	FString CombinedStatusString = GetStatusString();

	// Give all of the AddOns a chance to add their status strings as well
	(void)ForEachAddOnConst(
		[&CombinedStatusString](const UFlowNodeAddOn& AddOn)
		{
			const FString AddOnStatusString = AddOn.GetStatusString();

			if (!AddOnStatusString.IsEmpty())
			{
				if (!CombinedStatusString.IsEmpty())
				{
					CombinedStatusString += TEXT("\n");
				}

				CombinedStatusString += AddOnStatusString;
			}

			return EFlowForEachAddOnFunctionReturnValue::Continue;
		});

	return CombinedStatusString;
}

bool UFlowNode::GetStatusBackgroundColor(FLinearColor& OutColor) const
{
	return K2_GetStatusBackgroundColor(OutColor);
}

FString UFlowNode::GetAssetPath()
{
	return K2_GetAssetPath();
}

UObject* UFlowNode::GetAssetToEdit()
{
	return K2_GetAssetToEdit();
}

AActor* UFlowNode::GetActorToFocus()
{
	return K2_GetActorToFocus();
}
#endif
