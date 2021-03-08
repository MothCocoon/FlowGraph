#include "Nodes/FlowNode.h"

#include "FlowAsset.h"
#include "FlowModule.h"
#include "FlowSubsystem.h"
#include "FlowTypes.h"

#include "Engine/World.h"
#include "Misc/App.h"

FName UFlowNode::DefaultInputName(TEXT("In"));
FName UFlowNode::DefaultOutputName(TEXT("Out"));

FString UFlowNode::MissingIdentityTag = TEXT("Missing Identity Tag!");
FString UFlowNode::NoActorsFound = TEXT("No actors found!");

#if !UE_BUILD_SHIPPING
FPinRecord::FPinRecord()
{
	Time = 0.0f;
	HumanReadableTime = FString();
}

FPinRecord::FPinRecord(const double InTime)
{
	Time = InTime;

	const FDateTime SystemTime(FDateTime::Now());
	HumanReadableTime = DoubleDigit(SystemTime.GetHour()) + TEXT(".")
		+ DoubleDigit(SystemTime.GetMinute()) + TEXT(".")
		+ DoubleDigit(SystemTime.GetSecond()) + TEXT(":")
		+ DoubleDigit(SystemTime.GetMillisecond()).Left(3);
}

FORCEINLINE FString FPinRecord::DoubleDigit(const int32 Number)
{
	return Number > 9 ? FString::FromInt(Number) : TEXT("0") + FString::FromInt(Number);
}
#endif

UFlowNode::UFlowNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
#if WITH_EDITOR
	, GraphNode(nullptr)
#endif
	, bPreloaded(false)
#if !UE_BUILD_SHIPPING
	, ActivationState(EFlowActivationState::NeverActivated)
#endif
{
#if WITH_EDITOR
	Category = TEXT("Uncategorized");
	NodeStyle = EFlowNodeStyle::Default;
#endif

	InputNames = {DefaultInputName};
	OutputNames = {DefaultOutputName};
}

#if WITH_EDITOR
void UFlowNode::SetGraphNode(UEdGraphNode* NewGraph)
{
	GraphNode = NewGraph;
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

#if WITH_EDITOR
void UFlowNode::RemoveUserInput()
{
	Modify();
	InputNames.RemoveAt(InputNames.Num() - 1);
}

void UFlowNode::RemoveUserOutput()
{
	Modify();
	OutputNames.RemoveAt(OutputNames.Num() - 1);
}
#endif

void UFlowNode::SetNumericalInputs(const uint8 FirstNumber, const uint8 LastNumber)
{
	InputNames.Empty();

	for (uint8 i = FirstNumber; i <= LastNumber; i++)
	{
		InputNames.Emplace(FName(*FString::FromInt(i)));
	}
}

void UFlowNode::SetNumericalOutputs(const uint8 FirstNumber /*= 0*/, const uint8 LastNumber /*= 1*/)
{
	OutputNames.Empty();

	for (uint8 i = FirstNumber; i <= LastNumber; i++)
	{
		OutputNames.Emplace(FName(*FString::FromInt(i)));
	}
}

TSet<UFlowNode*> UFlowNode::GetConnectedNodes() const
{
	TSet<UFlowNode*> Result;
	for (const TPair<FName, FConnectedPin>& Connection : Connections)
	{
		Result.Emplace(GetFlowAsset()->GetNode(Connection.Value.NodeGuid));
	}
	return Result;
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

void UFlowNode::TriggerInput(const FName& PinName)
{
	ensureAlways(InputNames.Num() > 0);

#if !UE_BUILD_SHIPPING
	if (InputNames.Contains(PinName))
	{
		// record for debugging
		TArray<FPinRecord>& Records = InputRecords.FindOrAdd(PinName);
		Records.Add(FPinRecord(FApp::GetCurrentTime()));

		ActivationState = EFlowActivationState::Active;

#if WITH_EDITOR
		if (GetWorld()->WorldType == EWorldType::PIE && UFlowAsset::GetFlowGraphInterface().IsValid())
		{
			UFlowAsset::GetFlowGraphInterface()->OnInputTriggered(GraphNode, InputNames.IndexOfByKey(PinName));
		}
#endif // WITH_EDITOR
	}
	else
	{
		LogError(FString::Printf(TEXT("Input Pin name %s invalid"), *PinName.ToString()));
	}
#endif // UE_BUILD_SHIPPING

	ExecuteInput(PinName);
}

void UFlowNode::ExecuteInput(const FName& PinName)
{
	K2_ExecuteInput(PinName);
}

void UFlowNode::TriggerFirstOutput(const bool bFinish)
{
	if (OutputNames.Num() > 0)
	{
		TriggerOutput(OutputNames[0], bFinish);
	}
}

void UFlowNode::TriggerOutput(const FName& PinName, const bool bFinish /*= false*/)
{
	ensureAlways(OutputNames.Num() > 0);

#if !UE_BUILD_SHIPPING
	if (OutputNames.Contains(PinName))
	{
		// record for debugging, even if nothing is connected to this pin
		TArray<FPinRecord>& Records = OutputRecords.FindOrAdd(PinName);
		Records.Add(FPinRecord(FApp::GetCurrentTime()));

#if WITH_EDITOR
		if (GetWorld()->WorldType == EWorldType::PIE && UFlowAsset::GetFlowGraphInterface().IsValid())
		{
			UFlowAsset::GetFlowGraphInterface()->OnOutputTriggered(GraphNode, OutputNames.IndexOfByKey(PinName));
		}
#endif // WITH_EDITOR
	}
	else
	{
		LogError(FString::Printf(TEXT("Output Pin name %s invalid"), *PinName.ToString()));
	}
#endif // UE_BUILD_SHIPPING

	// clean up node, if needed
	if (bFinish)
	{
		Finish();
	}

	// call the next node
	if (OutputNames.Contains(PinName) && Connections.Contains(PinName))
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
#if !UE_BUILD_SHIPPING
	ActivationState = EFlowActivationState::WasActive;
#endif

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

#if !UE_BUILD_SHIPPING
void UFlowNode::ResetRecords()
{
	InputRecords.Empty();
	OutputRecords.Empty();
	ActivationState = EFlowActivationState::NeverActivated;
}
#endif

#if WITH_EDITOR
UFlowNode* UFlowNode::GetInspectedInstance() const
{
	if (const UFlowAsset* FlowInstance = GetFlowAsset()->GetInspectedInstance())
	{
		return FlowInstance->GetNodeInstance(GetGuid());
	}

	return nullptr;
}

TMap<uint8, FPinRecord> UFlowNode::GetWireRecords() const
{
	TMap<uint8, FPinRecord> Result;
	for (const TPair<FName, TArray<FPinRecord>>& Record : OutputRecords)
	{
		Result.Emplace(OutputNames.IndexOfByKey(Record.Key), Record.Value.Last());
	}
	return Result;
}

TArray<FPinRecord> UFlowNode::GetInputRecords(const FName& PinName) const
{
	return InputRecords.FindRef(PinName);
}

TArray<FPinRecord> UFlowNode::GetOutputRecords(const FName& PinName) const
{
	return OutputRecords.FindRef(PinName);
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

void UFlowNode::LogError(FString Message)
{
	const FString TemplatePath = GetFlowAsset()->TemplateAsset->GetPathName();
	Message += TEXT(" in node ") + GetName() + TEXT(", asset ") + FPaths::GetPath(TemplatePath) + TEXT("/") + FPaths::GetBaseFilename(TemplatePath);

	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, Message);
	UE_LOG(LogFlow, Error, TEXT("%s"), *Message);
}
