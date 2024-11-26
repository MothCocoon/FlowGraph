// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowAsset.h"

#include "FlowLogChannels.h"
#include "FlowSettings.h"
#include "FlowSubsystem.h"

#include "AddOns/FlowNodeAddOn.h"
#include "Interfaces/FlowDataPinGeneratorNodeInterface.h"
#include "Nodes/FlowNodeBase.h"
#include "Nodes/Route/FlowNode_CustomInput.h"
#include "Nodes/Route/FlowNode_CustomOutput.h"
#include "Nodes/Route/FlowNode_Start.h"
#include "Nodes/Route/FlowNode_SubGraph.h"

#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"

FString UFlowAsset::ValidationError_NodeClassNotAllowed = TEXT("Node class {0} is not allowed in this asset.");
FString UFlowAsset::ValidationError_NullNodeInstance = TEXT("Node with GUID {0} is NULL");
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowAsset)

UFlowAsset::UFlowAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bWorldBound(true)
#if WITH_EDITOR
	, FlowGraph(nullptr)
#endif
	, AllowedNodeClasses({UFlowNodeBase::StaticClass()})
	, AllowedInSubgraphNodeClasses({UFlowNode_SubGraph::StaticClass()})
	, bStartNodePlacedAsGhostNode(false)
	, TemplateAsset(nullptr)
	, FinishPolicy(EFlowFinishPolicy::Keep)
{
	if (!AssetGuid.IsValid())
	{
		AssetGuid = FGuid::NewGuid();
	}

	ExpectedOwnerClass = UFlowSettings::Get()->GetDefaultExpectedOwnerClass();
}

#if WITH_EDITOR
void UFlowAsset::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UFlowAsset* This = CastChecked<UFlowAsset>(InThis);
	Collector.AddReferencedObject(This->FlowGraph, This);

	Super::AddReferencedObjects(InThis, Collector);
}

void UFlowAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomInputs)
		|| PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowAsset, CustomOutputs)))
	{
		OnSubGraphReconstructionRequested.ExecuteIfBound();
	}
}

void UFlowAsset::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		AssetGuid = FGuid::NewGuid();
		Nodes.Empty();
	}
}

void UFlowAsset::PostLoad()
{
	Super::PostLoad();

	// If we removed or moved a flow node blueprint (and there is no redirector) we might loose the reference to it resulting
	// in null pointers in the Nodes FGUID->UFlowNode* Map. So here we iterate over all the Nodes and remove all pairs that
	// are nulled out.

	TSet<FGuid> NodesToRemoveGUID;

	for (auto& [Guid, Node] : GetNodes())
	{
		if (!IsValid(Node))
		{
			NodesToRemoveGUID.Emplace(Guid);
		}
	}

	for (const FGuid& Guid : NodesToRemoveGUID)
	{
		UnregisterNode(Guid);
	}
}

EDataValidationResult UFlowAsset::ValidateAsset(FFlowMessageLog& MessageLog)
{
	// validate nodes
	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (IsValid(Node.Value))
		{
			FText FailureReason;
			if (!IsNodeOrAddOnClassAllowed(Node.Value->GetClass(), &FailureReason))
			{
				const FString ErrorMsg =
					FailureReason.IsEmpty()
						? FString::Format(*ValidationError_NodeClassNotAllowed, {*Node.Value->GetClass()->GetName()})
						: FailureReason.ToString();

				MessageLog.Error(*ErrorMsg, Node.Value);
			}

			Node.Value->ValidationLog.Messages.Empty();
			if (Node.Value->ValidateNode() == EDataValidationResult::Invalid)
			{
				MessageLog.Messages.Append(Node.Value->ValidationLog.Messages);
			}
		}
		else
		{
			const FString ErrorMsg = FString::Format(*ValidationError_NullNodeInstance, {*Node.Key.ToString()});
			MessageLog.Error(*ErrorMsg, this);
		}
	}

	return MessageLog.Messages.Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

bool UFlowAsset::IsNodeOrAddOnClassAllowed(const UClass* FlowNodeOrAddOnClass, FText* OutOptionalFailureReason) const
{
	if (!IsValid(FlowNodeOrAddOnClass))
	{
		return false;
	}

	if (!CanFlowNodeClassBeUsedByFlowAsset(*FlowNodeOrAddOnClass))
	{
		return false;
	}

	if (!CanFlowAssetUseFlowNodeClass(*FlowNodeOrAddOnClass))
	{
		return false;
	}

	// Confirm plugin reference restrictions are being respected
	if (!CanFlowAssetReferenceFlowNode(*FlowNodeOrAddOnClass, OutOptionalFailureReason))
	{
		return false;
	}

	return true;
}

bool UFlowAsset::CanFlowNodeClassBeUsedByFlowAsset(const UClass& FlowNodeClass) const
{
	UFlowNode* NodeDefaults = Cast<UFlowNode>(FlowNodeClass.GetDefaultObject());
	if (!NodeDefaults)
	{
		check(FlowNodeClass.IsChildOf<UFlowNodeAddOn>());

		// AddOns don't have the AllowedAssetClasses/DeniedAssetClasses 
		// (yet?  maybe we move it up to the base?)
		return true;
	}

	// UFlowNode class limits which UFlowAsset class can use it
	const TArray<TSubclassOf<UFlowAsset>>& DeniedAssetClasses = NodeDefaults->DeniedAssetClasses;
	for (const UClass* DeniedAssetClass : DeniedAssetClasses)
	{
		if (DeniedAssetClass && GetClass()->IsChildOf(DeniedAssetClass))
		{
			return false;
		}
	}

	const TArray<TSubclassOf<UFlowAsset>>& AllowedAssetClasses = NodeDefaults->AllowedAssetClasses;
	if (AllowedAssetClasses.Num() > 0)
	{
		bool bAllowedInAsset = false;
		for (const UClass* AllowedAssetClass : AllowedAssetClasses)
		{
			if (AllowedAssetClass && GetClass()->IsChildOf(AllowedAssetClass))
			{
				bAllowedInAsset = true;
				break;
			}
		}
		if (!bAllowedInAsset)
		{
			return false;
		}
	}

	return true;
}

bool UFlowAsset::CanFlowAssetUseFlowNodeClass(const UClass& FlowNodeClass) const
{
	// UFlowAsset class can limit which UFlowNodeBase classes can be used
	if (IsFlowNodeClassInDeniedClasses(FlowNodeClass))
	{
		return false;
	}

	if (!IsFlowNodeClassInAllowedClasses(FlowNodeClass))
	{
		return false;
	}

	return true;
}

bool UFlowAsset::IsFlowNodeClassInDeniedClasses(const UClass& FlowNodeClass) const
{
	for (const TSubclassOf<UFlowNodeBase> DeniedNodeClass : DeniedNodeClasses)
	{
		if (DeniedNodeClass && FlowNodeClass.IsChildOf(DeniedNodeClass))
		{
			// Subclasses of a DeniedNodeClass can opt back in to being allowed
			if (!IsFlowNodeClassInAllowedClasses(FlowNodeClass, DeniedNodeClass))
			{
				return true;
			}
		}
	}

	return false;
}

bool UFlowAsset::IsFlowNodeClassInAllowedClasses(const UClass& FlowNodeClass, const TSubclassOf<UFlowNodeBase> RequiredAncestor) const
{
	if (AllowedNodeClasses.Num() > 0)
	{
		bool bAllowedInAsset = false;
		for (const TSubclassOf<UFlowNodeBase> AllowedNodeClass : AllowedNodeClasses)
		{
			// If a RequiredAncestor is provided, the AllowedNodeClass must be a subclass of the RequiredAncestor
			if (AllowedNodeClass && 
				FlowNodeClass.IsChildOf(AllowedNodeClass) &&
				(!RequiredAncestor || AllowedNodeClass->IsChildOf(RequiredAncestor)))
			{
				bAllowedInAsset = true;

				break;
			}
		}

		if (!bAllowedInAsset)
		{
			return false;
		}
	}

	return true;
}

bool UFlowAsset::CanFlowAssetReferenceFlowNode(const UClass& FlowNodeClass, FText* OutOptionalFailureReason) const
{
	if (!GEditor || !IsValid(&FlowNodeClass))
	{
		return false;
	}

	FAssetReferenceFilterContext AssetReferenceFilterContext;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
	AssetReferenceFilterContext.ReferencingAssets.Add(FAssetData(this));
#else
	AssetReferenceFilterContext.AddReferencingAsset(FAssetData(this));
#endif

	// Confirm plugin reference restrictions are being respected
	const TSharedPtr<IAssetReferenceFilter> FlowAssetReferenceFilter = GEditor->MakeAssetReferenceFilter(AssetReferenceFilterContext);
	if (FlowAssetReferenceFilter.IsValid())
	{
		const FAssetData FlowNodeAssetData(&FlowNodeClass);
		if (!FlowAssetReferenceFilter->PassesFilter(FlowNodeAssetData, OutOptionalFailureReason))
		{
			return false;
		}
	}

	return true;
}

TSharedPtr<IFlowGraphInterface> UFlowAsset::FlowGraphInterface = nullptr;

void UFlowAsset::SetFlowGraphInterface(TSharedPtr<IFlowGraphInterface> InFlowAssetEditor)
{
	check(!FlowGraphInterface.IsValid());
	FlowGraphInterface = InFlowAssetEditor;
}

UFlowNode* UFlowAsset::CreateNode(const UClass* NodeClass, UEdGraphNode* GraphNode)
{
	UFlowNode* NewNode = NewObject<UFlowNode>(this, NodeClass, NAME_None, RF_Transactional);
	NewNode->SetGraphNode(GraphNode);

	RegisterNode(GraphNode->NodeGuid, NewNode);
	return NewNode;
}

void UFlowAsset::RegisterNode(const FGuid& NewGuid, UFlowNode* NewNode)
{
	NewNode->SetGuid(NewGuid);
	Nodes.Emplace(NewGuid, NewNode);

	HarvestNodeConnections();
	(void)TryUpdateManagedFlowPinsForNode(*NewNode);
}

void UFlowAsset::UnregisterNode(const FGuid& NodeGuid)
{
	Nodes.Remove(NodeGuid);
	Nodes.Compact();

	HarvestNodeConnections();

	MarkPackageDirty();
}

void UFlowAsset::HarvestNodeConnections()
{
	TMap<FName, FConnectedPin> Connections;
	bool bGraphDirty = false;

	// last moment to remove invalid nodes
	for (auto NodeIt = Nodes.CreateIterator(); NodeIt; ++NodeIt)
	{
		const TPair<FGuid, UFlowNode*>& Pair = *NodeIt;
		if (Pair.Value == nullptr)
		{
			NodeIt.RemoveCurrent();
			bGraphDirty = true;
		}
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : ObjectPtrDecay(Nodes))
	{
		UFlowNode* FlowNode = Pair.Value;
		TMap<FName, FConnectedPin> FoundConnections;
		const TArray<UEdGraphPin*>& GraphNodePins = FlowNode->GetGraphNode()->Pins;

		for (const UEdGraphPin* ThisPin : GraphNodePins)
		{
			const bool bIsExecPin = FFlowPin::IsExecPinCategory(ThisPin->PinType.PinCategory);
			const bool bIsDataPin = FFlowPin::IsDataPinCategory(ThisPin->PinType.PinCategory);
			const bool bIsOutputPin = (ThisPin->Direction == EGPD_Output);
			const bool bIsInputPin = (ThisPin->Direction == EGPD_Input);
			const bool bHasAtLeastOneConnection = ThisPin->LinkedTo.Num() > 0;

			if (bIsExecPin && bIsOutputPin && bHasAtLeastOneConnection)
			{
				// For Exec Pins, harvest the 0th connection (we should have only 1 connection, because of schema rules)
				if (const UEdGraphPin* LinkedPin = ThisPin->LinkedTo[0])
				{
					const UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
					FoundConnections.Add(ThisPin->PinName, FConnectedPin(LinkedNode->NodeGuid, LinkedPin->PinName));
				}
			}
			else if (bIsDataPin && bIsInputPin && bHasAtLeastOneConnection)
			{
				// For Data Pins, harvest the 0th connection (we should have only 1 connection, because of schema rules)
				if (const UEdGraphPin* LinkedPin = ThisPin->LinkedTo[0])
				{
					const UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
					FoundConnections.Add(ThisPin->PinName, FConnectedPin(LinkedNode->NodeGuid, LinkedPin->PinName));
				}
			}
		}

		// This check exists to ensure that we don't mark graph dirty, if none of connections changed
		// Optimization: we need check it only until the first node would be marked dirty, as this already marks Flow Asset package dirty
		if (bGraphDirty == false)
		{
			const TMap<FName, FConnectedPin>& OldConnections = FlowNode->Connections;

			if (FoundConnections.Num() != OldConnections.Num())
			{
				bGraphDirty = true;
			}
			else
			{
				for (const TPair<FName, FConnectedPin>& FoundConnection : FoundConnections)
				{
					if (const FConnectedPin* OldConnection = OldConnections.Find(FoundConnection.Key))
					{
						if (FoundConnection.Value != *OldConnection)
						{
							bGraphDirty = true;
							break;
						}
					}
					else
					{
						bGraphDirty = true;
						break;
					}
				}
			}
		}

		if (bGraphDirty)
		{
			FlowNode->SetFlags(RF_Transactional);
			FlowNode->Modify();

			FlowNode->SetConnections(FoundConnections);
			FlowNode->PostEditChange();
		}
	}

	// NOTE (gtaylor) @mothdoctor, do we need to do anything with bGraphDirty here?  
	// It's scope seems like we wanted to do something at this point.
}

bool UFlowAsset::TryUpdateManagedFlowPinsForNode(UFlowNode& FlowNode)
{
	const UClass* FlowNodeClass = FlowNode.GetClass();
	if (!IsValid(FlowNodeClass))
	{
		return false;
	}

	// Setup the working data struct
	FFlowHarvestDataPinsWorkingData WorkingData =
		FFlowHarvestDataPinsWorkingData(
			FlowNode,
			FlowNode.GetPinNameToBoundPropertyNameMap(),
			FlowNode.GetAutoInputDataPins(),
			FlowNode.GetAutoOutputDataPins());

	// Some nodes can auto-generate some pins directly, 
	// so let them append their pins into our arrays first.
	if (IFlowDataPinGeneratorNodeInterface* AutoGeneratorNode = Cast<IFlowDataPinGeneratorNodeInterface>(&FlowNode))
	{
		AutoGeneratorNode->AutoGenerateDataPins(
			WorkingData.PinNameToBoundPropertyNameMapNext,
			WorkingData.AutoInputDataPinsNext,
			WorkingData.AutoOutputDataPinsNext);
	}

	// Try to harvest pins to auto-generate and/or bind to for each property in the flow node
	for (TFieldIterator<FProperty> PropertyIt(FlowNodeClass); PropertyIt; ++PropertyIt)
	{
		HarvestFlowPinMetadataForProperty(*PropertyIt, WorkingData);
	}

	// Check if the pin name to bound property map changed
	WorkingData.bPinNameMapChanged |= WorkingData.DidPinNameToBoundPropertyNameMapChange();

	// If the auto-generated data pins array changed, it counts as dirty as well
	const bool bAutoInputDataPinsChanged = WorkingData.DidAutoInputDataPinsChange();
	const bool bAutoOutputDataPinsChanged = WorkingData.DidAutoOutputDataPinsChange();

	if (WorkingData.bPinNameMapChanged || bAutoInputDataPinsChanged || bAutoOutputDataPinsChanged)
	{
		FlowNode.SetFlags(RF_Transactional);
		FlowNode.Modify();

		// Lock-in the data that changed.
		if (WorkingData.bPinNameMapChanged)
		{
			FlowNode.SetPinNameToBoundPropertyNameMap(WorkingData.PinNameToBoundPropertyNameMapNext);
		}

		if (bAutoInputDataPinsChanged || bAutoOutputDataPinsChanged)
		{
			if (bAutoInputDataPinsChanged)
			{
				FlowNode.SetAutoInputDataPins(WorkingData.AutoInputDataPinsNext);
			}

			if (bAutoOutputDataPinsChanged)
			{
				FlowNode.SetAutoOutputDataPins(WorkingData.AutoOutputDataPinsNext);
			}

			if (FlowNode.GraphNode)
			{
				FlowNode.OnReconstructionRequested.ExecuteIfBound();
			}
		}

		FlowNode.PostEditChange();

		return true;
	}

	return false;
}

void UFlowAsset::HarvestFlowPinMetadataForProperty(const FProperty* Property, FFlowHarvestDataPinsWorkingData& InOutData)
{
	FText PinDisplayName = Property->GetDisplayNameText();
	const FName& PinAuthoredName = Property->GetFName();

	// Default assumption is the pin is will be a output pin, if no metadata is specified (ie, bIsSourceForOutputPin == false),
	// because this is the most common case (the auto-generated input-pin-from-property case is only for defaulting)
	TArray<FFlowPin>* FlowPinArray = &InOutData.AutoOutputDataPinsNext;

	const FString* SourceForOutputFlowPinName = Property->FindMetaData(FFlowPin::MetadataKey_SourceForOutputFlowPin);
	const FString* DefaultForInputFlowPinName = Property->FindMetaData(FFlowPin::MetadataKey_DefaultForInputFlowPin);

	if (SourceForOutputFlowPinName && DefaultForInputFlowPinName)
	{
		LogError(
			FString::Printf(TEXT("Error.  A property cannot be both a %s and %s"),
			                *FFlowPin::MetadataKey_SourceForOutputFlowPin.ToString(),
			                *FFlowPin::MetadataKey_DefaultForInputFlowPin.ToString()),
			InOutData.FlowNode);

		return;
	}

	if (SourceForOutputFlowPinName)
	{
		const FString SpecifyOutputPinNameString = *SourceForOutputFlowPinName;

		if (SpecifyOutputPinNameString.Len() > 0)
		{
			// Replace the default PinDisplayName with the name specified in the Metadata value
			PinDisplayName = FText::FromString(SpecifyOutputPinNameString);
		}
	}
	else if (DefaultForInputFlowPinName)
	{
		const FString SpecifyInputPinNameString = *DefaultForInputFlowPinName;

		if (SpecifyInputPinNameString.Len() > 0)
		{
			// Replace the default PinDisplayName with the name specified in the Metadata value
			PinDisplayName = FText::FromString(SpecifyInputPinNameString);
		}

		// If the property is a Default Input for a data pin, then we need to generate the pin in the
		// Input Pins array.
		FlowPinArray = &InOutData.AutoInputDataPinsNext;
	}

	// Check for relevant metadata keys on the property's USTRUCT()
	const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
	if (StructProperty && StructProperty->Struct)
	{
		const UScriptStruct* ScriptStruct = StructProperty->Struct;

		// We also look in the USTRUCT for DefaultForInputFlowPin
		DefaultForInputFlowPinName = ScriptStruct->FindMetaData(FFlowPin::MetadataKey_DefaultForInputFlowPin);
		if (DefaultForInputFlowPinName)
		{
			// If the property is a Default Input for a data pin, then we need to generate the pin in the
			// Input Pins array.
			FlowPinArray = &InOutData.AutoInputDataPinsNext;
		}

		if (const FString* AutoPinType = ScriptStruct->FindMetaData(FFlowPin::MetadataKey_FlowPinType))
		{
			const bool bIsInputPin = DefaultForInputFlowPinName != nullptr;

			// Auto-generate the pin for this property
			if (!TryCreateFlowDataPinFromMetadataValue(*AutoPinType, *InOutData.FlowNode, *Property, PinDisplayName, bIsInputPin, FlowPinArray))
			{
				LogError(FString::Printf(TEXT("Error.  Unknown value %s for metadata %s"), **AutoPinType, *FFlowPin::MetadataKey_FlowPinType.ToString()), InOutData.FlowNode);

				return;
			}

			// Add a binding for the new pin to its property
			AddDataPinPropertyBindingToMap(
				PinAuthoredName,
				Property->GetFName(),
				InOutData);

			return;
		}
	}

	const FString* AutoPinType = Property->FindMetaData(FFlowPin::MetadataKey_FlowPinType);

	if (!SourceForOutputFlowPinName && !DefaultForInputFlowPinName && !AutoPinType)
	{
		// If we didn't detect any the relevent metadata keys, we can exit early

		return;
	}

	if (AutoPinType)
	{
		// Auto-generate the desired pin for this property
		const bool bIsInputPin = DefaultForInputFlowPinName != nullptr;

		if (!TryCreateFlowDataPinFromMetadataValue(*AutoPinType, *InOutData.FlowNode, *Property, PinDisplayName, bIsInputPin, FlowPinArray))
		{
			LogError(FString::Printf(TEXT("Unknown value %s for metadata %s"), **AutoPinType, *FFlowPin::MetadataKey_FlowPinType.ToString()), InOutData.FlowNode);

			return;
		}
	}
	else if (SourceForOutputFlowPinName)
	{
		// Bind to the output data pin to source from the property (but do not auto-generate the pin)

		FFlowPin* FoundFlowPin = InOutData.FlowNode->FindOutputPinByName(PinAuthoredName);
		if (!FoundFlowPin)
		{
			LogError(FString::Printf(TEXT("Could not find bound data pin named %s for property %s"), *PinAuthoredName.ToString(), *Property->GetName()), InOutData.FlowNode);

			return;
		}
	}
	else if (DefaultForInputFlowPinName)
	{
		// Bind to the input data pin to default its value from the property (but do not auto-generate the pin)

		FFlowPin* FoundFlowPin = InOutData.FlowNode->FindInputPinByName(PinAuthoredName);
		if (!FoundFlowPin)
		{
			LogError(FString::Printf(TEXT("Could not find bound data pin named %s for property %s"), *PinAuthoredName.ToString(), *Property->GetName()), InOutData.FlowNode);

			return;
		}
	}

	// Add a binding for the data pin to its property
	AddDataPinPropertyBindingToMap(
		PinAuthoredName,
		Property->GetFName(),
		InOutData);
}

void UFlowAsset::AddDataPinPropertyBindingToMap(
	const FName& PinAuthoredName,
	const FName& PropertyAuthoredName,
	FFlowHarvestDataPinsWorkingData& InOutData)
{
	// Add a new entry in the map for this DataPin name to the property it sources from
	InOutData.PinNameToBoundPropertyNameMapNext.Add(PinAuthoredName, PropertyAuthoredName);
}

template <typename TEnumProperty, typename TVectorProperty, typename TRotatorProperty, typename TTransformProperty, typename TGameplayTagProperty, typename TGameplayTagContainerProperty,
          typename TInstancedStructProperty, typename TObjectProperty, typename TClassProperty>
void AddPinForPinType(EFlowPinType PinType, UFlowNode& FlowNode, const FProperty& Property, const FText& PinDisplayName, TArray<FFlowPin>* InOutDataPinsNext)
{
	const FName& PinAuthoredName = Property.GetFName();

	// Some of the FlowPinTypes require a SubCategoryObject to fully define the type, so
	// we need to find that for the cases that it applies to.

	FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);

	FFlowPin& NewFlowPin = InOutDataPinsNext->Add_GetRef(FFlowPin(PinAuthoredName, PinDisplayName));
	switch (PinType)
	{
	case EFlowPinType::Enum:
		{
			UEnum* EnumClass = nullptr;

			if (const FStructProperty* StructProperty = CastField<FStructProperty>(&Property))
			{
				// Check for a wrapper struct to get the enum data from
				const UStruct* ScriptStruct = TEnumProperty::StaticStruct();
				if (StructProperty->Struct == ScriptStruct)
				{
					TEnumProperty ValueStruct;
					StructProperty->GetValue_InContainer(&FlowNode, &ValueStruct);

					EnumClass = ValueStruct.EnumClass;
				}
			}
			else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(&Property))
			{
				// Get the enum data from the FEnumProperty
				EnumClass = EnumProperty->GetEnum();
			}

			NewFlowPin.SetPinType(PinType, EnumClass);
		}
		break;

	case EFlowPinType::Vector:
		{
			UScriptStruct* ValueStructType = FFlowDataPinProperty::FindScriptStructForFlowDataPinProperty<TVectorProperty, FVector>(Property);
			NewFlowPin.SetPinType(PinType, ValueStructType);
		}
		break;

	case EFlowPinType::Rotator:
		{
			UScriptStruct* ValueStructType = FFlowDataPinProperty::FindScriptStructForFlowDataPinProperty<TRotatorProperty, FRotator>(Property);
			NewFlowPin.SetPinType(PinType, ValueStructType);
		}
		break;

	case EFlowPinType::Transform:
		{
			UScriptStruct* ValueStructType = FFlowDataPinProperty::FindScriptStructForFlowDataPinProperty<TTransformProperty, FTransform>(Property);
			NewFlowPin.SetPinType(PinType, ValueStructType);
		}
		break;

	case EFlowPinType::GameplayTag:
		{
			UScriptStruct* ValueStructType = FFlowDataPinProperty::FindScriptStructForFlowDataPinProperty<TGameplayTagProperty, FGameplayTag>(Property);
			NewFlowPin.SetPinType(PinType, ValueStructType);
		}
		break;

	case EFlowPinType::GameplayTagContainer:
		{
			UScriptStruct* ValueStructType = FFlowDataPinProperty::FindScriptStructForFlowDataPinProperty<TGameplayTagContainerProperty, FGameplayTagContainer>(Property);
			NewFlowPin.SetPinType(PinType, ValueStructType);
		}
		break;

	case EFlowPinType::InstancedStruct:
		{
			UScriptStruct* ValueStructType = FFlowDataPinProperty::FindScriptStructForFlowDataPinProperty<TInstancedStructProperty, FInstancedStruct>(Property);
			NewFlowPin.SetPinType(PinType, ValueStructType);
		}
		break;

	case EFlowPinType::Object:
		{
			UClass* Class = nullptr;
			if (const FStructProperty* StructProperty = CastField<FStructProperty>(&Property))
			{
				const UStruct* ScriptStruct = TObjectProperty::StaticStruct();
				static const UStruct* SoftObjectPathStruct = TBaseStructure<FSoftObjectPath>::Get();

				if (StructProperty->Struct == ScriptStruct)
				{
					TObjectProperty ValueStruct;
					StructProperty->GetValue_InContainer(&FlowNode, &ValueStruct);

					// Get the Object property's base UClass from the FFlowDataPinProperty
					Class = ValueStruct.DeriveObjectClass(*StructProperty);
				}
				else if (StructProperty->Struct == SoftObjectPathStruct)
				{
					// Get the Object property's base UClass from the struct property's MetaData
					Class = FFlowDataPinOutputProperty_Object::TryGetObjectClassFromProperty(*StructProperty);
				}
			}
			else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(&Property))
			{
				// Get the Object property's base UClass from the property's MetaData
				Class = ObjectProperty->PropertyClass;
			}
			else if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(&Property))
			{
				// Get the Object property's base UClass from the property's MetaData
				Class = SoftObjectProperty->PropertyClass;
			}
			else if (const FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(&Property))
			{
				// Get the Object property's base UClass from the property's MetaData
				Class = WeakObjectProperty->PropertyClass;
			}
			else if (const FLazyObjectProperty* LazyObjectProperty = CastField<FLazyObjectProperty>(&Property))
			{
				// Get the Object property's base UClass from the property's MetaData
				Class = LazyObjectProperty->PropertyClass;
			}

			NewFlowPin.SetPinType(PinType, Class);
		}
		break;

	case EFlowPinType::Class:
		{
			UClass* Class = nullptr;
			if (const FStructProperty* StructProperty = CastField<FStructProperty>(&Property))
			{
				const UStruct* ScriptStruct = TClassProperty::StaticStruct();
				static const UStruct* SoftClassPathStruct = TBaseStructure<FSoftClassPath>::Get();

				if (StructProperty->Struct == ScriptStruct)
				{
					TClassProperty ValueStruct;
					StructProperty->GetValue_InContainer(&FlowNode, &ValueStruct);

					// Get the Class property's base UClass from the FFlowDataPinProperty
					Class = ValueStruct.DeriveMetaClass(*StructProperty);
				}
				else if (StructProperty->Struct == SoftClassPathStruct)
				{
					// Get the Class property's base UClass from the struct property's MetaData
					Class = FFlowDataPinOutputProperty_Class::TryGetMetaClassFromProperty(*StructProperty);
				}
			}
			else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(&Property))
			{
				// Get the Class property's base UClass from the property's MetaData
				Class = ClassProperty->MetaClass;
			}
			else if (const FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(&Property))
			{
				// Get the Class property's base UClass from the property's MetaData
				Class = SoftClassProperty->MetaClass;
			}

			NewFlowPin.SetPinType(PinType, Class);
		}
		break;

	default:
		{
			NewFlowPin.SetPinType(PinType);
		}
		break;
	}
}

bool UFlowAsset::TryCreateFlowDataPinFromMetadataValue(
	const FString& MetadataValue,
	UFlowNode& FlowNode,
	const FProperty& Property,
	const FText& PinDisplayName,
	const bool bIsInputPin,
	TArray<FFlowPin>* InOutDataPinsNext) const
{
	check(InOutDataPinsNext);

	const TArray<FName>& CachedEnumValueNames = FFlowPin::GetFlowPinTypeEnumValuesWithoutSpaces();

	const FName MetadataValueAsName = FName(MetadataValue);

	for (EFlowPinType PinType : TEnumRange<EFlowPinType>())
	{
		const int32 PinTypeAsInt = FlowEnum::ToInt(PinType);
		check(CachedEnumValueNames.IsValidIndex(PinTypeAsInt));
		const FName& EnumValueAsName = CachedEnumValueNames[PinTypeAsInt];

		if (MetadataValueAsName == EnumValueAsName)
		{
			if (bIsInputPin)
			{
				AddPinForPinType<
					FFlowDataPinInputProperty_Enum,
					FFlowDataPinInputProperty_Vector,
					FFlowDataPinInputProperty_Rotator,
					FFlowDataPinInputProperty_Transform,
					FFlowDataPinInputProperty_GameplayTag,
					FFlowDataPinInputProperty_GameplayTagContainer,
					FFlowDataPinInputProperty_InstancedStruct,
					FFlowDataPinInputProperty_Object,
					FFlowDataPinInputProperty_Class>(
					PinType,
					FlowNode,
					Property,
					PinDisplayName,
					InOutDataPinsNext);
			}
			else
			{
				AddPinForPinType<
					FFlowDataPinOutputProperty_Enum,
					FFlowDataPinOutputProperty_Vector,
					FFlowDataPinInputProperty_Rotator,
					FFlowDataPinOutputProperty_Transform,
					FFlowDataPinOutputProperty_GameplayTag,
					FFlowDataPinOutputProperty_GameplayTagContainer,
					FFlowDataPinOutputProperty_InstancedStruct,
					FFlowDataPinOutputProperty_Object,
					FFlowDataPinOutputProperty_Class>(
					PinType,
					FlowNode,
					Property,
					PinDisplayName,
					InOutDataPinsNext);
			}

			return true;
		}
	}

	// Subclasses of UFlowAsset can extend the supported MetadataValues -> FFlowPin mappings
	return false;
}

#endif

UFlowNode* UFlowAsset::GetDefaultEntryNode() const
{
	UFlowNode* FirstStartNode = nullptr;

	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (UFlowNode_Start* StartNode = Cast<UFlowNode_Start>(Node.Value))
		{
			if (StartNode->GatherConnectedNodes().Num() > 0)
			{
				return StartNode;
			}
			else if (FirstStartNode == nullptr)
			{
				FirstStartNode = StartNode;
			}
		}
	}

	// If none of the found start nodes have connections, fallback to the first start node we found
	return FirstStartNode;
}

#if WITH_EDITOR
void UFlowAsset::AddCustomInput(const FName& EventName)
{
	if (!CustomInputs.Contains(EventName))
	{
		CustomInputs.Add(EventName);
	}
}

void UFlowAsset::RemoveCustomInput(const FName& EventName)
{
	if (CustomInputs.Contains(EventName))
	{
		CustomInputs.Remove(EventName);
	}
}

void UFlowAsset::AddCustomOutput(const FName& EventName)
{
	if (!CustomOutputs.Contains(EventName))
	{
		CustomOutputs.Add(EventName);
	}
}

void UFlowAsset::RemoveCustomOutput(const FName& EventName)
{
	if (CustomOutputs.Contains(EventName))
	{
		CustomOutputs.Remove(EventName);
	}
}
#endif // WITH_EDITOR

UFlowNode_CustomInput* UFlowAsset::TryFindCustomInputNodeByEventName(const FName& EventName) const
{
	for (UFlowNode_CustomInput* InputNode : CustomInputNodes)
	{
		if (IsValid(InputNode) && InputNode->GetEventName() == EventName)
		{
			return InputNode;
		}
	}

	return nullptr;
}

UFlowNode_CustomOutput* UFlowAsset::TryFindCustomOutputNodeByEventName(const FName& EventName) const
{
	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (UFlowNode_CustomOutput* CustomOutput = Cast<UFlowNode_CustomOutput>(Node.Value))
		{
			if (CustomOutput->GetEventName() == EventName)
			{
				return CustomOutput;
			}
		}
	}

	return nullptr;
}

TArray<FName> UFlowAsset::GatherCustomInputNodeEventNames() const
{
	// Runtime-safe gathering of the CustomInputs (which is editor-only data)
	//  from the actual flow nodes
	TArray<FName> Results;

	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (UFlowNode_CustomInput* CustomInput = Cast<UFlowNode_CustomInput>(Node.Value))
		{
			Results.Add(CustomInput->GetEventName());
		}
	}

	return Results;
}

TArray<FName> UFlowAsset::GatherCustomOutputNodeEventNames() const
{
	// Runtime-safe gathering of the CustomOutputs (which is editor-only data)
	//  from the actual flow nodes
	TArray<FName> Results;

	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (UFlowNode_CustomOutput* CustomOutput = Cast<UFlowNode_CustomOutput>(Node.Value))
		{
			Results.Add(CustomOutput->GetEventName());
		}
	}

	return Results;
}

TArray<UFlowNode*> UFlowAsset::GetNodesInExecutionOrder(UFlowNode* FirstIteratedNode, const TSubclassOf<UFlowNode> FlowNodeClass)
{
	TArray<UFlowNode*> FoundNodes;
	GetNodesInExecutionOrder<UFlowNode>(FirstIteratedNode, FoundNodes);

	// filter out nodes by class
	for (int32 i = FoundNodes.Num() - 1; i >= 0; i--)
	{
		if (!FoundNodes[i]->GetClass()->IsChildOf(FlowNodeClass))
		{
			FoundNodes.RemoveAt(i);
		}
	}
	FoundNodes.Shrink();
	
	return FoundNodes;
}

void UFlowAsset::AddInstance(UFlowAsset* Instance)
{
	ActiveInstances.Add(Instance);
}

int32 UFlowAsset::RemoveInstance(UFlowAsset* Instance)
{
#if WITH_EDITOR
	if (InspectedInstance.IsValid() && InspectedInstance.Get() == Instance)
	{
		SetInspectedInstance(NAME_None);
	}
#endif

	ActiveInstances.Remove(Instance);
	return ActiveInstances.Num();
}

void UFlowAsset::ClearInstances()
{
#if WITH_EDITOR
	if (InspectedInstance.IsValid())
	{
		SetInspectedInstance(NAME_None);
	}
#endif

	for (int32 i = ActiveInstances.Num() - 1; i >= 0; i--)
	{
		if (ActiveInstances.IsValidIndex(i) && ActiveInstances[i])
		{
			ActiveInstances[i]->FinishFlow(EFlowFinishPolicy::Keep);
		}
	}

	ActiveInstances.Empty();
}

#if WITH_EDITOR
void UFlowAsset::GetInstanceDisplayNames(TArray<TSharedPtr<FName>>& OutDisplayNames) const
{
	for (const UFlowAsset* Instance : ActiveInstances)
	{
		OutDisplayNames.Emplace(MakeShareable(new FName(Instance->GetDisplayName())));
	}
}

void UFlowAsset::SetInspectedInstance(const FName& NewInspectedInstanceName)
{
	if (NewInspectedInstanceName.IsNone())
	{
		InspectedInstance = nullptr;
	}
	else
	{
		for (UFlowAsset* ActiveInstance : ActiveInstances)
		{
			if (ActiveInstance && ActiveInstance->GetDisplayName() == NewInspectedInstanceName)
			{
				if (!InspectedInstance.IsValid() || InspectedInstance != ActiveInstance)
				{
					InspectedInstance = ActiveInstance;
				}
				break;
			}
		}
	}

	BroadcastDebuggerRefresh();
}

void UFlowAsset::BroadcastDebuggerRefresh() const
{
	RefreshDebuggerEvent.Broadcast();
}

void UFlowAsset::BroadcastRuntimeMessageAdded(const TSharedRef<FTokenizedMessage>& Message) const
{
	RuntimeMessageEvent.Broadcast(this, Message);
}
#endif // WITH_EDITOR

void UFlowAsset::InitializeInstance(const TWeakObjectPtr<UObject> InOwner, UFlowAsset* InTemplateAsset)
{
	Owner = InOwner;
	TemplateAsset = InTemplateAsset;

	for (TPair<FGuid, TObjectPtr<UFlowNode>>& Node : Nodes)
	{
		UFlowNode* NewNodeInstance = NewObject<UFlowNode>(this, Node.Value->GetClass(), NAME_None, RF_Transient, Node.Value, false, nullptr);
		Node.Value = NewNodeInstance;

		if (UFlowNode_CustomInput* CustomInput = Cast<UFlowNode_CustomInput>(NewNodeInstance))
		{
			if (!CustomInput->EventName.IsNone())
			{
				CustomInputNodes.Emplace(CustomInput);
			}
		}

		NewNodeInstance->InitializeInstance();
	}
}

void UFlowAsset::DeinitializeInstance()
{
	for (const TPair<FGuid, UFlowNode*>& Node : ObjectPtrDecay(Nodes))
	{
		if (IsValid(Node.Value))
		{
			Node.Value->DeinitializeInstance();
		}
	}

	if (TemplateAsset)
	{
		const int32 ActiveInstancesLeft = TemplateAsset->RemoveInstance(this);
		if (ActiveInstancesLeft == 0 && GetFlowSubsystem())
		{
			GetFlowSubsystem()->RemoveInstancedTemplate(TemplateAsset);
		}
	}
}

void UFlowAsset::PreStartFlow()
{
	ResetNodes();

#if WITH_EDITOR
	if (TemplateAsset->ActiveInstances.Num() == 1)
	{
		// this instance is the only active one, set it directly as Inspected Instance
		TemplateAsset->SetInspectedInstance(GetDisplayName());
	}
	else
	{
		// request to refresh list to show newly created instance
		TemplateAsset->BroadcastDebuggerRefresh();
	}
#endif
}

void UFlowAsset::StartFlow(IFlowDataPinValueSupplierInterface* DataPinValueSupplier)
{
	PreStartFlow();

	if (UFlowNode* ConnectedEntryNode = GetDefaultEntryNode())
	{
		RecordedNodes.Add(ConnectedEntryNode);

		if (IFlowNodeWithExternalDataPinSupplierInterface* ExternalPinSuppliedNode = Cast<IFlowNodeWithExternalDataPinSupplierInterface>(ConnectedEntryNode))
		{
			ExternalPinSuppliedNode->SetDataPinValueSupplier(DataPinValueSupplier);
		}

		ConnectedEntryNode->TriggerFirstOutput(true);
	}
}

void UFlowAsset::FinishFlow(const EFlowFinishPolicy InFinishPolicy, const bool bRemoveInstance /*= true*/)
{
	FinishPolicy = InFinishPolicy;

	// end execution of this asset and all of its nodes
	for (UFlowNode* Node : ActiveNodes)
	{
		Node->Deactivate();
	}
	ActiveNodes.Empty();

	// flush preloaded content
	for (UFlowNode* PreloadedNode : PreloadedNodes)
	{
		PreloadedNode->TriggerFlush();
	}
	PreloadedNodes.Empty();

	// provides option to finish game-specific logic prior to removing asset instance 
	if (bRemoveInstance)
	{
		DeinitializeInstance();
	}
}

bool UFlowAsset::HasStartedFlow() const
{
	return RecordedNodes.Num() > 0;
}

AActor* UFlowAsset::TryFindActorOwner() const
{
	const UActorComponent* OwnerAsComponent = Cast<UActorComponent>(GetOwner());
	if (IsValid(OwnerAsComponent))
	{
		return Cast<AActor>(OwnerAsComponent->GetOwner());
	}

	return nullptr;
}

TWeakObjectPtr<UFlowAsset> UFlowAsset::GetFlowInstance(UFlowNode_SubGraph* SubGraphNode) const
{
	return ActiveSubGraphs.FindRef(SubGraphNode);
}

void UFlowAsset::TriggerCustomInput_FromSubGraph(UFlowNode_SubGraph* SubGraphNode, const FName& EventName) const
{
	// NOTE (gtaylor) Custom Input nodes cannot currently add data pins (like Start or DefineProperties nodes can)
	// but we may want to allow them to source parameters, so I am providing the subgraph node as the 
	// IFlowDataPinValueSupplierInterface when triggering the node (even though it's not used at this time).

	const TWeakObjectPtr<UFlowAsset> FlowInstance = ActiveSubGraphs.FindRef(SubGraphNode);
	if (FlowInstance.IsValid())
	{
		FlowInstance->TriggerCustomInput(EventName, SubGraphNode);
	}
}

void UFlowAsset::TriggerCustomInput(const FName& EventName, IFlowDataPinValueSupplierInterface* DataPinValueSupplier)
{
	for (UFlowNode_CustomInput* CustomInputNode : CustomInputNodes)
	{
		if (CustomInputNode->EventName == EventName)
		{
			RecordedNodes.Add(CustomInputNode);

			// NOTE (gtaylor) Custom Input nodes cannot currently add data pins (like Start or DefineProperties nodes can)
			// but we may want to allow them to source parameters, so I am providing the subgraph node as the 
			// IFlowDataPinValueSupplierInterface when triggering the node (even though it's not used at this time).

			if (IFlowNodeWithExternalDataPinSupplierInterface* ExternalPinSuppliedNode = Cast<IFlowNodeWithExternalDataPinSupplierInterface>(CustomInputNode))
			{
				ExternalPinSuppliedNode->SetDataPinValueSupplier(DataPinValueSupplier);
			}

			CustomInputNode->ExecuteInput(EventName);
		}
	}
}

void UFlowAsset::TriggerCustomOutput(const FName& EventName)
{
	if (NodeOwningThisAssetInstance.IsValid())
	{
		// it's a SubGraph
		NodeOwningThisAssetInstance->TriggerOutput(EventName);
	}
	else
	{
		// it's a Root Flow, so the intention here might be to call event on the Flow Component
		if (UFlowComponent* FlowComponent = Cast<UFlowComponent>(GetOwner()))
		{
			FlowComponent->OnTriggerRootFlowOutputEventDispatcher(this, EventName);
		}
	}
}

void UFlowAsset::TriggerInput(const FGuid& NodeGuid, const FName& PinName)
{
	if (UFlowNode* Node = Nodes.FindRef(NodeGuid))
	{
		if (!ActiveNodes.Contains(Node))
		{
			ActiveNodes.Add(Node);
			RecordedNodes.Add(Node);
		}

		Node->TriggerInput(PinName);
	}
}

void UFlowAsset::FinishNode(UFlowNode* Node)
{
	if (ActiveNodes.Contains(Node))
	{
		ActiveNodes.Remove(Node);

		// if graph reached Finish and this asset instance was created by SubGraph node
		if (Node->CanFinishGraph())
		{
			if (NodeOwningThisAssetInstance.IsValid())
			{
				NodeOwningThisAssetInstance.Get()->TriggerFirstOutput(true);

				return;
			}

			// if this instance is a Root Flow, we need to deregister it from the subsystem first
			if (Owner.IsValid())
			{
				const TSet<UFlowAsset*>& RootFlowInstances = GetFlowSubsystem()->GetRootInstancesByOwner(Owner.Get());
				if (RootFlowInstances.Contains(this))
				{
					GetFlowSubsystem()->FinishRootFlow(Owner.Get(), TemplateAsset, EFlowFinishPolicy::Keep);
					return;
				}
			}

			FinishFlow(EFlowFinishPolicy::Keep);
		}
	}
}

void UFlowAsset::ResetNodes()
{
	for (UFlowNode* Node : RecordedNodes)
	{
		Node->ResetRecords();
	}

	RecordedNodes.Empty();
}

UFlowSubsystem* UFlowAsset::GetFlowSubsystem() const
{
	return Cast<UFlowSubsystem>(GetOuter());
}

FName UFlowAsset::GetDisplayName() const
{
	return GetFName();
}

UFlowNode_SubGraph* UFlowAsset::GetNodeOwningThisAssetInstance() const
{
	return NodeOwningThisAssetInstance.Get();
}

UFlowAsset* UFlowAsset::GetParentInstance() const
{
	return NodeOwningThisAssetInstance.IsValid() ? NodeOwningThisAssetInstance.Get()->GetFlowAsset() : nullptr;
}

FFlowAssetSaveData UFlowAsset::SaveInstance(TArray<FFlowAssetSaveData>& SavedFlowInstances)
{
	FFlowAssetSaveData AssetRecord;
	AssetRecord.WorldName = IsBoundToWorld() ? GetWorld()->GetName() : FString();
	AssetRecord.InstanceName = GetName();

	// opportunity to collect data before serializing asset
	OnSave();

	// iterate nodes
	TArray<UFlowNode*> NodesInExecutionOrder;
	GetNodesInExecutionOrder<UFlowNode>(GetDefaultEntryNode(), NodesInExecutionOrder);
	for (UFlowNode* Node : NodesInExecutionOrder)
	{
		if (Node && Node->ActivationState == EFlowNodeState::Active)
		{
			// iterate SubGraphs
			if (UFlowNode_SubGraph* SubGraphNode = Cast<UFlowNode_SubGraph>(Node))
			{
				const TWeakObjectPtr<UFlowAsset> SubFlowInstance = GetFlowInstance(SubGraphNode);
				if (SubFlowInstance.IsValid())
				{
					const FFlowAssetSaveData SubAssetRecord = SubFlowInstance->SaveInstance(SavedFlowInstances);
					SubGraphNode->SavedAssetInstanceName = SubAssetRecord.InstanceName;
				}
			}

			FFlowNodeSaveData NodeRecord;
			Node->SaveInstance(NodeRecord);

			AssetRecord.NodeRecords.Emplace(NodeRecord);
		}
	}

	// serialize asset
	FMemoryWriter MemoryWriter(AssetRecord.AssetData, true);
	FFlowArchive Ar(MemoryWriter);
	Serialize(Ar);

	// write archive to SaveGame
	SavedFlowInstances.Emplace(AssetRecord);

	return AssetRecord;
}

void UFlowAsset::LoadInstance(const FFlowAssetSaveData& AssetRecord)
{
	FMemoryReader MemoryReader(AssetRecord.AssetData, true);
	FFlowArchive Ar(MemoryReader);
	Serialize(Ar);

	PreStartFlow();

	// iterate graph "from the end", backward to execution order
	// prevents issue when the preceding node would instantly fire output to a not-yet-loaded node
	for (int32 i = AssetRecord.NodeRecords.Num() - 1; i >= 0; i--)
	{
		if (UFlowNode* Node = Nodes.FindRef(AssetRecord.NodeRecords[i].NodeGuid))
		{
			Node->LoadInstance(AssetRecord.NodeRecords[i]);
		}
	}

	OnLoad();
}

void UFlowAsset::OnActivationStateLoaded(UFlowNode* Node)
{
	if (Node->ActivationState != EFlowNodeState::NeverActivated)
	{
		RecordedNodes.Emplace(Node);
	}

	if (Node->ActivationState == EFlowNodeState::Active)
	{
		ActiveNodes.Emplace(Node);
	}
}

void UFlowAsset::OnSave_Implementation()
{
}

void UFlowAsset::OnLoad_Implementation()
{
}

bool UFlowAsset::IsBoundToWorld_Implementation()
{
	return bWorldBound;
}

#if WITH_EDITOR
void UFlowAsset::LogError(const FString& MessageToLog, const UFlowNodeBase* Node) const
{
	// this is runtime log which is should be only called on runtime instances of asset
	if (TemplateAsset == nullptr)
	{
		UE_LOG(LogFlow, Log, TEXT("Attempted to use Runtime Log on template asset %s"), *MessageToLog);
	}

	if (RuntimeLog.Get())
	{
		const TSharedRef<FTokenizedMessage> TokenizedMessage = RuntimeLog.Get()->Error(*MessageToLog, Node);
		BroadcastRuntimeMessageAdded(TokenizedMessage);
	}
}

void UFlowAsset::LogWarning(const FString& MessageToLog, const UFlowNodeBase* Node) const
{
	// this is runtime log which is should be only called on runtime instances of asset
	if (TemplateAsset == nullptr)
	{
		UE_LOG(LogFlow, Log, TEXT("Attempted to use Runtime Log on template asset %s"), *MessageToLog);
	}

	if (RuntimeLog.Get())
	{
		const TSharedRef<FTokenizedMessage> TokenizedMessage = RuntimeLog.Get()->Warning(*MessageToLog, Node);
		BroadcastRuntimeMessageAdded(TokenizedMessage);
	}
}

void UFlowAsset::LogNote(const FString& MessageToLog, const UFlowNodeBase* Node) const
{
	// this is runtime log which is should be only called on runtime instances of asset
	if (TemplateAsset == nullptr)
	{
		UE_LOG(LogFlow, Log, TEXT("Attempted to use Runtime Log on template asset %s"), *MessageToLog);
	}

	if (RuntimeLog.Get())
	{
		const TSharedRef<FTokenizedMessage> TokenizedMessage = RuntimeLog.Get()->Note(*MessageToLog, Node);
		BroadcastRuntimeMessageAdded(TokenizedMessage);
	}
}
#endif

#if WITH_EDITOR
bool FFlowHarvestDataPinsWorkingData::DidPinNameToBoundPropertyNameMapChange() const
{
	if (PinNameToBoundPropertyNameMapPrev.Num() != PinNameToBoundPropertyNameMapNext.Num())
	{
		return true;
	}

	for (const auto& KV : PinNameToBoundPropertyNameMapPrev)
	{
		const FName& PinNameFromPrev = KV.Key;
		const FName& PropertyNameFromPrev = KV.Value;

		const FName* FoundPropertyNameInNext = PinNameToBoundPropertyNameMapNext.Find(PinNameFromPrev);
		if (!FoundPropertyNameInNext)
		{
			return true;
		}

		if (*FoundPropertyNameInNext != PropertyNameFromPrev)
		{
			return true;
		}
	}

	return false;
}

bool FFlowHarvestDataPinsWorkingData::DidAutoInputDataPinsChange() const
{
	return !FFlowPin::ArePinArraysMatchingNamesAndTypes(AutoInputDataPinsPrev, AutoInputDataPinsNext);
}

bool FFlowHarvestDataPinsWorkingData::DidAutoOutputDataPinsChange() const
{
	return !FFlowPin::ArePinArraysMatchingNamesAndTypes(AutoOutputDataPinsPrev, AutoOutputDataPinsNext);
}

#endif
