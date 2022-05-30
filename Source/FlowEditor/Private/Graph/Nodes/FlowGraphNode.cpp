// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode.h"

#include "Asset/FlowDebugger.h"
#include "FlowEditorCommands.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSettings.h"
#include "Graph/Widgets/SFlowGraphNode.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

#include "Developer/ToolMenus/Public/ToolMenus.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ScopedTransaction.h"
#include "SourceCodeNavigation.h"
#include "Textures/SlateIcon.h"
#include "ToolMenuSection.h"
#include "UnrealEd.h"

#define LOCTEXT_NAMESPACE "FlowGraphNode"

//////////////////////////////////////////////////////////////////////////
// Flow Breakpoint

void FFlowBreakpoint::AddBreakpoint()
{
	if (!bHasBreakpoint)
	{
		bHasBreakpoint = true;
		bBreakpointEnabled = true;
	}
}

void FFlowBreakpoint::RemoveBreakpoint()
{
	if (bHasBreakpoint)
	{
		bHasBreakpoint = false;
		bBreakpointEnabled = false;
	}
}

bool FFlowBreakpoint::HasBreakpoint() const
{
	return bHasBreakpoint;
}

void FFlowBreakpoint::EnableBreakpoint()
{
	if (bHasBreakpoint && !bBreakpointEnabled)
	{
		bBreakpointEnabled = true;
	}
}

bool FFlowBreakpoint::CanEnableBreakpoint() const
{
	return bHasBreakpoint && !bBreakpointEnabled;
}

void FFlowBreakpoint::DisableBreakpoint()
{
	if (bHasBreakpoint && bBreakpointEnabled)
	{
		bBreakpointEnabled = false;
	}
}

bool FFlowBreakpoint::IsBreakpointEnabled() const
{
	return bHasBreakpoint && bBreakpointEnabled;
}

void FFlowBreakpoint::ToggleBreakpoint()
{
	if (bHasBreakpoint)
	{
		bHasBreakpoint = false;
		bBreakpointEnabled = false;
	}
	else
	{
		bHasBreakpoint = true;
		bBreakpointEnabled = true;
	}
}

//////////////////////////////////////////////////////////////////////////
// Flow Graph Node

UFlowGraphNode::UFlowGraphNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FlowNode(nullptr)
	, bBlueprintCompilationPending(false)
	, bNeedsFullReconstruction(false)
{
	OrphanedPinSaveMode = ESaveOrphanPinMode::SaveAll;
}

void UFlowGraphNode::SetFlowNode(UFlowNode* InFlowNode)
{
	FlowNode = InFlowNode;
}

UFlowNode* UFlowGraphNode::GetFlowNode() const
{
	if (FlowNode)
	{
		if (const UFlowAsset* InspectedInstance = FlowNode->GetFlowAsset()->GetInspectedInstance())
		{
			return InspectedInstance->GetNode(FlowNode->GetGuid());
		}

		return FlowNode;
	}

	return nullptr;
}

void UFlowGraphNode::PostLoad()
{
	Super::PostLoad();

	if (FlowNode)
	{
		FlowNode->FixNode(this); // fix already created nodes
		SubscribeToExternalChanges();
	}

	ReconstructNode();
}

void UFlowGraphNode::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		CreateNewGuid();

		if (FlowNode && FlowNode->GetFlowAsset())
		{
			FlowNode->GetFlowAsset()->RegisterNode(NodeGuid, FlowNode);
		}
	}
}

void UFlowGraphNode::PostEditImport()
{
	Super::PostEditImport();

	PostCopyNode();
	SubscribeToExternalChanges();
}

void UFlowGraphNode::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	SubscribeToExternalChanges();
}

void UFlowGraphNode::PrepareForCopying()
{
	Super::PrepareForCopying();

	if (FlowNode)
	{
		// Temporarily take ownership of the FlowNode, so that it is not deleted when cutting
		FlowNode->Rename(nullptr, this, REN_DontCreateRedirectors);
	}
}

void UFlowGraphNode::PostCopyNode()
{
	// Make sure this FlowNode is owned by the FlowAsset it's being pasted into
	if (FlowNode)
	{
		UFlowAsset* FlowAsset = CastChecked<UFlowGraph>(GetGraph())->GetFlowAsset();

		if (FlowNode->GetOuter() != FlowAsset)
		{
			// Ensures FlowNode is owned by the FlowAsset
			FlowNode->Rename(nullptr, FlowAsset, REN_DontCreateRedirectors);
		}

		FlowNode->SetGraphNode(this);
	}
}

void UFlowGraphNode::SubscribeToExternalChanges()
{
	if (FlowNode)
	{
		FlowNode->OnReconstructionRequested.BindUObject(this, &UFlowGraphNode::OnExternalChange);

		// blueprint nodes
		if (FlowNode->GetClass()->ClassGeneratedBy && GEditor)
		{
			GEditor->OnBlueprintPreCompile().AddUObject(this, &UFlowGraphNode::OnBlueprintPreCompile);
			GEditor->OnBlueprintCompiled().AddUObject(this, &UFlowGraphNode::OnBlueprintCompiled);
		}
	}
}

void UFlowGraphNode::OnBlueprintPreCompile(UBlueprint* Blueprint)
{
	if (Blueprint && Blueprint == FlowNode->GetClass()->ClassGeneratedBy)
	{
		bBlueprintCompilationPending = true;
	}
}

void UFlowGraphNode::OnBlueprintCompiled()
{
	if (bBlueprintCompilationPending)
	{
		OnExternalChange();
	}

	bBlueprintCompilationPending = false;
}

void UFlowGraphNode::OnExternalChange()
{
	bNeedsFullReconstruction = true;

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
}

bool UFlowGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->IsA(UFlowGraphSchema::StaticClass());
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
			if (CONNECT_RESPONSE_MAKE == Response.Response)
			{
				if (Schema->TryCreateConnection(FromPin, Pin))
				{
					NodeList.Add(FromPin->GetOwningNode());
					NodeList.Add(this);
				}
				break;
			}
			else if (CONNECT_RESPONSE_BREAK_OTHERS_A == Response.Response)
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

void UFlowGraphNode::InsertNewNode(UEdGraphPin* FromPin, UEdGraphPin* NewLinkPin, TSet<UEdGraphNode*>& OutNodeList)
{
	const UFlowGraphSchema* Schema = CastChecked<UFlowGraphSchema>(GetSchema());

	// The pin we are creating from already has a connection that needs to be broken. We want to "insert" the new node in between, so that the output of the new node is hooked up too
	UEdGraphPin* OldLinkedPin = FromPin->LinkedTo[0];
	check(OldLinkedPin);

	FromPin->BreakAllPinLinks();

	// Hook up the old linked pin to the first valid output pin on the new node
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		UEdGraphPin* OutputExecPin = Pins[PinIndex];
		check(OutputExecPin);
		if (CONNECT_RESPONSE_MAKE == Schema->CanCreateConnection(OldLinkedPin, OutputExecPin).Response)
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

void UFlowGraphNode::ReconstructNode()
{
	// Store old pins
	TArray<UEdGraphPin*> OldPins(Pins);

	// Reset pin arrays
	Pins.Reset();
	InputPins.Reset();
	OutputPins.Reset();

	// Recreate pins
	if (SupportsContextPins() && (FlowNode->CanRefreshContextPinsOnLoad() || bNeedsFullReconstruction))
	{
		RefreshContextPins(false);
	}
	AllocateDefaultPins();
	RewireOldPinsToNewPins(OldPins);

	// Destroy old pins
	for (UEdGraphPin* OldPin : OldPins)
	{
		OldPin->Modify();
		OldPin->BreakAllPinLinks();
		DestroyPin(OldPin);
	}

	bNeedsFullReconstruction = false;
}

void UFlowGraphNode::AllocateDefaultPins()
{
	check(Pins.Num() == 0);

	if (FlowNode)
	{
		for (const FFlowPin& InputPin : FlowNode->InputPins)
		{
			CreateInputPin(InputPin);
		}

		for (const FFlowPin& OutputPin : FlowNode->OutputPins)
		{
			CreateOutputPin(OutputPin);
		}
	}
}

void UFlowGraphNode::RewireOldPinsToNewPins(TArray<UEdGraphPin*>& InOldPins)
{
	TArray<UEdGraphPin*> OrphanedOldPins;
	TArray<bool> NewPinMatched; // Tracks whether a NewPin has already been matched to an OldPin
	TMap<UEdGraphPin*, UEdGraphPin*> MatchedPins; // Old to New

	const int32 NumNewPins = Pins.Num();
	NewPinMatched.AddDefaulted(NumNewPins);

	// Rewire any connection to pins that are matched by name (O(N^2) right now)
	// NOTE: we iterate backwards through the list because ReconstructSinglePin()
	//       destroys pins as we go along (clearing out parent pointers, etc.); 
	//       we need the parent pin chain intact for DoPinsMatchForReconstruction();              
	//       we want to destroy old pins from the split children (leaves) up, so 
	//       we do this since split child pins are ordered later in the list 
	//       (after their parents) 
	for (int32 OldPinIndex = InOldPins.Num() - 1; OldPinIndex >= 0; --OldPinIndex)
	{
		UEdGraphPin* OldPin = InOldPins[OldPinIndex];

		// common case is for InOldPins and Pins to match, so we start searching from the current index:
		bool bMatched = false;
		int32 NewPinIndex = (NumNewPins ? OldPinIndex % NumNewPins : 0);
		for (int32 NewPinCount = NumNewPins - 1; NewPinCount >= 0; --NewPinCount)
		{
			// if Pins grows then we may skip entries and fail to find a match or NewPinMatched will not be accurate
			check(NumNewPins == Pins.Num());
			if (!NewPinMatched[NewPinIndex])
			{
				UEdGraphPin* NewPin = Pins[NewPinIndex];

				if (NewPin->PinName == OldPin->PinName)
				{
					ReconstructSinglePin(NewPin, OldPin);

					MatchedPins.Add(OldPin, NewPin);
					bMatched = true;
					NewPinMatched[NewPinIndex] = true;
					break;
				}
			}
			NewPinIndex = (NewPinIndex + 1) % Pins.Num();
		}

		// Orphaned pins are those that existed in the OldPins array but do not in the NewPins.
		// We will save these pins and add them to the NewPins array if they are linked to other pins or have non-default value unless:
		// * The node has been flagged to not save orphaned pins
		// * The pin has been flagged not be saved if orphaned
		// * The pin is hidden
		if (UEdGraphPin::AreOrphanPinsEnabled() && !bDisableOrphanPinSaving && OrphanedPinSaveMode == ESaveOrphanPinMode::SaveAll
			&& !bMatched && !OldPin->bHidden && OldPin->ShouldSavePinIfOrphaned() && OldPin->LinkedTo.Num() > 0)
		{
			OldPin->bOrphanedPin = true;
			OldPin->bNotConnectable = true;
			OrphanedOldPins.Add(OldPin);
			InOldPins.RemoveAt(OldPinIndex, 1, false);
		}
	}

	// The orphaned pins get placed after the rest of the new pins
	for (int32 OrphanedIndex = OrphanedOldPins.Num() - 1; OrphanedIndex >= 0; --OrphanedIndex)
	{
		UEdGraphPin* OrphanedPin = OrphanedOldPins[OrphanedIndex];
		if (OrphanedPin->ParentPin == nullptr)
		{
			Pins.Add(OrphanedPin);
		}
	}
}

void UFlowGraphNode::ReconstructSinglePin(UEdGraphPin* NewPin, UEdGraphPin* OldPin)
{
	check(NewPin && OldPin);

	// Copy over modified persistent data
	NewPin->MovePersistentDataFromOldPin(*OldPin);

	// Update the in breakpoints as the old pin will be going the way of the dodo
	for (TPair<FEdGraphPinReference, FFlowBreakpoint>& PinBreakpoint : PinBreakpoints)
	{
		if (PinBreakpoint.Key.Get() == OldPin)
		{
			PinBreakpoint.Key = NewPin;
			break;
		}
	}
}

void UFlowGraphNode::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	const FGraphEditorCommandsImpl& GraphCommands = FGraphEditorCommands::Get();
	const FFlowGraphCommands& FlowGraphCommands = FFlowGraphCommands::Get();

	if (Context->Pin)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphPinActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
			if (Context->Pin->LinkedTo.Num() > 0)
			{
				Section.AddMenuEntry(GraphCommands.BreakPinLinks);
			}

			if (Context->Pin->Direction == EGPD_Input && CanUserRemoveInput(Context->Pin))
			{
				Section.AddMenuEntry(FlowGraphCommands.RemovePin);
			}
			else if (Context->Pin->Direction == EGPD_Output && CanUserRemoveOutput(Context->Pin))
			{
				Section.AddMenuEntry(FlowGraphCommands.RemovePin);
			}
		}

		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphPinBreakpoints", LOCTEXT("PinBreakpointsMenuHeader", "Pin Breakpoints"));
			Section.AddMenuEntry(FlowGraphCommands.AddPinBreakpoint);
			Section.AddMenuEntry(FlowGraphCommands.RemovePinBreakpoint);
			Section.AddMenuEntry(FlowGraphCommands.EnablePinBreakpoint);
			Section.AddMenuEntry(FlowGraphCommands.DisablePinBreakpoint);
			Section.AddMenuEntry(FlowGraphCommands.TogglePinBreakpoint);
		}

		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphPinExecutionOverride", LOCTEXT("PinExecutionOverrideMenuHeader", "Execution Override"));
			Section.AddMenuEntry(FlowGraphCommands.ForcePinActivation);
		}
	}
	else if (Context->Node)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphNodeActions", LOCTEXT("NodeActionsMenuHeader", "Node Actions"));
			Section.AddMenuEntry(GenericCommands.Delete);
			Section.AddMenuEntry(GenericCommands.Cut);
			Section.AddMenuEntry(GenericCommands.Copy);
			Section.AddMenuEntry(GenericCommands.Duplicate);

			Section.AddMenuEntry(GraphCommands.BreakNodeLinks);

			if (SupportsContextPins())
			{
				Section.AddMenuEntry(FlowGraphCommands.RefreshContextPins);
			}

			if (CanUserAddInput())
			{
				Section.AddMenuEntry(FlowGraphCommands.AddInput);
			}
			if (CanUserAddOutput())
			{
				Section.AddMenuEntry(FlowGraphCommands.AddOutput);
			}
		}

		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphNodeBreakpoints", LOCTEXT("NodeBreakpointsMenuHeader", "Node Breakpoints"));
			Section.AddMenuEntry(GraphCommands.AddBreakpoint);
			Section.AddMenuEntry(GraphCommands.RemoveBreakpoint);
			Section.AddMenuEntry(GraphCommands.EnableBreakpoint);
			Section.AddMenuEntry(GraphCommands.DisableBreakpoint);
			Section.AddMenuEntry(GraphCommands.ToggleBreakpoint);
		}

		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphNodeJumps", LOCTEXT("NodeJumpsMenuHeader", "Jumps"));
			if (CanFocusViewport())
			{
				Section.AddMenuEntry(FlowGraphCommands.FocusViewport);
			}
			if (CanJumpToDefinition())
			{
				Section.AddMenuEntry(FlowGraphCommands.JumpToNodeDefinition);
			}
		}
	}
}

bool UFlowGraphNode::CanUserDeleteNode() const
{
	return FlowNode ? FlowNode->bCanDelete : Super::CanUserDeleteNode();
}

bool UFlowGraphNode::CanDuplicateNode() const
{
	return FlowNode ? FlowNode->bCanDuplicate : Super::CanDuplicateNode();
}

TSharedPtr<SGraphNode> UFlowGraphNode::CreateVisualWidget()
{
	return SNew(SFlowGraphNode, this);
}

FText UFlowGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (FlowNode)
	{
		if (UFlowGraphEditorSettings::Get()->bShowNodeClass)
		{
			FString CleanAssetName;
			if (FlowNode->GetClass()->ClassGeneratedBy)
			{
				FlowNode->GetClass()->GetPathName(nullptr, CleanAssetName);
				const int32 SubStringIdx = CleanAssetName.Find(".", ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				CleanAssetName.LeftInline(SubStringIdx);
			}
			else
			{
				CleanAssetName = FlowNode->GetClass()->GetName();
			}

			FFormatNamedArguments Args;
			Args.Add(TEXT("NodeTitle"), FlowNode->GetNodeTitle());
			Args.Add(TEXT("AssetName"), FText::FromString(CleanAssetName));
			return FText::Format(INVTEXT("{NodeTitle}\n{AssetName}"), Args);
		}

		return FlowNode->GetNodeTitle();
	}

	return Super::GetNodeTitle(TitleType);
}

FLinearColor UFlowGraphNode::GetNodeTitleColor() const
{
	if (FlowNode)
	{
		FLinearColor DynamicColor;
		if (FlowNode->GetDynamicTitleColor(DynamicColor))
		{
			return DynamicColor;
		}

		UFlowGraphSettings* GraphSettings = UFlowGraphSettings::Get();
		if (const FLinearColor* NodeSpecificColor = GraphSettings->NodeSpecificColors.Find(FlowNode->GetClass()))
		{
			return *NodeSpecificColor;
		}
		if (const FLinearColor* StyleColor = GraphSettings->NodeTitleColors.Find(FlowNode->GetNodeStyle()))
		{
			return *StyleColor;
		}
	}

	return Super::GetNodeTitleColor();
}

FSlateIcon UFlowGraphNode::GetIconAndTint(FLinearColor& OutColor) const
{
	return FSlateIcon();
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

FString UFlowGraphNode::GetNodeDescription() const
{
	return FlowNode ? FlowNode->GetNodeDescription() : FString();
}

UFlowNode* UFlowGraphNode::GetInspectedNodeInstance() const
{
	return FlowNode ? FlowNode->GetInspectedInstance() : nullptr;
}

EFlowNodeState UFlowGraphNode::GetActivationState() const
{
	if (FlowNode)
	{
		if (const UFlowNode* NodeInstance = FlowNode->GetInspectedInstance())
		{
			return NodeInstance->GetActivationState();
		}
	}

	return EFlowNodeState::NeverActivated;
}

FString UFlowGraphNode::GetStatusString() const
{
	if (FlowNode)
	{
		if (const UFlowNode* NodeInstance = FlowNode->GetInspectedInstance())
		{
			return NodeInstance->GetStatusString();
		}
	}

	return FString();
}

bool UFlowGraphNode::IsContentPreloaded() const
{
	if (FlowNode)
	{
		if (const UFlowNode* NodeInstance = FlowNode->GetInspectedInstance())
		{
			return NodeInstance->bPreloaded;
		}
	}

	return false;
}

bool UFlowGraphNode::CanFocusViewport() const
{
	return FlowNode ? (GEditor->bIsSimulatingInEditor && FlowNode->GetActorToFocus()) : false;
}

bool UFlowGraphNode::CanJumpToDefinition() const
{
	return FlowNode != nullptr;
}

void UFlowGraphNode::JumpToDefinition() const
{
	if (FlowNode)
	{
		if (FlowNode->GetClass()->IsNative())
		{
			if (FSourceCodeNavigation::CanNavigateToClass(FlowNode->GetClass()))
			{
				const bool bSucceeded = FSourceCodeNavigation::NavigateToClass(FlowNode->GetClass());
				if (bSucceeded)
				{
					return;
				}
			}

			// Failing that, fall back to the older method which will still get the file open assuming it exists
			FString NativeParentClassHeaderPath;
			const bool bFileFound = FSourceCodeNavigation::FindClassHeaderPath(FlowNode->GetClass(), NativeParentClassHeaderPath) && (IFileManager::Get().FileSize(*NativeParentClassHeaderPath) != INDEX_NONE);
			if (bFileFound)
			{
				const FString AbsNativeParentClassHeaderPath = FPaths::ConvertRelativePathToFull(NativeParentClassHeaderPath);
				FSourceCodeNavigation::OpenSourceFile(AbsNativeParentClassHeaderPath);
			}
		}
		else
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(FlowNode->GetClass());
		}
	}
}

void UFlowGraphNode::CreateInputPin(const FFlowPin& FlowPin, const int32 Index /*= INDEX_NONE*/)
{
	if (FlowPin.PinName.IsNone())
	{
		return;
	}

	const FEdGraphPinType PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Exec, FName(NAME_None), nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
	UEdGraphPin* NewPin = CreatePin(EGPD_Input, PinType, FlowPin.PinName, Index);
	check(NewPin);
	NewPin->PinToolTip = FlowPin.PinToolTip;

	InputPins.Emplace(NewPin);
}

void UFlowGraphNode::CreateOutputPin(const FFlowPin& FlowPin, const int32 Index /*= INDEX_NONE*/)
{
	if (FlowPin.PinName.IsNone())
	{
		return;
	}

	const FEdGraphPinType PinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Exec, FName(NAME_None), nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
	UEdGraphPin* NewPin = CreatePin(EGPD_Output, PinType, FlowPin.PinName, Index);
	check(NewPin);
	NewPin->PinToolTip = FlowPin.PinToolTip;

	OutputPins.Emplace(NewPin);
}

void UFlowGraphNode::RemoveOrphanedPin(UEdGraphPin* Pin)
{
	const FScopedTransaction Transaction(LOCTEXT("RemoveOrphanedPin", "Remove Orphaned Pin"));
	Modify();

	PinBreakpoints.Remove(Pin);

	Pin->MarkPendingKill();
	Pins.Remove(Pin);

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
}

bool UFlowGraphNode::SupportsContextPins() const
{
	return FlowNode && FlowNode->SupportsContextPins();
}

bool UFlowGraphNode::CanUserAddInput() const
{
	return FlowNode && FlowNode->CanUserAddInput() && InputPins.Num() < 256;
}

bool UFlowGraphNode::CanUserAddOutput() const
{
	return FlowNode && FlowNode->CanUserAddOutput() && OutputPins.Num() < 256;
}

bool UFlowGraphNode::CanUserRemoveInput(const UEdGraphPin* Pin) const
{
	return FlowNode && FlowNode->InputPins.Num() > FlowNode->GetClass()->GetDefaultObject<UFlowNode>()->InputPins.Num();
}

bool UFlowGraphNode::CanUserRemoveOutput(const UEdGraphPin* Pin) const
{
	return FlowNode && FlowNode->OutputPins.Num() > FlowNode->GetClass()->GetDefaultObject<UFlowNode>()->OutputPins.Num();
}

void UFlowGraphNode::AddUserInput()
{
	AddInstancePin(EGPD_Input, *FString::FromInt(InputPins.Num()));
}

void UFlowGraphNode::AddUserOutput()
{
	AddInstancePin(EGPD_Output, *FString::FromInt(OutputPins.Num()));
}

void UFlowGraphNode::AddInstancePin(const EEdGraphPinDirection Direction, const FName& PinName)
{
	const FScopedTransaction Transaction(LOCTEXT("AddInstancePin", "Add Instance Pin"));
	Modify();

	if (Direction == EGPD_Input)
	{
		FlowNode->InputPins.Emplace(PinName);
		CreateInputPin(FlowNode->InputPins.Last());
	}
	else
	{
		FlowNode->OutputPins.Emplace(PinName);
		CreateOutputPin(FlowNode->OutputPins.Last());
	}

	GetGraph()->NotifyGraphChanged();
}

void UFlowGraphNode::RemoveInstancePin(UEdGraphPin* Pin)
{
	const FScopedTransaction Transaction(LOCTEXT("RemoveInstancePin", "Remove Instance Pin"));
	Modify();

	PinBreakpoints.Remove(Pin);

	if (Pin->Direction == EGPD_Input)
	{
		if (InputPins.Contains(Pin))
		{
			InputPins.Remove(Pin);
			FlowNode->RemoveUserInput();

			Pin->MarkPendingKill();
			Pins.Remove(Pin);
		}
	}
	else
	{
		if (OutputPins.Contains(Pin))
		{
			OutputPins.Remove(Pin);
			FlowNode->RemoveUserOutput();

			Pin->MarkPendingKill();
			Pins.Remove(Pin);
		}
	}

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
}

void UFlowGraphNode::RefreshContextPins(const bool bReconstructNode)
{
	if (SupportsContextPins())
	{
		const FScopedTransaction Transaction(LOCTEXT("RefreshContextPins", "Refresh Context Pins"));
		Modify();

		const UFlowNode* NodeDefaults = FlowNode->GetClass()->GetDefaultObject<UFlowNode>();

		// recreate inputs
		FlowNode->InputPins = NodeDefaults->InputPins;
		FlowNode->AddInputPins(FlowNode->GetContextInputs());

		// recreate outputs
		FlowNode->OutputPins = NodeDefaults->OutputPins;
		FlowNode->AddOutputPins(FlowNode->GetContextOutputs());

		if (bReconstructNode)
		{
			ReconstructNode();
			GetGraph()->NotifyGraphChanged();
		}
	}
}

void UFlowGraphNode::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	// start with the default hover text (from the pin's tool-tip)
	Super::GetPinHoverText(Pin, HoverTextOut);

	// add information on pin activations
	if (GEditor->PlayWorld)
	{
		if (const UFlowNode* InspectedNodeInstance = GetInspectedNodeInstance())
		{
			if (!HoverTextOut.IsEmpty())
			{
				HoverTextOut.Append(LINE_TERMINATOR).Append(LINE_TERMINATOR);
			}

			const TArray<FPinRecord>& PinRecords = InspectedNodeInstance->GetPinRecords(Pin.PinName, Pin.Direction);
			if (PinRecords.Num() == 0)
			{
				HoverTextOut.Append(FPinRecord::NoActivations);
			}
			else
			{
				HoverTextOut.Append(FPinRecord::PinActivations);
				for (int32 i = 0; i < PinRecords.Num(); i++)
				{
					HoverTextOut.Append(LINE_TERMINATOR);
					HoverTextOut.Appendf(TEXT("%d) %s"), i + 1, *PinRecords[i].HumanReadableTime);

					if (PinRecords[i].bForcedActivation)
					{
						HoverTextOut.Append(FPinRecord::ForcedActivation);
					}
				}
			}
		}
	}
}

void UFlowGraphNode::OnInputTriggered(const int32 Index)
{
	if (InputPins.IsValidIndex(Index) && PinBreakpoints.Contains(InputPins[Index]))
	{
		PinBreakpoints[InputPins[Index]].bBreakpointHit = true;
		TryPausingSession(true);
	}

	TryPausingSession(false);
}

void UFlowGraphNode::OnOutputTriggered(const int32 Index)
{
	if (OutputPins.IsValidIndex(Index) && PinBreakpoints.Contains(OutputPins[Index]))
	{
		PinBreakpoints[OutputPins[Index]].bBreakpointHit = true;
		TryPausingSession(true);
	}

	TryPausingSession(false);
}

void UFlowGraphNode::TryPausingSession(bool bPauseSession)
{
	// Node breakpoints waits on any pin triggered
	if (NodeBreakpoint.IsBreakpointEnabled())
	{
		NodeBreakpoint.bBreakpointHit = true;
		bPauseSession = true;
	}

	if (bPauseSession)
	{
		FEditorDelegates::ResumePIE.AddUObject(this, &UFlowGraphNode::OnResumePIE);
		FEditorDelegates::EndPIE.AddUObject(this, &UFlowGraphNode::OnEndPIE);

		FFlowDebugger::PausePlaySession();
	}
}

void UFlowGraphNode::OnResumePIE(const bool bIsSimulating)
{
	ResetBreakpoints();
}

void UFlowGraphNode::OnEndPIE(const bool bIsSimulating)
{
	ResetBreakpoints();
}

void UFlowGraphNode::ResetBreakpoints()
{
	FEditorDelegates::ResumePIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);

	NodeBreakpoint.bBreakpointHit = false;
	for (TPair<FEdGraphPinReference, FFlowBreakpoint>& PinBreakpoint : PinBreakpoints)
	{
		PinBreakpoint.Value.bBreakpointHit = false;
	}
}

void UFlowGraphNode::ForcePinActivation(const FEdGraphPinReference PinReference) const
{
	UFlowNode* InspectedNodeInstance = GetInspectedNodeInstance();
	if (InspectedNodeInstance == nullptr)
	{
		return;
	}

	if (const UEdGraphPin* FoundPin = PinReference.Get())
	{
		switch (FoundPin->Direction)
		{
			case EGPD_Input:
				InspectedNodeInstance->TriggerInput(FoundPin->PinName, true);
				break;
			case EGPD_Output:
				InspectedNodeInstance->TriggerOutput(FoundPin->PinName, false, true);
				break;
			default: ;
		}
	}
}

#undef LOCTEXT_NAMESPACE
