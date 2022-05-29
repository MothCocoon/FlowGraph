// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/FlowNode.h"

#include "FlowAsset.h"
#include "FlowModule.h"
#include "FlowSubsystem.h"
#include "FlowTypes.h"

#include "Engine/Engine.h"
#include "Engine/ViewportStatsSubsystem.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
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
#if WITH_EDITOR
	, GraphNode(nullptr)
	, bCanDelete(true)
	, bCanDuplicate(true)
	, bNodeDeprecated(false)
#endif
	, bPreloaded(false)
	, ActivationState(EFlowNodeState::NeverActivated)
{
#if WITH_EDITOR
	Category = TEXT("Uncategorized");
	NodeStyle = EFlowNodeStyle::Default;
#endif

	InputPins = {DefaultInputPin};
	OutputPins = {DefaultOutputPin};
}

#if WITH_EDITOR
void UFlowNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property
		&& (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowNode, InputPins) || PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFlowNode, OutputPins)))
	{
		OnReconstructionRequested.ExecuteIfBound();
	}
}

void UFlowNode::PostLoad()
{
	Super::PostLoad();

	// fix Class Default Object
	FixNode(nullptr);
}

void UFlowNode::FixNode(UEdGraphNode* NewGraph)
{
	// Fix any node pointers that may be out of date
	if (NewGraph)
	{
		GraphNode = NewGraph;
	}

	// v1.1 upgraded pins to be defined as structs
	if (InputNames_DEPRECATED.Num() > InputPins.Num())
	{
		for (int32 i = InputPins.Num(); i < InputNames_DEPRECATED.Num(); i++)
		{
			InputPins.Emplace(InputNames_DEPRECATED[i]);
		}
	}
	if (OutputNames_DEPRECATED.Num() > OutputPins.Num())
	{
		for (int32 i = OutputPins.Num(); i < OutputNames_DEPRECATED.Num(); i++)
		{
			OutputPins.Emplace(OutputNames_DEPRECATED[i]);
		}
	}
}

void UFlowNode::SetGraphNode(UEdGraphNode* NewGraph)
{
	GraphNode = NewGraph;
}

FString UFlowNode::GetNodeCategory() const
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

FText UFlowNode::GetNodeTitle() const
{
	if (GetClass()->ClassGeneratedBy)
	{
		const FString& BlueprintTitle = Cast<UBlueprint>(GetClass()->ClassGeneratedBy)->BlueprintDisplayName;
		if (!BlueprintTitle.IsEmpty())
		{
			return FText::FromString(BlueprintTitle);
		}
	}

	return GetClass()->GetDisplayNameText();
}

FText UFlowNode::GetNodeToolTip() const
{
	if (GetClass()->ClassGeneratedBy)
	{
		const FString& BlueprintToolTip = Cast<UBlueprint>(GetClass()->ClassGeneratedBy)->BlueprintDescription;
		if (!BlueprintToolTip.IsEmpty())
		{
			return FText::FromString(BlueprintToolTip);
		}
	}

	return GetClass()->GetToolTipText();
}

FString UFlowNode::GetNodeDescription() const
{
	return K2_GetNodeDescription();
}
#endif

UFlowAsset* UFlowNode::GetFlowAsset() const
{
	return GetOuter() ? Cast<UFlowAsset>(GetOuter()) : nullptr;
}

void UFlowNode::AddInputPins(TArray<FName> PinNames)
{
	for (const FName& PinName : PinNames)
	{
		InputPins.Emplace(PinName);
	}
}

void UFlowNode::AddOutputPins(TArray<FName> PinNames)
{
	for (const FName& PinName : PinNames)
	{
		OutputPins.Emplace(PinName);
	}
}

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
bool UFlowNode::CanUserAddInput() const
{
	return K2_CanUserAddInput();
}

bool UFlowNode::CanUserAddOutput() const
{
	return K2_CanUserAddOutput();
}

void UFlowNode::RemoveUserInput()
{
	Modify();
	InputPins.RemoveAt(InputPins.Num() - 1);
}

void UFlowNode::RemoveUserOutput()
{
	Modify();
	OutputPins.RemoveAt(OutputPins.Num() - 1);
}
#endif

TSet<UFlowNode*> UFlowNode::GetConnectedNodes() const
{
	TSet<UFlowNode*> Result;
	for (const TPair<FName, FConnectedPin>& Connection : Connections)
	{
		Result.Emplace(GetFlowAsset()->GetNode(Connection.Value.NodeGuid));
	}
	return Result;
}

bool UFlowNode::IsInputConnected(const FName& PinName) const
{
	if (GetFlowAsset())
	{
		for (const TPair<FGuid, UFlowNode*>& Pair : GetFlowAsset()->Nodes)
		{
			if (Pair.Value)
			{
				for (const TPair<FName, FConnectedPin>& Connection : Pair.Value->Connections)
				{
					if (Connection.Value.NodeGuid == NodeGuid && Connection.Value.PinName == PinName)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool UFlowNode::IsOutputConnected(const FName& PinName) const
{
	return OutputPins.Contains(PinName) && Connections.Contains(PinName);
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
		for (UFlowNode* ConnectedNode : Node->GetConnectedNodes())
		{
			RecursiveFindNodesByClass(ConnectedNode, Class, Depth, OutNodes);
		}
	}
}

UFlowSubsystem* UFlowNode::GetFlowSubsystem() const
{
	return GetFlowAsset() ? GetFlowAsset()->GetFlowSubsystem() : nullptr;
}

UWorld* UFlowNode::GetWorld() const
{
	if (GetFlowAsset() && GetFlowAsset()->GetFlowSubsystem())
	{
		return GetFlowAsset()->GetFlowSubsystem()->GetWorld();
	}

	return nullptr;
}

void UFlowNode::InitializeInstance()
{
	K2_InitializeInstance();
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

void UFlowNode::PreloadContent()
{
	K2_PreloadContent();
}

void UFlowNode::FlushContent()
{
	K2_FlushContent();
}

void UFlowNode::TriggerInput(const FName& PinName, const bool bForcedActivation /*= false*/)
{
	if (InputPins.Contains(PinName))
	{
		ActivationState = EFlowNodeState::Active;

#if !UE_BUILD_SHIPPING
		// record for debugging
		TArray<FPinRecord>& Records = InputRecords.FindOrAdd(PinName);
		Records.Add(FPinRecord(FApp::GetCurrentTime(), bForcedActivation));
#endif // UE_BUILD_SHIPPING

#if WITH_EDITOR
		if (GetWorld()->WorldType == EWorldType::PIE && UFlowAsset::GetFlowGraphInterface().IsValid())
		{
			UFlowAsset::GetFlowGraphInterface()->OnInputTriggered(GraphNode, InputPins.IndexOfByKey(PinName));
		}
#endif // WITH_EDITOR
	}
	else
	{
#if !UE_BUILD_SHIPPING
		LogError(FString::Printf(TEXT("Input Pin name %s invalid"), *PinName.ToString()));
#endif // UE_BUILD_SHIPPING
		return;
	}

	ExecuteInput(PinName);
}

void UFlowNode::ExecuteInput(const FName& PinName)
{
	K2_ExecuteInput(PinName);
}

void UFlowNode::TriggerFirstOutput(const bool bFinish)
{
	if (OutputPins.Num() > 0)
	{
		TriggerOutput(OutputPins[0].PinName, bFinish);
	}
}

void UFlowNode::TriggerOutput(const FName& PinName, const bool bFinish /*= false*/, const bool bForcedActivation /*= false*/)
{
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
		Records.Add(FPinRecord(FApp::GetCurrentTime(), bForcedActivation));

#if WITH_EDITOR
		if (GetWorld()->WorldType == EWorldType::PIE && UFlowAsset::GetFlowGraphInterface().IsValid())
		{
			UFlowAsset::GetFlowGraphInterface()->OnOutputTriggered(GraphNode, OutputPins.IndexOfByKey(PinName));
		}
#endif // WITH_EDITOR
	}
	else
	{
		LogError(FString::Printf(TEXT("Output Pin name %s invalid"), *PinName.ToString()));
	}
#endif // UE_BUILD_SHIPPING

	// call the next node
	if (OutputPins.Contains(PinName) && Connections.Contains(PinName))
	{
		const FConnectedPin FlowPin = GetConnection(PinName);
		GetFlowAsset()->TriggerInput(FlowPin.NodeGuid, FlowPin.PinName);
	}
}

void UFlowNode::TriggerOutput(const FString& PinName, const bool bFinish)
{
	TriggerOutput(*PinName, bFinish);
}

void UFlowNode::TriggerOutput(const FText& PinName, const bool bFinish)
{
	TriggerOutput(*PinName.ToString(), bFinish);
}

void UFlowNode::TriggerOutput(const TCHAR* PinName, const bool bFinish)
{
	TriggerOutput(FName(PinName), bFinish);
}

void UFlowNode::Finish()
{
	Deactivate();
	GetFlowAsset()->FinishNode(this);
}

void UFlowNode::Deactivate()
{
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

void UFlowNode::Cleanup()
{
	K2_Cleanup();
}

void UFlowNode::ForceFinishNode()
{
	K2_ForceFinishNode();
}

void UFlowNode::ResetRecords()
{
	ActivationState = EFlowNodeState::NeverActivated;

#if !UE_BUILD_SHIPPING
	InputRecords.Empty();
	OutputRecords.Empty();
#endif
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

FString UFlowNode::GetStatusString() const
{
	return K2_GetStatusString();
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

FString UFlowNode::GetProgressAsString(float Value)
{
	// Avoids negative zero
	if (Value == 0)
	{
		Value = 0;
	}

	// First create the string
	FString TempString = FString::Printf(TEXT("%f"), Value);
	if (!TempString.IsNumeric())
	{
		// String did not format as a valid decimal number so avoid messing with it
		return TempString;
	}

	// Get position of the first digit after decimal separator
	int32 TrimIndex = INDEX_NONE;
	for (int32 CharIndex = 0; CharIndex < TempString.Len(); CharIndex++)
	{
		const TCHAR Char = TempString[CharIndex];
		if (Char == TEXT('.'))
		{
			TrimIndex = CharIndex + 2;
			break;
		}
		if (TrimIndex == INDEX_NONE && Char != TEXT('0'))
		{
			TrimIndex = CharIndex + 1;
		}
	}

	TempString.RemoveAt(TrimIndex, TempString.Len() - TrimIndex, /*bAllowShrinking*/false);
	return TempString;
}

void UFlowNode::LogError(FString Message, const EFlowOnScreenMessageType OnScreenMessageType) const
{
	const FString TemplatePath = GetFlowAsset()->TemplateAsset->GetPathName();
	Message += TEXT(" --- node ") + GetName() + TEXT(", asset ") + FPaths::GetPath(TemplatePath) / FPaths::GetBaseFilename(TemplatePath);

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
					return IsValid(this) && ActivationState != EFlowNodeState::NeverActivated;
				});
			}
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, Message);
	}

	UE_LOG(LogFlow, Error, TEXT("%s"), *Message);
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

	OnLoad();
}

void UFlowNode::OnSave_Implementation()
{
}

void UFlowNode::OnLoad_Implementation()
{
}
