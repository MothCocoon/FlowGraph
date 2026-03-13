// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/FlowNode.h"
#include "AddOns/FlowNodeAddOn.h"

#include "FlowAsset.h"
#include "FlowSettings.h"
#include "Interfaces/FlowNodeWithExternalDataPinSupplierInterface.h"
#include "Types/FlowAutoDataPinsWorkingData.h"
#include "Types/FlowDataPinValue.h"
#include "Types/FlowPinConnectionChange.h"
#include "Types/FlowPinType.h"

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

UFlowNode::UFlowNode()
	: AllowedSignalModes({EFlowSignalMode::Enabled, EFlowSignalMode::Disabled, EFlowSignalMode::PassThrough})
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
		// Potentially need to rebuild the pins from this node
		OnReconstructionRequested.ExecuteIfBound();
	}
}

EDataValidationResult UFlowNode::ValidateNode()
{
	EDataValidationResult ValidationResult = Super::ValidateNode();

	// Validate that output and input pins have unique names
	TSet<FName> UniquePinNames;
	ValidateFlowPinArrayIsUnique(InputPins, UniquePinNames, ValidationResult);
	ValidateFlowPinArrayIsUnique(OutputPins, UniquePinNames, ValidationResult);

	return ValidationResult;
}

void UFlowNode::ValidateFlowPinArrayIsUnique(const TArray<FFlowPin>& FlowPins, TSet<FName>& InOutUniquePinNames, EDataValidationResult& InOutResult)
{
	for (const FFlowPin& FlowPin : FlowPins)
	{
		const FName& ThisPinName = FlowPin.PinName;
		if (InOutUniquePinNames.Contains(ThisPinName))
		{
			ValidationLog.Warning<UFlowNode>(
				*FString::Printf(
					TEXT("All pin names on a flow node must be unique, pin name %s is duplicated"),
					*ThisPinName.ToString()),
				this);

			InOutResult = EDataValidationResult::Invalid;
		}
		else
		{
			InOutUniquePinNames.Add(FlowPin.PinName);
		}
	}
}

void UFlowNode::EnsureAddOnFlowNodePointersForEditor()
{
	ForEachAddOn(
		[this](UFlowNodeAddOn& AddOn) -> EFlowForEachAddOnFunctionReturnValue
		{
			AddOn.SetFlowNodeForEditor(this);
			return EFlowForEachAddOnFunctionReturnValue::Continue;
		});
}

#endif

void UFlowNode::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	// fix Class Default Object
	FixNode(nullptr);
#endif

	if (!HasAnyFlags(RF_ArchetypeObject | RF_ClassDefaultObject))
	{
		FixupDataPinTypes();
	}
}

bool UFlowNode::IsSupportedInputPinName(const FName& PinName) const
{
	const FFlowPin* InputPin = FindInputPinByName(PinName);

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

void UFlowNode::SetupForEditing(UEdGraphNode& EdGraphNode)
{
	Super::SetupForEditing(EdGraphNode);

	// Ensure AddOn editor pointers are correct as soon as we're prepared for editing.
	EnsureAddOnFlowNodePointersForEditor();
}

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

	if (!AutoInputDataPins.IsEmpty() || !AutoOutputDataPins.IsEmpty())
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
	for (const FFlowPin& AutoGeneratedDataPin : AutoInputDataPins)
	{
		ContextOutputs.AddUnique(AutoGeneratedDataPin);
	}

	return ContextOutputs;
}

TArray<FFlowPin> UFlowNode::GetContextOutputs() const
{
	TArray<FFlowPin> ContextOutputs = Super::GetContextOutputs();

	// Add the Auto-Generated DataPins as ContextOutputs
	for (const FFlowPin& AutoGeneratedDataPin : AutoOutputDataPins)
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

bool UFlowNode::TryUpdateAutoDataPins()
{
	FFlowAutoDataPinsWorkingData WorkingData(AutoInputDataPins, AutoOutputDataPins);

	FFlowDataPinValueOwnerCollection ValueOwnerCollection;
	GatherDataPinValueOwnerCollection(ValueOwnerCollection);

	for (FFlowDataPinValueOwner& ValueOwner : ValueOwnerCollection.GetValueOwners())
	{
		check(ValueOwner.IsValid());
		ValueOwner.OwnerInterface->AutoGenerateDataPins(ValueOwner, WorkingData);
	}

	FFlowAutoDataPinsWorkingData::FBuildResult BuildResult;
	WorkingData.Build(*this, BuildResult);

	const bool bAutoInputDataPinsChanged = !FFlowAutoDataPinsWorkingData::CheckIfProposedPinsMatchPreviousPins(AutoInputDataPins, BuildResult.AutoInputPins);
	const bool bAutoOutputDataPinsChanged = !FFlowAutoDataPinsWorkingData::CheckIfProposedPinsMatchPreviousPins(AutoOutputDataPins, BuildResult.AutoOutputPins);

	// Compare runtime map vs proposed editor map
	bool bPropertySourceMapChanged = false;
	if (MapDataPinNameToPropertySource.Num() != BuildResult.MapDataPinNameToPropertySource.Num())
	{
		bPropertySourceMapChanged = true;
	}
	else
	{
		for (const TPair<FName, FFlowPinPropertySource>& KVP : MapDataPinNameToPropertySource)
		{
			const FFlowPinPropertySource* Proposed = BuildResult.MapDataPinNameToPropertySource.Find(KVP.Key);
			if (!Proposed || Proposed->PropertyName != KVP.Value.PropertyName || Proposed->ValueOwnerIndex != KVP.Value.ValueOwnerIndex)
			{
				bPropertySourceMapChanged = true;
				break;
			}
		}
	}

	bool bAnyWrapperChanged = false;
	for (const FFlowAutoDataPinsWorkingData::FDeferredValuePinNamePatch& Patch : BuildResult.DeferredValuePatches)
	{
		if (Patch.DataPinValue && Patch.DataPinValue->PropertyPinName != Patch.NewPinName)
		{
			bAnyWrapperChanged = true;
			break;
		}
	}

	const bool bAnyChange = bAutoInputDataPinsChanged || bAutoOutputDataPinsChanged || bPropertySourceMapChanged || bAnyWrapperChanged;
	if (!bAnyChange)
	{
		return false;
	}

	// Only Modify() if the regenerated pins are significantly different
	SetFlags(RF_Transactional);
	Modify();

	if (bAutoInputDataPinsChanged)
	{
		AutoInputDataPins = MoveTemp(BuildResult.AutoInputPins);
	}

	if (bAutoOutputDataPinsChanged)
	{
		AutoOutputDataPins = MoveTemp(BuildResult.AutoOutputPins);
	}

	if (bPropertySourceMapChanged)
	{
		MapDataPinNameToPropertySource.Reset();
		MapDataPinNameToPropertySource.Reserve(BuildResult.MapDataPinNameToPropertySource.Num());

		for (const TPair<FName, FFlowPinPropertySource>& KVP : BuildResult.MapDataPinNameToPropertySource)
		{
			MapDataPinNameToPropertySource.Add(KVP.Key, KVP.Value);
		}
	}

	for (const FFlowAutoDataPinsWorkingData::FDeferredValuePinNamePatch& Patch : BuildResult.DeferredValuePatches)
	{
		if (Patch.DataPinValue)
		{
			Patch.DataPinValue->PropertyPinName = Patch.NewPinName;
		}
	}

	return true;
}
#endif

FFlowDataPinResult UFlowNode::TrySupplyDataPin(FName PinName) const
{
	const FFlowPin* FlowPin = FindOutputPinByName(PinName);
	if (!FlowPin)
	{
		// Also look in the Input Pins (for supplying default values for unconnected pins)
		FlowPin = FindInputPinByName(PinName);
		if (!FlowPin)
		{
			return FFlowDataPinResult(EFlowDataPinResolveResult::FailedUnknownPin);
		}
	}

	const FFlowPinType* DataPinType = FlowPin->ResolveFlowPinType();
	if (!DataPinType)
	{
		return FFlowDataPinResult(EFlowDataPinResolveResult::FailedMismatchedType);
	}

	FFlowDataPinResult SuppliedResult;
	if (TryGatherPropertyOwnersAndPopulateResult(PinName, *DataPinType, *FlowPin, SuppliedResult))
	{
		return SuppliedResult;
	}

	return FFlowDataPinResult(EFlowDataPinResolveResult::FailedUnknownPin);
}

void UFlowNode::GatherDataPinValueOwnerCollection(FFlowDataPinValueOwnerCollection& ValueOwnerCollection) const
{
	// When called in the editor, 'this' may mutate to disambiguate generated data pins
	UFlowNode* MutableThis = const_cast<UFlowNode*>(this);
	ValueOwnerCollection.AddValueOwner(*MutableThis);

	// Give all the AddOns a chance to supply data pins as well
	(void) ForEachAddOn(
		[&ValueOwnerCollection](UFlowNodeAddOn& AddOn)
		{
			ValueOwnerCollection.AddValueOwner(AddOn);

			return EFlowForEachAddOnFunctionReturnValue::Continue;
		});
}

bool UFlowNode::TryGatherPropertyOwnersAndPopulateResult(
	const FName& PinName,
	const FFlowPinType& DataPinType,
	const FFlowPin& FlowPin,
	FFlowDataPinResult& OutSuppliedResult) const
{
	// Gather all potential UObject instances that might own properties
	// mapped to data pins on this node (usually the node itself + any referenced objects)
	FFlowDataPinValueOwnerCollection ValueOwnerCollection; 
	GatherDataPinValueOwnerCollection(ValueOwnerCollection);

	// Early out if we have no possible owners at all
	if (ValueOwnerCollection.IsEmpty())
	{
		LogError(FString::Printf(TEXT("No property owners available for data pin '%s' on node %s"),
			*PinName.ToString(), *GetName()), EFlowOnScreenMessageType::Temporary);

		return false;
	}

	const FFlowDataPinValueOwner* ValueOwner = nullptr;
	FName PropertyNameToLookup;
	const TArray<FFlowDataPinValueOwner>& ValueOwners = ValueOwnerCollection.GetValueOwners();

	// Look up explicit mapping (used for non-default owners or disambiguated pins)
	if (const FFlowPinPropertySource* FlowPropertySource = MapDataPinNameToPropertySource.Find(PinName))
	{
		const int32 OwnerIndex = FlowPropertySource->ValueOwnerIndex;

		if (ValueOwners.IsValidIndex(OwnerIndex))
		{
			ValueOwner = &ValueOwners[OwnerIndex];
			PropertyNameToLookup = FlowPropertySource->PropertyName;
		}
		else
		{
			// Critical: mapped index is out of bounds → configuration or generation bug
			LogError(FString::Printf(TEXT("Invalid property owner index %d for pin '%s' on node %s (max %d owners)"),
				OwnerIndex, *PinName.ToString(), *GetName(), ValueOwners.Num() - 1),
				EFlowOnScreenMessageType::Temporary);

			return false;
		}
	}
	else 
	{
		check(!ValueOwners.IsEmpty());

		// Fallback for unmapped pins → assume default owner (index 0) + pin name == property name
		ValueOwner = &ValueOwners[0];
		check(ValueOwner->IsDefaultValueOwner());

		PropertyNameToLookup = PinName;
	}

	if (!ValueOwner)
	{
		LogError(FString::Printf(TEXT("Failed to resolve property owner for data pin '%s' on node %s"),
			*PinName.ToString(), *GetName()), EFlowOnScreenMessageType::Temporary);

		return false;
	}

	// Populate the value for the pin on the its owner object
	const UObject* ValueOwnerAsObject = Cast<UObject>(ValueOwner->OwnerInterface);
	const UFlowNode& FlowNodeThis = *this;
	if (DataPinType.PopulateResult(*ValueOwnerAsObject, FlowNodeThis, PropertyNameToLookup, OutSuppliedResult))
	{
		return true;
	}

	return false;
}
// --

bool UFlowNode::TryGetFlowDataPinSupplierDatasForPinName(const FName& PinName, TFlowPinValueSupplierDataArray& InOutPinValueSupplierDatas) const
{
	const IFlowDataPinValueSupplierInterface* ThisAsPinValueSupplier = Cast<IFlowDataPinValueSupplierInterface>(this);

	// This function will build the inverse-priority-ordered array of data suppliers for a given PinName.
	// It works in two modes:
	// - Standard case - Add a connected node as the priority supplier, and this node as the default value supplier
	// - Exception case - for External data supplied nodes, we recurse (below) to crawl further and add the supplier
	//   for the external supplier's node.  In practice, this is a node (A) connected to a Start node, which is 
	//   supplied by its outer SubGraph node, which sources its values from the nodes that are connected to the external inputs
	//   that the subgraph node added as inputs for its instanced subgraph).  The external supplier's value has top priority,
	//   then it falls to the standard case sources (as above).

	// Potentially add this current node as a default value supplier
	// (this will be pushed down the priority queue as higher priority suppliers are found)
	FFlowPinValueSupplierData NewPinValueSupplier;
	NewPinValueSupplier.PinValueSupplier = ThisAsPinValueSupplier;
	NewPinValueSupplier.SupplierPinName = PinName;
	TryAddSupplierDataToArray(NewPinValueSupplier, InOutPinValueSupplierDatas);

	// If the pin is connected, try to add the connected node as the priority supplier
	FConnectedPin ConnectedPin;

	if (FindConnectedNodeForPinCached(PinName, ConnectedPin))
	{
		const FGuid& ConnectedNodeGuid = ConnectedPin.NodeGuid;

		FFlowPinValueSupplierData ConnectedPinValueSupplier;
		ConnectedPinValueSupplier.SupplierPinName = ConnectedPin.PinName;

		if (const UFlowAsset* FlowAsset = GetFlowAsset())
		{
			const UFlowNode* SupplierFlowNode = FlowAsset->GetNode(ConnectedNodeGuid);

			if (IsValid(SupplierFlowNode))
			{
				ConnectedPinValueSupplier.PinValueSupplier = Cast<IFlowDataPinValueSupplierInterface>(SupplierFlowNode);

				TryAddSupplierDataToArray(ConnectedPinValueSupplier, InOutPinValueSupplierDatas);
			}
		}
	}

	return !InOutPinValueSupplierDatas.IsEmpty();
}

void UFlowNode::TryAddSupplierDataToArray(FFlowPinValueSupplierData& InOutSupplierData, TFlowPinValueSupplierDataArray& InOutPinValueSupplierDatas) const
{
	// If the connected node can supply data pin values, insert it into the top of the priority queue
	const UFlowNode* SupplierFlowNode = CastChecked<UFlowNode>(InOutSupplierData.PinValueSupplier);
	if (InOutSupplierData.PinValueSupplier && SupplierFlowNode->CanSupplyDataPinValues())
	{
		InOutPinValueSupplierDatas.Add(InOutSupplierData);
	}

	// Exception case for nodes with external suppliers, recurse here to crawl further 
	// to the external supplier's connected pin as our most preferred source (see block comment above).
	if (const IFlowNodeWithExternalDataPinSupplierInterface* HasExternalPinSupplierInterface = Cast<IFlowNodeWithExternalDataPinSupplierInterface>(SupplierFlowNode))
	{
		if (const UFlowNode* ExternalDataPinSupplierFlowNode = Cast<UFlowNode>(HasExternalPinSupplierInterface->GetExternalDataPinSupplier()))
		{
			ExternalDataPinSupplierFlowNode->TryGetFlowDataPinSupplierDatasForPinName(InOutSupplierData.SupplierPinName, InOutPinValueSupplierDatas);
		}
	}
}

// #FlowDataPinLegacy
void UFlowNode::FixupDataPinTypes()
{
	FixupDataPinTypesForArray(InputPins);
	FixupDataPinTypesForArray(OutputPins);
#if WITH_EDITOR
	FixupDataPinTypesForArray(AutoInputDataPins);
	FixupDataPinTypesForArray(AutoOutputDataPins);
#endif
}

void UFlowNode::FixupDataPinTypesForArray(TArray<FFlowPin>& MutableDataPinArray)
{
	for (FFlowPin& MutableFlowPin : MutableDataPinArray)
	{
		FixupDataPinTypesForPin(MutableFlowPin);
	}
}

void UFlowNode::FixupDataPinTypesForPin(FFlowPin& MutableDataPin)
{
	const FFlowPinTypeName NewPinTypeName = FFlowPin::GetPinTypeNameForLegacyPinType(MutableDataPin.PinType);

	if (!NewPinTypeName.IsNone())
	{
		MutableDataPin.SetPinTypeName(NewPinTypeName);
	}

	if (MutableDataPin.GetPinTypeName().IsNone())
	{
		// Ensure we have a pin type even if the enum was invalid before
		MutableDataPin.SetPinTypeName(FFlowPinType_Exec::GetPinTypeNameStatic());
	}

	MutableDataPin.PinType = EFlowPinType::Invalid;
}
// --

#if WITH_EDITOR

void UFlowNode::BuildConnectionChangeList(
	const UFlowAsset& FlowAsset,
	const TMap<FName, FConnectedPin>& OldConnections,
	const TMap<FName, FConnectedPin>& NewConnections,
	TArray<FFlowPinConnectionChange>& OutChanges)
{
	OutChanges.Reset();

	// Gather union of keys
	TSet<FName> Keys;
	Keys.Reserve(OldConnections.Num() + NewConnections.Num());

	for (const TPair<FName, FConnectedPin>& KVP : OldConnections)
	{
		Keys.Add(KVP.Key);
	}

	for (const TPair<FName, FConnectedPin>& KVP : NewConnections)
	{
		Keys.Add(KVP.Key);
	}

	for (const FName& PinName : Keys)
	{
		const FConnectedPin* OldConnectedPin = OldConnections.Find(PinName);
		const FConnectedPin* NewConnectedPin = NewConnections.Find(PinName);

		const bool bHadOld = (OldConnectedPin != nullptr);
		const bool bHasNew = (NewConnectedPin != nullptr);

		// If present in both and equal => no change
		if (bHadOld && bHasNew && (*OldConnectedPin == *NewConnectedPin))
		{
			continue;
		}

		UFlowNode* OldConnectedNode = nullptr;
		FName OldConnectedPinName;
		if (bHadOld)
		{
			OldConnectedNode = FlowAsset.GetNode(OldConnectedPin->NodeGuid);
			OldConnectedPinName = OldConnectedPin->PinName;
		}

		UFlowNode* NewConnectedNode = nullptr;
		FName NewConnectedPinName;
		if (bHasNew)
		{
			NewConnectedNode = FlowAsset.GetNode(NewConnectedPin->NodeGuid);
			NewConnectedPinName = NewConnectedPin->PinName;
		}

		FFlowPinConnectionChange Change =
			FFlowPinConnectionChange(
				PinName,
				OldConnectedNode,
				OldConnectedPinName,
				NewConnectedNode,
				NewConnectedPinName);

		OutChanges.Add(MoveTemp(Change));
	}
}

void UFlowNode::BroadcastEditorPinConnectionsChanged(const TArray<FFlowPinConnectionChange>& Changes)
{
	OnEditorPinConnectionsChanged(Changes);

	ForEachAddOn([&Changes](UFlowNodeAddOn& AddOn) -> EFlowForEachAddOnFunctionReturnValue
		{
			AddOn.OnEditorPinConnectionsChanged(Changes);

			return EFlowForEachAddOnFunctionReturnValue::Continue;
		});
}

void UFlowNode::SetConnections(const TMap<FName, FConnectedPin>& InConnections)
{
	const TMap<FName, FConnectedPin> OldConnections = Connections;

	// Early-out if maps are identical (cheap check first, then deep equality).
	// Note: TMap equality operator exists for comparable value types; keep explicit check to be safe.
	if (OldConnections.Num() == InConnections.Num())
	{
		bool bAllEqual = true;
		for (const TPair<FName, FConnectedPin>& KVP : OldConnections)
		{
			const FConnectedPin* Other = InConnections.Find(KVP.Key);
			if (!Other || !(*Other == KVP.Value))
			{
				bAllEqual = false;
				break;
			}
		}

		if (bAllEqual)
		{
			return;
		}
	}

	Connections = InConnections;

	// Compute per-pin deltas and broadcast to self + addons
	TArray<FFlowPinConnectionChange> Changes;
	BuildConnectionChangeList(*GetFlowAsset(), OldConnections, Connections, Changes);

	if (!Changes.IsEmpty())
	{
		BroadcastEditorPinConnectionsChanged(Changes);
	}
}
#endif

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
	// TODO (gtaylor) Maybe we make a blueprint accessible version with the FConnectedPin array access
	constexpr TArray<FConnectedPin>* ConnectedPins = nullptr;
	return FindInputPinConnections(PinName, bErrorIfPinNotFound, ConnectedPins);
}

bool UFlowNode::IsOutputConnected(const FName& PinName, bool bErrorIfPinNotFound) const
{
	// TODO (gtaylor) Maybe we make a blueprint accessible version with the FConnectedPin array access
	constexpr TArray<FConnectedPin>* ConnectedPins = nullptr;
	return FindOutputPinConnections(PinName, bErrorIfPinNotFound, ConnectedPins);
}

bool UFlowNode::FindFirstInputPinConnection(const FName& PinName, bool bErrorIfPinNotFound, FConnectedPin& FirstConnectedPin) const
{
	if (const FFlowPin* FlowPin = FindInputPinByName(PinName))
	{
		return FindFirstInputPinConnection(*FlowPin, FirstConnectedPin);
	}

	if (bErrorIfPinNotFound)
	{
		LogError(FString::Printf(TEXT("Unknown input pin %s"), *PinName.ToString()), EFlowOnScreenMessageType::Temporary);
	}

	return false;
}

bool UFlowNode::FindInputPinConnections(const FName& PinName, bool bErrorIfPinNotFound, TArray<FConnectedPin>* ConnectedPins) const
{
	if (const FFlowPin* FlowPin = FindInputPinByName(PinName))
	{
		return FindInputPinConnections(*FlowPin, ConnectedPins);
	}

	if (bErrorIfPinNotFound)
	{
		LogError(FString::Printf(TEXT("Unknown input pin %s"), *PinName.ToString()), EFlowOnScreenMessageType::Temporary);
	}

	return false;
}

bool UFlowNode::FindFirstOutputPinConnection(const FName& PinName, bool bErrorIfPinNotFound, FConnectedPin& FirstConnectedPin) const
{
	if (const FFlowPin* FlowPin = FindOutputPinByName(PinName))
	{
		return FindFirstOutputPinConnection(*FlowPin, FirstConnectedPin);
	}

	if (bErrorIfPinNotFound)
	{
		LogError(FString::Printf(TEXT("Unknown output pin %s"), *PinName.ToString()), EFlowOnScreenMessageType::Temporary);
	}

	return false;
}

bool UFlowNode::FindOutputPinConnections(const FName& PinName, bool bErrorIfPinNotFound, TArray<FConnectedPin>* ConnectedPins) const
{
	if (const FFlowPin* FlowPin = FindOutputPinByName(PinName))
	{
		return FindOutputPinConnections(*FlowPin, ConnectedPins);
	}

	if (bErrorIfPinNotFound)
	{
		LogError(FString::Printf(TEXT("Unknown output pin %s"), *PinName.ToString()), EFlowOnScreenMessageType::Temporary);
	}

	return false;
}

template <bool bExecIsCached>
bool UFlowNode::FindFirstPinConnection(
	const FFlowPin& FlowPin,
	const TArray<FFlowPin>& FlowPinArray,
	FConnectedPin& FirstConnectedPin) const
{
	if (!FlowPinArray.Contains(FlowPin.PinName))
	{
		return false;
	}

	const bool bUseCachedPath = (bExecIsCached == FlowPin.IsExecPin());
	if (bUseCachedPath)
	{
		// Cached category: fast lookup (0/1 connection)
		return FindConnectedNodeForPinCached(FlowPin.PinName, FirstConnectedPin);
	}
	else
	{
		// NOTE (gtaylor) For optimal perf, you should use the array signature when asking for uncached path
		// (aka optimal use should use this branch)
		TArray<FConnectedPin> ConnectedPins;
		if (FindConnectedNodeForPinUncached(FlowPin.PinName, &ConnectedPins))
		{
			check(ConnectedPins.Num() > 0);
			FirstConnectedPin = ConnectedPins[0];
			return true;
		}
		else
		{
			return false;
		}
	}
}

template <bool bExecIsCached>
bool UFlowNode::FindPinConnections(const FFlowPin& FlowPin, const TArray<FFlowPin>& FlowPinArray, TArray<FConnectedPin>* ConnectedPins) const
{
	if (!FlowPinArray.Contains(FlowPin.PinName))
	{
		return false;
	}

	const bool bUseCachedPath = bExecIsCached == FlowPin.IsExecPin();
	if (bUseCachedPath)
	{
		// NOTE (gtaylor) For optimal perf, you should use the non-array signature when asking for cached path
		// (aka optimal use should use this branch)
		FConnectedPin ConnectedPin;
		const bool bFoundPin = FindConnectedNodeForPinCached(FlowPin.PinName, ConnectedPin);
		if (bFoundPin && ConnectedPins)
		{
			ConnectedPins->Add(ConnectedPin);
		}

		return bFoundPin;
	}
	else
	{
		// We don't cache the output data pins for fast lookup in Connections, so use the slow path for them:

		return FindConnectedNodeForPinUncached(FlowPin.PinName, ConnectedPins);
	}
}

bool UFlowNode::FindFirstInputPinConnection(const FFlowPin& FlowPin, FConnectedPin& FirstConnectedPin) const
{
	// Exec Input pins - not cached
	// Data Input pins - cached
	constexpr bool bIsExecCached = false;
	return FindFirstPinConnection<bIsExecCached>(FlowPin, InputPins, FirstConnectedPin);
}

bool UFlowNode::FindInputPinConnections(const FFlowPin& FlowPin, TArray<FConnectedPin>* ConnectedPins) const
{
	// Exec Input pins - not cached
	// Data Input pins - cached
	constexpr bool bIsExecCached = false;
	return FindPinConnections<bIsExecCached>(FlowPin, InputPins, ConnectedPins);
}

bool UFlowNode::FindFirstOutputPinConnection(const FFlowPin& FlowPin, FConnectedPin& FirstConnectedPin) const
{
	// Exec Output pins - cached
	// Data Output pins - not cached
	constexpr bool bIsExecCached = true;
	return FindFirstPinConnection<bIsExecCached>(FlowPin, OutputPins, FirstConnectedPin);
}

bool UFlowNode::FindOutputPinConnections(const FFlowPin& FlowPin, TArray<FConnectedPin>* ConnectedPins) const
{
	// Exec Output pins - cached
	// Data Output pins - not cached
	constexpr bool bIsExecCached = true;
	return FindPinConnections<bIsExecCached>(FlowPin, OutputPins, ConnectedPins);
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

bool UFlowNode::FindConnectedNodeForPinCached(const FName& FlowPinName, FConnectedPin& ConnectedPin) const
{
	// NOTE (gtaylor) The Connections array only caches:
	// - exec output pins
	// - data input pins
	// In both cases, there must be only one connection (due to schema rules in Flow).
	// For the opposite direction (exec inputs, data outputs, the uncached version must be used.
	const FConnectedPin* FoundConnectedPin = Connections.Find(FlowPinName);
	if (FoundConnectedPin)
	{
		ConnectedPin = *FoundConnectedPin;

		return true;
	}

	return false;
}

bool UFlowNode::FindConnectedNodeForPinUncached(const FName& PinName, TArray<FConnectedPin>* ConnectedPins) const
{
	const UFlowAsset* FlowAsset = GetFlowAsset();

	if (!IsValid(FlowAsset))
	{
		return false;
	}

	check(!ConnectedPins || ConnectedPins->IsEmpty());

	for (const TPair<FGuid, UFlowNode*>& Pair : ObjectPtrDecay(FlowAsset->Nodes))
	{
		const UFlowNode* ConnectedFromFlowNode = Pair.Value;

		if (!IsValid(ConnectedFromFlowNode))
		{
			continue;
		}

		for (const TPair<FName, FConnectedPin>& Connection : ConnectedFromFlowNode->Connections)
		{
			const FConnectedPin& ConnectedPinStruct = Connection.Value;

			if (ConnectedPinStruct.NodeGuid == NodeGuid && ConnectedPinStruct.PinName == PinName)
			{
				if (ConnectedPins)
				{
					ConnectedPins->Add(ConnectedPinStruct);
				}
				else
				{
					// Early return if not collecting the ConnectedPins, since only connected true/false matters
					return true;
				}
			}
		}
	}

	if (ConnectedPins && !ConnectedPins->IsEmpty())
	{
		return true;
	}

	return false;
}

TArray<FConnectedPin> UFlowNode::GetKnownConnectionsToPin(const FConnectedPin& Pin) const
{
	TArray<FConnectedPin> ConnectedPins;

	if (Pin.NodeGuid == NodeGuid)
	{
		const FConnectedPin& Connection = Connections.FindRef(Pin.PinName);
		if (Connection.NodeGuid.IsValid())
		{
			ConnectedPins.Add(Connection);
		}
	}
	else
	{
		for (const TPair<FName, FConnectedPin>& Connection : Connections)
		{
			if (Connection.Value.NodeGuid == Pin.NodeGuid && Connection.Value.PinName == Pin.PinName)
			{
				ConnectedPins.Emplace(NodeGuid, Connection.Key);
			}
		}
	}
	
	return ConnectedPins;
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
			(void)FlowAssetTemplate->OnPinTriggered.ExecuteIfBound(this, PinName);
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
			if (GetDefault<UFlowSettings>()->bLogOnSignalDisabled)
			{
				LogNote(FString::Printf(TEXT("Node disabled while triggering input %s"), *PinName.ToString()));
			}
			break;
		case EFlowSignalMode::PassThrough:
			if (GetDefault<UFlowSettings>()->bLogOnSignalPassthrough)
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
	if (HasFinished())
	{
		// do not trigger output if node is already finished or aborted
		LogError(TEXT("Trying to TriggerOutput after finished or aborted"), EFlowOnScreenMessageType::Disabled);
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
			FlowAssetTemplate->OnPinTriggered.ExecuteIfBound(this, PinName);
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
		GetFlowAsset()->TriggerInput(FlowPin.NodeGuid, FlowPin.PinName, FConnectedPin(GetGuid(), PinName));
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

bool UFlowNode::ShouldSave_Implementation()
{
	return GetActivationState() == EFlowNodeState::Active;
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
