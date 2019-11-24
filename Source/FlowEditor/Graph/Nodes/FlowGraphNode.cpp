#include "FlowGraphNode.h"
#include "../FlowAssetGraph.h"
#include "../FlowGraphActions.h"
#include "../FlowGraphSchema.h"

#include "Flow/Graph/FlowAsset.h"
#include "Flow/Graph/Nodes/FlowNode.h"

#include "Developer/ToolMenus/Public/ToolMenus.h"
#include "EdGraph/EdGraphSchema.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Font.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GraphEditorActions.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FlowGraphNode"

UFlowGraphNode::UFlowGraphNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowGraphNode::SetFlowNode(UFlowNode* InFlowNode)
{
	FlowNode = InFlowNode;
	InFlowNode->SetGraphNode(this);
}

UFlowNode* UFlowGraphNode::GetFlowNode() const
{
	if (FlowNode)
	{
		if (UFlowAsset* RuntimeIntance = FlowNode->GetFlowAsset()->GetInspectedInstance())
		{
			return RuntimeIntance->GetNodeInstance(FlowNode->GetGuid());
		}

		return FlowNode;
	}

	return nullptr;
}

void UFlowGraphNode::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	if (Context->Pin)
	{
		FToolMenuSection& Section = Menu->AddSection("FlowGraphNodeDeletePins");

		// todo: only for nodes with manually added inputs, i.e. AND
		if (Context->Pin->Direction == EGPD_Input && GetInputCount() > 2)
		{
			Section.AddMenuEntry(FFlowGraphCommands::Get().DeleteInput);
		}

		// todo: only for nodes with manually added outputs, i.e. Sequence
		if (Context->Pin->Direction == EGPD_Output && GetOutputCount() > 2)
		{
			Section.AddMenuEntry(FFlowGraphCommands::Get().DeleteOutput);
		}
	}
	else if (Context->Node)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphNodeEdit");
			Section.AddMenuEntry(FGenericCommands::Get().Delete);
			Section.AddMenuEntry(FGenericCommands::Get().Cut);
			Section.AddMenuEntry(FGenericCommands::Get().Copy);
			Section.AddMenuEntry(FGenericCommands::Get().Duplicate);
		}

		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphNodePins");
			if (CanAddInputPin())
			{
				Section.AddMenuEntry(FFlowGraphCommands::Get().AddInput);
			}
			if (CanAddOutputPin())
			{
				Section.AddMenuEntry(FFlowGraphCommands::Get().AddOutput);
			}
		}

		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphNodeDebug");
			if (CanAddInputPin())
			{
				Section.AddMenuEntry(FFlowGraphCommands::Get().FocusViewport);
			}
		}
	}
}

FString UFlowGraphNode::GetDesc()
{
	if (FlowNode)
	{
		return FlowNode->GetDesc();
	}

	return FString();
}

FText UFlowGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (FlowNode)
	{
		return FlowNode->GetTitle();
	}
	else
	{
		return Super::GetNodeTitle(TitleType);
	}
}

FText UFlowGraphNode::GetTooltipText() const
{
	FText Tooltip;
	if (FlowNode)
	{
		Tooltip = FlowNode->GetClass()->GetToolTipText();
	}
	if (Tooltip.IsEmpty())
	{
		Tooltip = GetNodeTitle(ENodeTitleType::ListView);
	}
	return Tooltip;
}

void UFlowGraphNode::CreateInputPins()
{
	if (FlowNode && FlowNode->HasInputPins())
	{
		CreateInputPin();
	}
}

void UFlowGraphNode::CreateInputPin(const FName PinName /*= TEXT("In")*/)
{
	UEdGraphPin* NewPin = CreatePin(EGPD_Input, TEXT("FlowNode"), PinName);
	if (NewPin->PinName.IsNone())
	{
		// Makes sure pin has a name for lookup purposes but user will never see it
		NewPin->PinName = CreateUniquePinName(TEXT("In"));
		NewPin->PinFriendlyName = FText::FromString(TEXT(" "));
	}

	InputPins.Add(NewPin);
}

void UFlowGraphNode::AddInputPin()
{
	const FScopedTransaction Transaction(LOCTEXT("FlowEditorAddInput", "Add Flow Node Input"));
	Modify();
	CreateInputPin();

	UFlowAsset* FlowAsset = CastChecked<UFlowAssetGraph>(GetGraph())->GetFlowAsset();
	FlowAsset->CompileNodeConnections();
	FlowAsset->MarkPackageDirty();

	// Refresh the current graph, so the pins can be updated
	GetGraph()->NotifyGraphChanged();
}

void UFlowGraphNode::RemoveInputPin(UEdGraphPin* InGraphPin)
{
	const FScopedTransaction Transaction(LOCTEXT("FlowEditorDeleteInput", "Delete Flow Node Input"));
	Modify();

	for (auto PinIt = InputPins.CreateIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* Pin = *PinIt;
		if (InGraphPin == Pin)
		{
			InGraphPin->MarkPendingKill();
			Pins.Remove(InGraphPin);
			InputPins.Remove(InGraphPin);
			FlowNode->Modify();
			break;
		}
	}

	UFlowAsset* FlowAsset = CastChecked<UFlowAssetGraph>(GetGraph())->GetFlowAsset();
	FlowAsset->CompileNodeConnections();
	FlowAsset->MarkPackageDirty();

	// Refresh the current graph, so the pins can be updated
	GetGraph()->NotifyGraphChanged();
}

UEdGraphPin* UFlowGraphNode::GetInputPin(const uint8 Index) const
{
	if (InputPins.IsValidIndex(Index))
	{
		return InputPins[Index];
	}

	return nullptr;
}

void UFlowGraphNode::GetInputPins(TArray<UEdGraphPin*>& OutPins) const
{
	OutPins = InputPins;
}

uint8 UFlowGraphNode::GetInputCount() const
{
	return InputPins.Num();
}

uint8 UFlowGraphNode::GetInputPinIndex(const UEdGraphPin* Pin) const
{
	for (uint8 i = 0; i < InputPins.Num(); i++)
	{
		if (Pin == InputPins[i])
		{
			return i;
		}
	}

	return 0;
}

void UFlowGraphNode::CreateOutputPins()
{
	if (FlowNode && FlowNode->HasOutputPins())
	{
		CreateOutputPin();
	}
}

void UFlowGraphNode::CreateOutputPin(const FName PinName /*= TEXT("Out")*/)
{
	UEdGraphPin* NewPin = CreatePin(EGPD_Output, TEXT("FlowNode"), PinName);
	if (NewPin->PinName.IsNone())
	{
		// Makes sure pin has a name for lookup purposes but user will never see it
		NewPin->PinName = CreateUniquePinName(TEXT("Out"));
		NewPin->PinFriendlyName = FText::FromString(TEXT(" "));
	}

	OutputPins.Add(NewPin);
}

void UFlowGraphNode::AddOutputPin()
{
	const FScopedTransaction Transaction(LOCTEXT("FlowEditorAddOutput", "Add Flow Node Input"));
	Modify();
	CreateOutputPin();

	UFlowAsset* FlowAsset = CastChecked<UFlowAssetGraph>(GetGraph())->GetFlowAsset();
	FlowAsset->CompileNodeConnections();
	FlowAsset->MarkPackageDirty();

	// Refresh the current graph, so the pins can be updated
	GetGraph()->NotifyGraphChanged();
}

void UFlowGraphNode::RemoveOutputPin(UEdGraphPin* InGraphPin)
{
	const FScopedTransaction Transaction(LOCTEXT("FlowEditorDeleteOutput", "Delete Flow Node Output"));
	Modify();

	for (auto PinIt = OutputPins.CreateIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* Pin = *PinIt;
		if (InGraphPin == Pin)
		{
			InGraphPin->MarkPendingKill();
			Pins.Remove(InGraphPin);
			OutputPins.Remove(InGraphPin);
			FlowNode->Modify();
			break;
		}
	}

	UFlowAsset* FlowAsset = CastChecked<UFlowAssetGraph>(GetGraph())->GetFlowAsset();
	FlowAsset->CompileNodeConnections();
	FlowAsset->MarkPackageDirty();

	// Refresh the current graph, so the pins can be updated
	GetGraph()->NotifyGraphChanged();
}

UEdGraphPin* UFlowGraphNode::GetOutputPin(const uint8 Index) const
{
	if (OutputPins.IsValidIndex(Index))
	{
		return OutputPins[Index];
	}

	return nullptr;
}

void UFlowGraphNode::GetOutputPins(TArray<UEdGraphPin*>& OutPins) const
{
	OutPins = OutputPins;
}

uint8 UFlowGraphNode::GetOutputCount() const
{
	return OutputPins.Num();
}

bool UFlowGraphNode::CanFocusViewport() const
{
	if (FlowNode)
	{
		return GEditor->bIsSimulatingInEditor && FlowNode->GetAssetToOpen();
	}

	return false;
}

int32 UFlowGraphNode::EstimateNodeWidth() const
{
	const int32 EstimatedCharWidth = 6;
	FString NodeTitle = GetNodeTitle(ENodeTitleType::FullTitle).ToString();
	UFont* Font = GetDefault<UEditorEngine>()->EditorFont;
	int32 Result = NodeTitle.Len()*EstimatedCharWidth;

	if (Font)
	{
		Result = Font->GetStringSize(*NodeTitle);
	}

	return Result;
}

void UFlowGraphNode::InsertNewNode(UEdGraphPin* FromPin, UEdGraphPin* NewLinkPin, TSet<UEdGraphNode*>& OutNodeList)
{
	const UFlowGraphSchema* Schema = CastChecked<UFlowGraphSchema>(GetSchema());

	// The pin we are creating from already has a connection that needs to be broken. We want to "insert" the new node in between, so that the output of the new node is hooked up too
	UEdGraphPin* OldLinkedPin = FromPin->LinkedTo[0];
	check(OldLinkedPin);

	FromPin->BreakAllPinLinks();

	// Hook up the old linked pin to the first valid output pin on the new node
	for (int32 OutpinPinIdx = 0; OutpinPinIdx < Pins.Num(); OutpinPinIdx++)
	{
		UEdGraphPin* OutputExecPin = Pins[OutpinPinIdx];
		check(OutputExecPin);
		if (ECanCreateConnectionResponse::CONNECT_RESPONSE_MAKE == Schema->CanCreateConnection(OldLinkedPin, OutputExecPin).Response)
		{
			if (Schema->TryCreateConnection(OldLinkedPin, OutputExecPin))
			{
				OutNodeList.Add(OldLinkedPin->GetOwningNode());
				OutNodeList.Add(this);
			}
			break;
		}
	}

	if (Schema->TryCreateConnection(FromPin, NewLinkPin))
	{
		OutNodeList.Add(FromPin->GetOwningNode());
		OutNodeList.Add(this);
	}
}

void UFlowGraphNode::AllocateDefaultPins()
{
	check(Pins.Num() == 0);

	CreateInputPins();
	CreateOutputPins();
}

void UFlowGraphNode::ReconstructNode()
{
	// Break any links to 'orphan' pins
	for (UEdGraphPin* Pin : Pins)
	{
		TArray<class UEdGraphPin*>& LinkedToRef = Pin->LinkedTo;
		for (int32 LinkIdx = 0; LinkIdx < LinkedToRef.Num(); LinkIdx++)
		{
			UEdGraphPin* OtherPin = LinkedToRef[LinkIdx];
			// If we are linked to a pin that its owner doesn't know about, break that link
			if (!OtherPin->GetOwningNode()->Pins.Contains(OtherPin))
			{
				Pin->LinkedTo.Remove(OtherPin);
			}
		}
	}

	// cache pins in arrays
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			InputPins.Add(Pin);
		}
		else
		{
			OutputPins.Add(Pin);
		}
	}

	// Store the old Input and Output pins
	TArray<UEdGraphPin*> OldInputPins(InputPins);
	TArray<UEdGraphPin*> OldOutputPins(OutputPins);

	// Move the existing pins to a saved array
	TArray<UEdGraphPin*> OldPins(Pins);
	Pins.Reset();
	InputPins.Reset();
	OutputPins.Reset();

	// Recreate the new pins
	AllocateDefaultPins();

	// Get new Input pins
	for (int32 PinIndex = 0; PinIndex < OldInputPins.Num(); PinIndex++)
	{
		if (PinIndex < InputPins.Num())
		{
			InputPins[PinIndex]->MovePersistentDataFromOldPin(*OldInputPins[PinIndex]);
		}
	}

	// Get new Output pins
	for (int32 PinIndex = 0; PinIndex < OldOutputPins.Num(); PinIndex++)
	{
		if (PinIndex < OutputPins.Num())
		{
			OutputPins[PinIndex]->MovePersistentDataFromOldPin(*OldOutputPins[PinIndex]);
		}
	}

	// Throw away the original pins
	for (UEdGraphPin* OldPin : OldPins)
	{
		OldPin->Modify();
		UEdGraphNode::DestroyPin(OldPin);
	}
}

void UFlowGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (FromPin != nullptr)
	{
		const UFlowGraphSchema* Schema = CastChecked<UFlowGraphSchema>(GetSchema());

		TSet<UEdGraphNode*> NodeList;

		// auto-connect from dragged pin to first compatible pin on the new node
		for (UEdGraphPin* Pin : Pins)
		{
			check(Pin);
			FPinConnectionResponse Response = Schema->CanCreateConnection(FromPin, Pin);
			if (ECanCreateConnectionResponse::CONNECT_RESPONSE_MAKE == Response.Response)
			{
				if (Schema->TryCreateConnection(FromPin, Pin))
				{
					NodeList.Add(FromPin->GetOwningNode());
					NodeList.Add(this);
				}
				break;
			}
			else if (ECanCreateConnectionResponse::CONNECT_RESPONSE_BREAK_OTHERS_A == Response.Response)
			{
				InsertNewNode(FromPin, Pin, NodeList);
				break;
			}
		}

		// Send all nodes that received a new pin connection a notification
		for (auto It = NodeList.CreateConstIterator(); It; ++It)
		{
			UEdGraphNode* Node = (*It);
			Node->NodeConnectionListChanged();
		}
	}
}

bool UFlowGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->IsA(UFlowGraphSchema::StaticClass());
}

UObject* UFlowGraphNode::GetAssetToOpen() const
{
	if (FlowNode)
	{
		return FlowNode->GetAssetToOpen();
	}

	return nullptr;
}

void UFlowGraphNode::PostLoad()
{
	Super::PostLoad();

	// Fixup any FlowNode back pointers that may be out of date
	if (FlowNode)
	{
		FlowNode->SetGraphNode(this);
	}

	for (UEdGraphPin* Pin : Pins)
	{
		// cache pins in arrays
		if (Pin->Direction == EGPD_Input)
		{
			InputPins.Add(Pin);
		}
		else
		{
			OutputPins.Add(Pin);
		}

		if (Pin->PinName.IsNone())
		{
			// Makes sure pin has a name for lookup purposes but user will never see it
			if (Pin->Direction == EGPD_Input)
			{
				Pin->PinName = CreateUniquePinName(TEXT("Input"));
			}
			else
			{
				Pin->PinName = CreateUniquePinName(TEXT("Output"));
			}
			Pin->PinFriendlyName = FText::FromString(TEXT(" "));
		}
	}
}

void UFlowGraphNode::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		CreateNewGuid();
	}
}

void UFlowGraphNode::PostEditImport()
{
	// Make sure this FlowNode is owned by the FlowAsset it's being pasted into.
	ResetFlowNodeOwner();
}

void UFlowGraphNode::PrepareForCopying()
{
	if (FlowNode)
	{
		// Temporarily take ownership of the FlowNode, so that it is not deleted when cutting
		FlowNode->Rename(nullptr, this, REN_DontCreateRedirectors);
	}
}

void UFlowGraphNode::PostCopyNode()
{
	// Make sure the FlowNode goes back to being owned by the FlowAsset after copying.
	ResetFlowNodeOwner();
}

void UFlowGraphNode::ResetFlowNodeOwner()
{
	if (FlowNode)
	{
		UFlowAsset* FlowAsset = CastChecked<UFlowAssetGraph>(GetGraph())->GetFlowAsset();

		if (FlowNode->GetOuter() != FlowAsset)
		{
			// Ensures FlowNode is owned by the FlowAsset
			FlowNode->Rename(nullptr, FlowAsset, REN_DontCreateRedirectors);
		}

		// Set up the back pointer for newly created flow nodes
		FlowNode->SetGraphNode(this);
	}
}

#undef LOCTEXT_NAMESPACE
