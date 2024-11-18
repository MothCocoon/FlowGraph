// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/Nodes/FlowGraphNode.h"

#include "AddOns/FlowNodeAddOn.h"
#include "Asset/FlowDebuggerSubsystem.h"
#include "FlowEditorCommands.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSettings.h"
#include "Graph/Widgets/SFlowGraphNode.h"
#include "Graph/Widgets/SGraphEditorActionMenuFlow.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

#include "BlueprintNodeHelpers.h"
#include "Developer/ToolMenus/Public/ToolMenus.h"
#include "DiffResults.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphDiffControl.h"
#include "GraphEditorActions.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ScopedTransaction.h"
#include "SourceCodeNavigation.h"
#include "Textures/SlateIcon.h"
#include "ToolMenuSection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraphNode)

#define LOCTEXT_NAMESPACE "FlowGraphNode"

UFlowGraphNode::UFlowGraphNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NodeInstance(nullptr)
	, bBlueprintCompilationPending(false)
	, bIsReconstructingNode(false)
	, bNeedsFullReconstruction(false)
{
	OrphanedPinSaveMode = ESaveOrphanPinMode::SaveAll;
}

void UFlowGraphNode::SetNodeTemplate(UFlowNodeBase* InFlowNode)
{
	NodeInstance = InFlowNode;
}

const UFlowNodeBase* UFlowGraphNode::GetNodeTemplate() const
{
	return NodeInstance;
}

UFlowNodeBase* UFlowGraphNode::GetFlowNodeBase() const
{
	if (NodeInstance)
	{
		if (UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance))
		{
			if (const UFlowAsset* InspectedInstance = FlowNode->GetFlowAsset()->GetInspectedInstance())
			{
				return InspectedInstance->GetNode(FlowNode->GetGuid());
			}
		}

		return NodeInstance;
	}

	return nullptr;
}

void UFlowGraphNode::PostLoad()
{
	Super::PostLoad();

	if (NodeInstance)
	{
		NodeInstance->FixNode(this); // fix already created nodes
		SubscribeToExternalChanges();
	}
}

void UFlowGraphNode::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		CreateNewGuid();

		UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
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

	// Reset the owning graph after an edit import
	ResetNodeOwner();

	if (NodeInstance)
	{
		InitializeInstance();
	}
}

void UFlowGraphNode::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	SubscribeToExternalChanges();

	// NOTE - NodeInstance can be already spawned by paste operation, don't override it

	if (NodeInstanceClass.IsPending())
	{
		NodeInstanceClass.LoadSynchronous();
	}

	UClass* NodeClass = NodeInstanceClass.Get();
	if (NodeClass && (NodeInstance == nullptr))
	{
		UEdGraph* MyGraph = GetGraph();
		UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr;
		if (GraphOwner)
		{
			NodeInstance = Cast<UFlowNodeBase>(NewObject<UObject>(GraphOwner, NodeClass));
			NodeInstance->SetFlags(RF_Transactional);

			InitializeInstance();
		}
	}
}

void UFlowGraphNode::PrepareForCopying()
{
	Super::PrepareForCopying();

	if (NodeInstance)
	{
		// Temporarily take ownership of the node instance, so that it is not deleted when cutting
		NodeInstance->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
	}
}

void UFlowGraphNode::PostCopyNode()
{
	// Make sure this NodeInstance is owned by the FlowAsset it's being pasted into
	if (NodeInstance)
	{
		UFlowAsset* FlowAsset = GetFlowAsset();

		if (NodeInstance->GetOuter() != FlowAsset)
		{
			// Ensures NodeInstance is owned by the FlowAsset
			NodeInstance->Rename(nullptr, FlowAsset, REN_DontCreateRedirectors);
		}

		NodeInstance->SetGraphNode(this);
	}

	// Reset the node's owning graph prior to copying
	ResetNodeOwner();
}

void UFlowGraphNode::SubscribeToExternalChanges()
{
	if (NodeInstance)
	{
		NodeInstance->OnReconstructionRequested.BindUObject(this, &UFlowGraphNode::OnExternalChange);
	}
}

void UFlowGraphNode::OnExternalChange()
{
	if (bIsReconstructingNode)
	{
		return;
	}

	// Do not create transaction here, since this function triggers from modifying UFlowNode's property, which itself already made inside of transaction.
	Modify();

	bNeedsFullReconstruction = true;

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
}

void UFlowGraphNode::OnGraphRefresh()
{
	RefreshContextPins(true);
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
	if (const UFlowGraph* FlowGraph = GetFlowGraph())
	{
		// If the graph is locked, we shouldn't reconstruct nodes 
		// (all nodes will all be reconstructed when the graph is unlocked)

		if (FlowGraph->IsLocked())
		{
			return;
		}
	}

	if (bIsReconstructingNode)
	{
		return;
	}

	bIsReconstructingNode = true;

	// Store old pins
	TArray<UEdGraphPin*> OldPins(Pins);

	// Reset pin arrays
	Pins.Reset();
	InputPins.Reset();
	OutputPins.Reset();

	bool bChangedAutoFlowPins = false;

	// Harvest the auto-generated pins before refreshing context pins
	if (UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance))
	{
		if (UFlowAsset* FlowAsset = NodeInstance->GetFlowAsset())
		{
			bChangedAutoFlowPins = FlowAsset->TryUpdateManagedFlowPinsForNode(*FlowNode);
		}
	}

	// Recreate pins
	constexpr bool bReconstructNode = false;
	RefreshContextPins(bReconstructNode);

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
	bIsReconstructingNode = false;
}

void UFlowGraphNode::AllocateDefaultPins()
{
	check(Pins.Num() == 0);

	if (UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance))
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
			InOldPins.RemoveAt(OldPinIndex, 1, false);
#else
			InOldPins.RemoveAt(OldPinIndex, 1, EAllowShrinking::No);
#endif
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
	for (TPair<FEdGraphPinReference, FFlowPinTrait>& PinBreakpoint : PinBreakpoints)
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
			FToolMenuSection& Section = Menu->AddSection("FlowGraphNodeAddOns", LOCTEXT("NodeAddOnsMenuHeader", "AddOns"));
			Section.AddSubMenu(
				"AttachAddOn",
				LOCTEXT("AttachAddOn", "Attach AddOn..."),
				LOCTEXT("AttachAddOnTooltip", "Attaches an AddOn to the Node"),
				FNewToolMenuDelegate::CreateUObject(this, &UFlowGraphNode::CreateAttachAddOnSubMenu, (UEdGraph*)Context->Graph)
			);
		}

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
			FToolMenuSection& Section = Menu->AddSection("FlowGraphNodeExecutionOverride", LOCTEXT("NodeExecutionOverrideMenuHeader", "Execution Override"));
			if (CanSetSignalMode(EFlowSignalMode::Enabled))
			{
				Section.AddMenuEntry(FlowGraphCommands.EnableNode);
			}
			if (CanSetSignalMode(EFlowSignalMode::Disabled))
			{
				Section.AddMenuEntry(FlowGraphCommands.DisableNode);
			}
			if (CanSetSignalMode(EFlowSignalMode::PassThrough))
			{
				Section.AddMenuEntry(FlowGraphCommands.SetPassThrough);
			}
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

		{
			FToolMenuSection& Section = Menu->AddSection("FlowGraphNodeOrganisation", LOCTEXT("NodeOrganisation", "Organisation"));
			Section.AddSubMenu("Alignment", LOCTEXT("AlignmentHeader", "Alignment"), FText(), FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
			{
				FToolMenuSection& SubMenuSection = SubMenu->AddSection("EdGraphSchemaAlignment", LOCTEXT("AlignHeader", "Align"));
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesTop);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesMiddle);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesBottom);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesLeft);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesCenter);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesRight);
				SubMenuSection.AddMenuEntry(FGraphEditorCommands::Get().StraightenConnections);
			}));
		}
	}
}

void UFlowGraphNode::CreateAttachAddOnSubMenu(UToolMenu* Menu, UEdGraph* Graph) const
{
	UFlowGraphNode* MutableThis = const_cast<UFlowGraphNode*>(this);

	TSharedRef<SGraphEditorActionMenuFlow> Widget =
		SNew(SGraphEditorActionMenuFlow)
		.GraphObj(Graph)
		.GraphNode(MutableThis)
		.AutoExpandActionMenu(true);

	Menu->AddMenuEntry("Section", FToolMenuEntry::InitWidget("Widget", Widget, FText(), true));
}

bool UFlowGraphNode::CanUserDeleteNode() const
{
	return NodeInstance ? NodeInstance->bCanDelete : Super::CanUserDeleteNode();
}

bool UFlowGraphNode::CanDuplicateNode() const
{
	return NodeInstance ? NodeInstance->bCanDuplicate : Super::CanDuplicateNode();
}

TSharedPtr<SGraphNode> UFlowGraphNode::CreateVisualWidget()
{
	return SNew(SFlowGraphNode, this);
}

FText UFlowGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (NodeInstance)
	{
		if (UFlowGraphEditorSettings::Get()->bShowNodeClass)
		{
			FString CleanAssetName;
			if (NodeInstance->GetClass()->ClassGeneratedBy)
			{
				NodeInstance->GetClass()->GetPathName(nullptr, CleanAssetName);
				const int32 SubStringIdx = CleanAssetName.Find(".", ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				CleanAssetName.LeftInline(SubStringIdx);
			}
			else
			{
				CleanAssetName = NodeInstance->GetClass()->GetName();
			}

			FFormatNamedArguments Args;
			Args.Add(TEXT("NodeTitle"), NodeInstance->GetNodeTitle());
			Args.Add(TEXT("AssetName"), FText::FromString(CleanAssetName));
			return FText::Format(INVTEXT("{NodeTitle}\n{AssetName}"), Args);
		}

		return NodeInstance->GetNodeTitle();
	}

	return Super::GetNodeTitle(TitleType);
}

FLinearColor UFlowGraphNode::GetNodeTitleColor() const
{
	if (NodeInstance)
	{
		FLinearColor DynamicColor;
		if (NodeInstance->GetDynamicTitleColor(DynamicColor))
		{
			return DynamicColor;
		}

		if (const FLinearColor* StyleColor = UFlowGraphSettings::Get()->LookupNodeTitleColorForNode(*NodeInstance))
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
	if (NodeInstance)
	{
		Tooltip = NodeInstance->GetNodeToolTip();
	}
	if (Tooltip.IsEmpty())
	{
		Tooltip = GetNodeTitle(ENodeTitleType::ListView);
	}
	return Tooltip;
}

FString UFlowGraphNode::GetNodeDescription() const
{
	if (NodeInstance && (GEditor->PlayWorld == nullptr || UFlowGraphEditorSettings::Get()->bShowNodeDescriptionWhilePlaying))
	{
		return NodeInstance->GetNodeDescription();
	}

	return FString();
}

UFlowNode* UFlowGraphNode::GetInspectedNodeInstance() const
{
	const UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	return FlowNode ? FlowNode->GetInspectedInstance() : nullptr;
}

EFlowNodeState UFlowGraphNode::GetActivationState() const
{
	const UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	if (FlowNode)
	{
		if (const UFlowNode* InspectedInstance = FlowNode->GetInspectedInstance())
		{
			return InspectedInstance->GetActivationState();
		}
	}

	return EFlowNodeState::NeverActivated;
}

FString UFlowGraphNode::GetStatusString() const
{
	const UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	if (FlowNode)
	{
		if (const UFlowNode* InspectedInstance = FlowNode->GetInspectedInstance())
		{
			return InspectedInstance->GetStatusStringForNodeAndAddOns();
		}
	}

	return FString();
}

FLinearColor UFlowGraphNode::GetStatusBackgroundColor() const
{
	const UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	if (FlowNode)
	{
		if (const UFlowNode* InspectedInstance = FlowNode->GetInspectedInstance())
		{
			FLinearColor ObtainedColor;
			if (InspectedInstance->GetStatusBackgroundColor(ObtainedColor))
			{
				return ObtainedColor;
			}
		}
	}

	return UFlowGraphSettings::Get()->NodeStatusBackground;
}

bool UFlowGraphNode::IsContentPreloaded() const
{
	const UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	if (FlowNode)
	{
		if (const UFlowNode* InspectedInstance = FlowNode->GetInspectedInstance())
		{
			return InspectedInstance->bPreloaded;
		}
	}

	return false;
}

bool UFlowGraphNode::CanFocusViewport() const
{
	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	return FlowNode ? (GEditor->bIsSimulatingInEditor && FlowNode->GetActorToFocus()) : false;
}

bool UFlowGraphNode::CanJumpToDefinition() const
{
	return NodeInstance != nullptr;
}

void UFlowGraphNode::JumpToDefinition() const
{
	if (NodeInstance)
	{
		if (NodeInstance->GetClass()->IsNative())
		{
			if (FSourceCodeNavigation::CanNavigateToClass(NodeInstance->GetClass()))
			{
				const bool bSucceeded = FSourceCodeNavigation::NavigateToClass(NodeInstance->GetClass());
				if (bSucceeded)
				{
					return;
				}
			}

			// Failing that, fall back to the older method which will still get the file open assuming it exists
			FString NativeParentClassHeaderPath;
			const bool bFileFound = FSourceCodeNavigation::FindClassHeaderPath(NodeInstance->GetClass(), NativeParentClassHeaderPath) && (IFileManager::Get().FileSize(*NativeParentClassHeaderPath) != INDEX_NONE);
			if (bFileFound)
			{
				const FString AbsNativeParentClassHeaderPath = FPaths::ConvertRelativePathToFull(NativeParentClassHeaderPath);
				FSourceCodeNavigation::OpenSourceFile(AbsNativeParentClassHeaderPath);
			}
		}
		else
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(NodeInstance->GetClass());
		}
	}
}

bool UFlowGraphNode::SupportsCommentBubble() const
{
	if (IsSubNode())
	{
		return false;
	}

	return Super::SupportsCommentBubble();
}

void UFlowGraphNode::CreateInputPin(const FFlowPin& FlowPin, const int32 Index /*= INDEX_NONE*/)
{
	if (FlowPin.PinName.IsNone())
	{
		return;
	}

	const FName PinCategory = GetPinCategoryFromFlowPin(FlowPin);
	const FName PinSubCategory = NAME_None;
	UObject* PinSubCategoryObject = FlowPin.GetPinSubCategoryObject().Get();
	constexpr bool bIsReference = false;

	const FEdGraphPinType PinType = FEdGraphPinType(PinCategory, PinSubCategory, PinSubCategoryObject, EPinContainerType::None, bIsReference, FEdGraphTerminalType());
	UEdGraphPin* NewPin = CreatePin(EGPD_Input, PinType, FlowPin.PinName, Index);
	check(NewPin);

	if (!FlowPin.PinFriendlyName.IsEmpty())
	{
		NewPin->bAllowFriendlyName = true;
		NewPin->PinFriendlyName = FlowPin.PinFriendlyName;
	}

	NewPin->PinToolTip = FlowPin.PinToolTip;

	InputPins.Emplace(NewPin);
}

void UFlowGraphNode::CreateOutputPin(const FFlowPin& FlowPin, const int32 Index /*= INDEX_NONE*/)
{
	if (FlowPin.PinName.IsNone())
	{
		return;
	}

	const FName PinCategory = GetPinCategoryFromFlowPin(FlowPin);
	const FName PinSubCategory = NAME_None;
	UObject* PinSubCategoryObject = FlowPin.GetPinSubCategoryObject().Get();
	constexpr bool bIsReference = false;

	const FEdGraphPinType PinType = FEdGraphPinType(PinCategory, PinSubCategory, PinSubCategoryObject, EPinContainerType::None, bIsReference, FEdGraphTerminalType());
	UEdGraphPin* NewPin = CreatePin(EGPD_Output, PinType, FlowPin.PinName, Index);
	check(NewPin);

	if (!FlowPin.PinFriendlyName.IsEmpty())
	{
		NewPin->bAllowFriendlyName = true;
		NewPin->PinFriendlyName = FlowPin.PinFriendlyName;
	}

	NewPin->PinToolTip = FlowPin.PinToolTip;

	OutputPins.Emplace(NewPin);
}

void UFlowGraphNode::RemoveOrphanedPin(UEdGraphPin* Pin)
{
	const FScopedTransaction Transaction(LOCTEXT("RemoveOrphanedPin", "Remove Orphaned Pin"));
	Modify();

	PinBreakpoints.Remove(Pin);

	Pin->MarkAsGarbage();
	Pins.Remove(Pin);

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
}

bool UFlowGraphNode::SupportsContextPins() const
{
	return NodeInstance && NodeInstance->SupportsContextPins();
}

bool UFlowGraphNode::CanUserAddInput() const
{
	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	return FlowNode && FlowNode->CanUserAddInput() && InputPins.Num() < 256;
}

bool UFlowGraphNode::CanUserAddOutput() const
{
	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	return FlowNode && FlowNode->CanUserAddOutput() && OutputPins.Num() < 256;
}

bool UFlowGraphNode::CanUserRemoveInput(const UEdGraphPin* Pin) const
{
	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	return FlowNode && !FlowNode->GetClass()->GetDefaultObject<UFlowNode>()->InputPins.Contains(Pin->PinName);
}

bool UFlowGraphNode::CanUserRemoveOutput(const UEdGraphPin* Pin) const
{
	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	return FlowNode && !FlowNode->GetClass()->GetDefaultObject<UFlowNode>()->OutputPins.Contains(Pin->PinName);
}

void UFlowGraphNode::AddUserInput()
{
	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	AddInstancePin(EGPD_Input, FlowNode->CountNumberedInputs());
}

void UFlowGraphNode::AddUserOutput()
{
	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	AddInstancePin(EGPD_Output, FlowNode->CountNumberedOutputs());
}

void UFlowGraphNode::AddInstancePin(const EEdGraphPinDirection Direction, const uint8 NumberedPinsAmount)
{
	const FScopedTransaction Transaction(LOCTEXT("AddInstancePin", "Add Instance Pin"));
	Modify();

	const FFlowPin PinName = FFlowPin(FString::FromInt(NumberedPinsAmount));

	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	if (Direction == EGPD_Input)
	{
		if (FlowNode->InputPins.IsValidIndex(NumberedPinsAmount))
		{
			FlowNode->InputPins.Insert(PinName, NumberedPinsAmount);
		}
		else
		{
			FlowNode->InputPins.Add(PinName);
		}

		CreateInputPin(PinName, NumberedPinsAmount);
	}
	else
	{
		if (FlowNode->OutputPins.IsValidIndex(NumberedPinsAmount))
		{
			FlowNode->OutputPins.Insert(PinName, NumberedPinsAmount);
		}
		else
		{
			FlowNode->OutputPins.Add(PinName);
		}

		CreateOutputPin(PinName, FlowNode->InputPins.Num() + NumberedPinsAmount);
	}

	GetGraph()->NotifyGraphChanged();
}

void UFlowGraphNode::RemoveInstancePin(UEdGraphPin* Pin)
{
	const FScopedTransaction Transaction(LOCTEXT("RemoveInstancePin", "Remove Instance Pin"));
	Modify();

	PinBreakpoints.Remove(Pin);

	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	if (Pin->Direction == EGPD_Input)
	{
		if (InputPins.Contains(Pin))
		{
			InputPins.Remove(Pin);
			FlowNode->RemoveUserInput(Pin->PinName);

			Pin->MarkAsGarbage();
			Pins.Remove(Pin);
		}
	}
	else
	{
		if (OutputPins.Contains(Pin))
		{
			OutputPins.Remove(Pin);
			FlowNode->RemoveUserOutput(Pin->PinName);

			Pin->MarkAsGarbage();
			Pins.Remove(Pin);
		}
	}

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
}

void UFlowGraphNode::RefreshContextPins(const bool bReconstructNode)
{
	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	if (!IsValid(FlowNode))
	{
		return;
	}

	// Update the auto-generated pins before refreshing context pins
	bool bChangedAutoFlowPins = false;
	if (UFlowAsset* FlowAsset = NodeInstance->GetFlowAsset())
	{
		bChangedAutoFlowPins = FlowAsset->TryUpdateManagedFlowPinsForNode(*FlowNode);
	}

	bool bIsLoad = false;
	if (UFlowGraph* FlowGraph = GetFlowGraph())
	{
		bIsLoad = FlowGraph->IsLoadingGraph();
	}

	// Confirm that we should be refreshing context pins
	const bool bIsAllowedToRefreshPins = !bIsLoad || NodeInstance->CanRefreshContextPinsOnLoad();
	const bool bShouldConsiderRefreshingContextPins = bIsAllowedToRefreshPins && (SupportsContextPins() || bHasContextPins);
	const bool bShouldRefreshContextPins = bShouldConsiderRefreshingContextPins || bChangedAutoFlowPins || bNeedsFullReconstruction;

	if (!bShouldRefreshContextPins)
	{
		return;
	}

	const TArray<FFlowPin> ContextInputs = FlowNode->GetContextInputs();
	const TArray<FFlowPin> ContextOutputs = FlowNode->GetContextOutputs();

	const bool bPrevHasContextPins = bHasContextPins;
	bHasContextPins = !ContextInputs.IsEmpty() || !ContextOutputs.IsEmpty();

	// Skip the rest if the node went from no ContextPins to no ContextPins
	const bool bMaintainedNoContextPins = !bPrevHasContextPins && !bHasContextPins;

	if (bMaintainedNoContextPins || !HavePinsChanged())
	{
		// We dont have contextual pins to account for; or the contextual pins have not changed. We can skip now. 
		return;
	}
		
	const FScopedTransaction Transaction(LOCTEXT("RefreshContextPins", "Refresh Context Pins"));
	Modify();

	const UFlowNode* NodeDefaults = FlowNode->GetClass()->GetDefaultObject<UFlowNode>();

	// recreate inputs
	FlowNode->InputPins = NodeDefaults->InputPins;
	FlowNode->AddInputPins(ContextInputs);

	// recreate outputs
	FlowNode->OutputPins = NodeDefaults->OutputPins;
	FlowNode->AddOutputPins(ContextOutputs);

	if (bReconstructNode)
	{
		ReconstructNode();
		GetGraph()->NotifyGraphChanged();
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

					switch (PinRecords[i].ActivationType)
					{
						case EFlowPinActivationType::Default:
							break;
						case EFlowPinActivationType::Forced:
							HoverTextOut.Append(FPinRecord::ForcedActivation);
							break;
						case EFlowPinActivationType::PassThrough:
							HoverTextOut.Append(FPinRecord::PassThroughActivation);
							break;
						default: ;
					}
				}
			}
		}
	}
}

const FName& UFlowGraphNode::GetPinCategoryFromFlowPin(const FFlowPin& FlowPin) const
{
	return FFlowPin::GetPinCategoryFromPinType(FlowPin.GetPinType());
}

void UFlowGraphNode::OnInputTriggered(const int32 Index)
{
	if (InputPins.IsValidIndex(Index) && PinBreakpoints.Contains(InputPins[Index]))
	{
		PinBreakpoints[InputPins[Index]].MarkAsHit();
		TryPausingSession(true);
	}

	TryPausingSession(false);
}

void UFlowGraphNode::OnOutputTriggered(const int32 Index)
{
	if (OutputPins.IsValidIndex(Index) && PinBreakpoints.Contains(OutputPins[Index]))
	{
		PinBreakpoints[OutputPins[Index]].MarkAsHit();
		TryPausingSession(true);
	}

	TryPausingSession(false);
}

void UFlowGraphNode::TryPausingSession(bool bPauseSession)
{
	// Node breakpoints waits on any pin triggered
	if (NodeBreakpoint.IsEnabled())
	{
		NodeBreakpoint.MarkAsHit();
		bPauseSession = true;
	}

	if (bPauseSession)
	{
		FEditorDelegates::ResumePIE.AddUObject(this, &UFlowGraphNode::OnResumePIE);
		FEditorDelegates::EndPIE.AddUObject(this, &UFlowGraphNode::OnEndPIE);

		UFlowDebuggerSubsystem::PausePlaySession();
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

	NodeBreakpoint.ResetHit();
	for (TPair<FEdGraphPinReference, FFlowPinTrait>& PinBreakpoint : PinBreakpoints)
	{
		PinBreakpoint.Value.ResetHit();
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
				InspectedNodeInstance->TriggerInput(FoundPin->PinName, EFlowPinActivationType::Forced);
				break;
			case EGPD_Output:
				InspectedNodeInstance->TriggerOutput(FoundPin->PinName, false, EFlowPinActivationType::Forced);
				break;
			default: ;
		}
	}
}

void UFlowGraphNode::SetSignalMode(const EFlowSignalMode Mode)
{
	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	if (FlowNode)
	{
		FlowNode->SignalMode = Mode;
		OnSignalModeChanged.ExecuteIfBound();
	}
}

EFlowSignalMode UFlowGraphNode::GetSignalMode() const
{
	if (IsSubNode())
	{
		// SubNodes count as enabled for signal mode queries in the editor
		return EFlowSignalMode::Enabled;
	}

	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	if (IsValid(FlowNode))
	{
		return FlowNode->SignalMode;
	}
	else
	{
		return EFlowSignalMode::Disabled;
	}
}

bool UFlowGraphNode::CanSetSignalMode(const EFlowSignalMode Mode) const
{
	UFlowNode* FlowNode = Cast<UFlowNode>(NodeInstance);
	return FlowNode ? (FlowNode->AllowedSignalModes.Contains(Mode) && FlowNode->SignalMode != Mode) : false;
}

void UFlowGraphNode::InitializeInstance()
{
	check(NodeInstance);

	// link editor and runtime nodes together
	NodeInstance->SetGraphNode(this);
}

void UFlowGraphNode::PostEditUndo()
{
	UEdGraphNode::PostEditUndo();
	ResetNodeOwner();

	if (ParentNode)
	{
		ParentNode->SubNodes.AddUnique(this);

		ParentNode->RebuildRuntimeAddOnsFromEditorSubNodes();
	}
	else
	{
		RebuildRuntimeAddOnsFromEditorSubNodes();
	}
}

UFlowAsset* UFlowGraphNode::GetFlowAsset() const
{
	if (UFlowGraph* FlowGraph = GetFlowGraph())
	{
		if (UFlowAsset* FlowAsset = FlowGraph->GetFlowAsset())
		{
			return FlowAsset;
		}
	}

	return nullptr;
}

void UFlowGraphNode::LogError(const FString& MessageToLog, const UFlowNodeBase* FlowNodeBase) const
{
	if (UFlowAsset* FlowAsset = GetFlowAsset())
	{
		FlowAsset->LogError(MessageToLog, FlowNodeBase);
	}
}

bool UFlowGraphNode::HavePinsChanged()
{
	const UFlowNode* FlowNodeInstance = Cast<UFlowNode>(NodeInstance);	
	if (!IsValid(FlowNodeInstance))
	{
		// default to having changed because we don't have a way to confirm that the pins have remained intact. 
		return true;
	}

	// Get the pins that are part of the node itself. We use the CDO because it inherently knows about the built-in
	// pins for this node. 
	const UFlowNode* FlowNodeCDO = FlowNodeInstance->GetClass()->GetDefaultObject<UFlowNode>();
	check(IsValid(FlowNodeCDO));

	TArray<FFlowPin> AllFlowPins = FlowNodeCDO->GetInputPins();
	AllFlowPins.Append(FlowNodeCDO->GetOutputPins());

	// Get the contextual pins for the underlying flow node instance.  
	AllFlowPins.Append(FlowNodeInstance->GetContextInputs());
	AllFlowPins.Append(FlowNodeInstance->GetContextOutputs());

	if (Pins.Num() != AllFlowPins.Num())
	{
		// There is a different number of EdGraphPins and Flow Node pins; something changed.
		return true;
	}
	
	TArray<FName> PinNames;
	for (const UEdGraphPin* Pin : Pins)
	{
		PinNames.Add(Pin->PinName);
	}

	for (const FFlowPin& Pin : AllFlowPins)
	{
		if (!PinNames.Contains(Pin.PinName))
		{
			// Could not match the pin from the flow node with any of the EdPins array.
			// we have a mismatch between the ed graph pins and the flow node, something changed. 
			return true;
		}
	}

	// Nothing changed
	return false;	
}

void UFlowGraphNode::ResetNodeOwner()
{
	if (NodeInstance)
	{
		UEdGraph* MyGraph = GetGraph();
		UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr;

		NodeInstance->Rename(NULL, GraphOwner, REN_DontCreateRedirectors | REN_DoNotDirty);
		NodeInstance->ClearFlags(RF_Transient);

		for (auto& SubNode : SubNodes)
		{
			SubNode->ResetNodeOwner();
		}
	}
}

FText UFlowGraphNode::GetDescription() const
{
	FString StoredClassName = NodeInstanceClass.GetAssetName();
	StoredClassName.RemoveFromEnd(TEXT("_C"));

	return FText::Format(LOCTEXT("NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
}

UEdGraphPin* UFlowGraphNode::GetInputPin(int32 InputIndex) const
{
	check(InputIndex >= 0);

	for (int32 PinIndex = 0, FoundInputs = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Input)
		{
			if (InputIndex == FoundInputs)
			{
				return Pins[PinIndex];
			}
			else
			{
				FoundInputs++;
			}
		}
	}

	return nullptr;
}

UEdGraphPin* UFlowGraphNode::GetOutputPin(int32 InputIndex) const
{
	check(InputIndex >= 0);

	for (int32 PinIndex = 0, FoundInputs = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Output)
		{
			if (InputIndex == FoundInputs)
			{
				return Pins[PinIndex];
			}
			else
			{
				FoundInputs++;
			}
		}
	}

	return nullptr;
}

UFlowGraph* UFlowGraphNode::GetFlowGraph() const
{
	return CastChecked<UFlowGraph>(GetGraph());
}

bool UFlowGraphNode::IsSubNode() const
{
	return bIsSubNode || (ParentNode != nullptr);
}

void UFlowGraphNode::NodeConnectionListChanged()
{
	Super::NodeConnectionListChanged();

	GetFlowGraph()->UpdateAsset();
}

FString UFlowGraphNode::GetPropertyNameAndValueForDiff(const FProperty* Prop, const uint8* PropertyAddr) const
{
	return BlueprintNodeHelpers::DescribeProperty(Prop, PropertyAddr);
}

void UFlowGraphNode::SetParentNodeForSubNode(UFlowGraphNode* InParentNode)
{
	if (InParentNode)
	{
		// Once a subnode, always a subnode
		bIsSubNode = true;
	}

	ParentNode = InParentNode;
}

void UFlowGraphNode::RebuildRuntimeAddOnsFromEditorSubNodes()
{
	// NOTE (gtaylor) Whenever we change the SubNodes array, we need to mirror the changes 
	// across to the AddOns array in the runtime instance data

	if (IsValid(NodeInstance))
	{
		TArray<UFlowNodeAddOn*>& NodeInstanceAddOns = NodeInstance->GetFlowNodeAddOnChildrenByEditor();
		NodeInstanceAddOns.Reset();
		NodeInstanceAddOns.Reserve(SubNodes.Num());

		for (UFlowGraphNode* SubNode : SubNodes)
		{
			if (!IsValid(SubNode))
			{
				LogError(FString::Printf(TEXT("%s: Has unexpectedly null SubNode"), *GetName()), NodeInstance);

				continue;
			}

			// Add the runtime AddOn to its runtime UFlowNode or UFlowNodeAddOn container
			UFlowNodeAddOn* AddOnSubNodeInstance = Cast<UFlowNodeAddOn>(SubNode->NodeInstance);
			if (IsValid(AddOnSubNodeInstance))
			{
				NodeInstanceAddOns.AddUnique(AddOnSubNodeInstance);
			}
			else
			{
				LogError(FString::Printf(TEXT("%s: SubNode is missing an AddOn NodeInstance"), *GetName()), NodeInstance);
			}
		}
	}

	// Update the subnodes as well
	for (UFlowGraphNode* SubNode : SubNodes)
	{
		if (IsValid(SubNode))
		{
			SubNode->RebuildRuntimeAddOnsFromEditorSubNodes();
		}
	}

	// Reconstruct the context pins for all flow nodes after their AddOns have been processed
	if (IsValid(NodeInstance) && NodeInstance->IsA<UFlowNode>())
	{
		constexpr bool bReconstructNode = true;
		RefreshContextPins(bReconstructNode);
	}
}

void UFlowGraphNode::FindDiffs(UEdGraphNode* OtherNode, FDiffResults& Results)
{
	Super::FindDiffs(OtherNode, Results);

	UFlowGraphNode* OtherGraphNode = Cast<UFlowGraphNode>(OtherNode);
	if (!IsValid(OtherGraphNode))
	{
		return;
	}

	if (NodeInstance && OtherGraphNode->NodeInstance)
	{
		FDiffSingleResult Diff;
		Diff.Diff = EDiffType::NODE_PROPERTY;
		Diff.Node1 = this;
		Diff.Node2 = OtherNode;
		Diff.Object1 = NodeInstance;
		Diff.Object2 = OtherGraphNode->NodeInstance;
		Diff.ToolTip = LOCTEXT("DIF_NodeInstancePropertyToolTip", "A property of the node instance has changed");
		Diff.Category = EDiffType::MODIFICATION;

		DiffProperties(NodeInstance->GetClass(), OtherGraphNode->NodeInstance->GetClass(), NodeInstance, OtherGraphNode->NodeInstance, Results, Diff);
	}

	DiffSubNodes(LOCTEXT("AddOnDiffDisplayName", "AddOn"), SubNodes, OtherGraphNode->SubNodes, Results);
}

void UFlowGraphNode::DiffSubNodes(
	const FText& NodeTypeDisplayName,
	const TArray<UFlowGraphNode*>& LhsSubNodes,
	const TArray<UFlowGraphNode*>& RhsSubNodes,
	FDiffResults& Results)
{
	TArray<FGraphDiffControl::FNodeMatch> NodeMatches;
	TSet<const UEdGraphNode*> MatchedRhsNodes;

	FGraphDiffControl::FNodeDiffContext AdditiveDiffContext;
	AdditiveDiffContext.NodeTypeDisplayName = NodeTypeDisplayName;
	AdditiveDiffContext.bIsRootNode = false;

	// march through the all the nodes in the rhs and look for matches 
	for (UEdGraphNode* RhsSubNode : RhsSubNodes)
	{
		FGraphDiffControl::FNodeMatch NodeMatch;
		NodeMatch.NewNode = RhsSubNode;

		// Do two passes, exact and soft
		for (UEdGraphNode* LhsSubNode : LhsSubNodes)
		{
			if (FGraphDiffControl::IsNodeMatch(LhsSubNode, RhsSubNode, true, &NodeMatches))
			{
				NodeMatch.OldNode = LhsSubNode;
				break;
			}
		}

		if (NodeMatch.NewNode == nullptr)
		{
			for (UEdGraphNode* LhsSubNode : LhsSubNodes)
			{
				if (FGraphDiffControl::IsNodeMatch(LhsSubNode, RhsSubNode, false, &NodeMatches))
				{
					NodeMatch.OldNode = LhsSubNode;
					break;
				}
			}
		}

		// if we found a corresponding node in the lhs graph, track it (so we can prevent future matches with the same nodes)
		if (NodeMatch.IsValid())
		{
			NodeMatches.Add(NodeMatch);
			MatchedRhsNodes.Add(NodeMatch.OldNode);
		}

		NodeMatch.Diff(AdditiveDiffContext, Results);
	}

	FGraphDiffControl::FNodeDiffContext SubtractiveDiffContext = AdditiveDiffContext;
	SubtractiveDiffContext.DiffMode = FGraphDiffControl::EDiffMode::Subtractive;
	SubtractiveDiffContext.DiffFlags = FGraphDiffControl::EDiffFlags::NodeExistance;

	// go through the lhs nodes to catch ones that may have been missing from the rhs graph
	for (UEdGraphNode* LhsSubNode : LhsSubNodes)
	{
		// if this node has already been matched, move on
		if (!LhsSubNode || MatchedRhsNodes.Find(LhsSubNode))
		{
			continue;
		}

		// There can't be a matching node in RhsGraph because it would have been found above
		FGraphDiffControl::FNodeMatch NodeMatch;
		NodeMatch.NewNode = LhsSubNode;

		NodeMatch.Diff(SubtractiveDiffContext, Results);
	}
}

void UFlowGraphNode::AddSubNode(UFlowGraphNode* SubNode, class UEdGraph* ParentGraph)
{
	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));
	ParentGraph->Modify();
	Modify();

	SubNode->SetFlags(RF_Transactional);

	// set outer to be the graph so it doesn't go away
	SubNode->Rename(nullptr, ParentGraph, REN_NonTransactional);
	SubNode->SetParentNodeForSubNode(this);

	SubNode->CreateNewGuid();
	SubNode->PostPlacedNewNode();
	SubNode->AllocateDefaultPins();
	SubNode->AutowireNewNode(nullptr);

	SubNode->NodePosX = 0;
	SubNode->NodePosY = 0;

	SubNodes.Add(SubNode);
	OnSubNodeAdded(SubNode);

	ParentGraph->NotifyGraphChanged();
	GetFlowGraph()->UpdateAsset();

	// NOTE - We do not need to RebuildRuntimeAddOnsFromEditorSubNodes here, because UpdateAsset() will do it
}

void UFlowGraphNode::OnSubNodeAdded(UFlowGraphNode* SubNode)
{
	// Empty in base class
}

void UFlowGraphNode::RemoveSubNode(UFlowGraphNode* SubNode)
{
	Modify();
	SubNodes.RemoveSingle(SubNode);

	RebuildRuntimeAddOnsFromEditorSubNodes();

	OnSubNodeRemoved(SubNode);
}

void UFlowGraphNode::RemoveAllSubNodes()
{
	SubNodes.Reset();

	RebuildRuntimeAddOnsFromEditorSubNodes();
}

void UFlowGraphNode::OnSubNodeRemoved(UFlowGraphNode* SubNode)
{
	// Empty in base class
}

int32 UFlowGraphNode::FindSubNodeDropIndex(UFlowGraphNode* SubNode) const
{
	const int32 InsertIndex = SubNodes.IndexOfByKey(SubNode);
	return InsertIndex;
}

void UFlowGraphNode::InsertSubNodeAt(UFlowGraphNode* SubNode, int32 DropIndex)
{
	if (DropIndex > -1)
	{
		SubNodes.Insert(SubNode, DropIndex);
	}
	else
	{
		SubNodes.Add(SubNode);
	}

	RebuildRuntimeAddOnsFromEditorSubNodes();
}

void UFlowGraphNode::DestroyNode()
{
	if (ParentNode)
	{
		ParentNode->RemoveSubNode(this);

		ParentNode->RebuildRuntimeAddOnsFromEditorSubNodes();
	}
	else
	{
		RebuildRuntimeAddOnsFromEditorSubNodes();
	}

	UEdGraphNode::DestroyNode();
}

bool UFlowGraphNode::UsesBlueprint() const
{
	return NodeInstance && NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
}

bool UFlowGraphNode::RefreshNodeClass()
{
	bool bUpdated = false;
	if (NodeInstance == nullptr)
	{
		if (NodeInstanceClass.IsPending())
		{
			NodeInstanceClass.LoadSynchronous();
		}

		if (NodeInstanceClass.IsValid())
		{
			PostPlacedNewNode();

			bUpdated = (NodeInstance != nullptr);
		}
	}

	return bUpdated;
}

void UFlowGraphNode::UpdateNodeClassData()
{
	if (NodeInstance)
	{
		NodeInstanceClass = NodeInstance->GetClass();
	}
}

bool UFlowGraphNode::HasErrors() const
{
	return ErrorMessage.Len() > 0 || !IsValid(NodeInstance);
}

void UFlowGraphNode::ValidateGraphNode(FFlowMessageLog& MessageLog) const
{
	// Verify that all input data pin connections are legal

	bool bAppendedErrors = false;

	if (!NodeInstance)
	{
		// Missing the node instance!

		MessageLog.Error<UFlowNode>(TEXT("FlowGraphNode is missing its UFlowNode instance!"), nullptr);

		return;
	}

	FFlowMessageLog& ValidationLog = NodeInstance->ValidationLog;

	const UFlowGraphSchema* Schema = CastChecked<UFlowGraphSchema>(GetSchema());

	for (const UEdGraphPin* EdGraphPin : InputPins)
	{
		if (!FFlowPin::IsDataPinCategory(EdGraphPin->PinType.PinCategory))
		{
			continue;
		}

		if (!EdGraphPin->HasAnyConnections())
		{
			continue;
		}

		for (UEdGraphPin* const ConnectedPin : EdGraphPin->LinkedTo)
		{
			const FPinConnectionResponse Response = Schema->CanCreateConnection(ConnectedPin, EdGraphPin);
			
			if (!Response.CanSafeConnect())
			{
				MessageLog.Error<UFlowNodeBase>(*FString::Printf(TEXT("Pin %s has invalid connection: %s"), *EdGraphPin->GetName(), *Response.Message.ToString()), NodeInstance);
			}
		}
	}
}

bool UFlowGraphNode::IsAncestorNode(const UFlowGraphNode& OtherNode) const
{
	const UFlowGraphNode* CurParentNode = ParentNode;
	while (CurParentNode)
	{
		if (CurParentNode == &OtherNode)
		{
			return true;
		}

		CurParentNode = CurParentNode->ParentNode;
	}

	return false;
}

bool UFlowGraphNode::CanAcceptSubNodeAsChild(const UFlowGraphNode& SubNodeToConsider, const TSet<const UEdGraphNode*>& AllRootSubNodesToPaste, FString* OutReasonString) const
{
	const UFlowNodeBase* OtherFlowNodeSubNode = SubNodeToConsider.NodeInstance;

	if (!OtherFlowNodeSubNode)
	{
		if (OutReasonString)
		{
			*OutReasonString = TEXT("Editor node is missing a runtime AddOn instance");
		}

		return false;
	}

	if (IsAncestorNode(SubNodeToConsider))
	{
		if (OutReasonString)
		{
			*OutReasonString = TEXT("Cannot be a AddOn of one of our own AddOns");
		}
	
		return false;
	}

	check(OtherFlowNodeSubNode);
	const UFlowNodeAddOn* AddOnToConsider = Cast<UFlowNodeAddOn>(OtherFlowNodeSubNode);

	// Build the array of other root AddOns that will also be added as children as an atomic operation (eg, multi-paste)
	TArray<UFlowNodeAddOn*> OtherAddOnsToPaste;

	for (TSet<const UEdGraphNode*>::TConstIterator It(AllRootSubNodesToPaste); It; ++It)
	{
		const UFlowGraphNode* NodeToPaste = Cast<UFlowGraphNode>(*It);
		UFlowNodeAddOn* AddOnToPaste = Cast<UFlowNodeAddOn>(NodeToPaste->NodeInstance);

		if (IsValid(AddOnToPaste) && AddOnToPaste != AddOnToConsider)
		{
			OtherAddOnsToPaste.Add(AddOnToPaste);
		}
	}

	const UFlowNodeBase* ThisFlowNodeBase = NodeInstance;
	const EFlowAddOnAcceptResult AcceptResult = ThisFlowNodeBase->CheckAcceptFlowNodeAddOnChild(AddOnToConsider, OtherAddOnsToPaste);

	// Undetermined and Reject both count as Rejection, only TentativeAccept is an 'accept' result

	if (AcceptResult == EFlowAddOnAcceptResult::TentativeAccept)
	{
		FLOW_ASSERT_ENUM_MAX(EFlowAddOnAcceptResult, 3);

		return true;
	}

	if (OutReasonString)
	{
		*OutReasonString = FString::Printf(TEXT("%s cannot accept AddOn type %s"), *GetClass()->GetName(), *OtherFlowNodeSubNode->GetClass()->GetName());
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
