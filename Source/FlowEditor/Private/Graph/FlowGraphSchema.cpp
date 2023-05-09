// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraphSchema.h"

#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphEditor.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/FlowGraphSettings.h"
#include "Graph/FlowGraphUtils.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/FlowNodeBlueprint.h"
#include "Nodes/Route/FlowNode_CustomInput.h"
#include "Nodes/Route/FlowNode_Start.h"
#include "Nodes/Route/FlowNode_Reroute.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FlowGraphSchema"

bool UFlowGraphSchema::bInitialGatherPerformed = false;
TArray<UClass*> UFlowGraphSchema::NativeFlowNodes;
TMap<FName, FAssetData> UFlowGraphSchema::BlueprintFlowNodes;
TMap<UClass*, UClass*> UFlowGraphSchema::GraphNodesByFlowNodes;

bool UFlowGraphSchema::bBlueprintCompilationPending;

FFlowGraphSchemaRefresh UFlowGraphSchema::OnNodeListChanged;

UFlowGraphSchema::UFlowGraphSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFlowGraphSchema::SubscribeToAssetChanges()
{
	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistry.Get().OnFilesLoaded().AddStatic(&UFlowGraphSchema::GatherNodes);
	AssetRegistry.Get().OnAssetAdded().AddStatic(&UFlowGraphSchema::OnAssetAdded);
	AssetRegistry.Get().OnAssetRemoved().AddStatic(&UFlowGraphSchema::OnAssetRemoved);

	FCoreUObjectDelegates::ReloadCompleteDelegate.AddStatic(&UFlowGraphSchema::OnHotReload);

	if (GEditor)
	{
		GEditor->OnBlueprintPreCompile().AddStatic(&UFlowGraphSchema::OnBlueprintPreCompile);
		GEditor->OnBlueprintCompiled().AddStatic(&UFlowGraphSchema::OnBlueprintCompiled);
	}
}

void UFlowGraphSchema::GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UClass* AssetClass, const FString& CategoryName)
{
	GetFlowNodeActions(ActionMenuBuilder, AssetClass->GetDefaultObject<UFlowAsset>(), CategoryName);
	GetCommentAction(ActionMenuBuilder);
}

void UFlowGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	GetFlowNodeActions(ContextMenuBuilder, GetAssetClassDefaults(ContextMenuBuilder.CurrentGraph), FString());
	GetCommentAction(ContextMenuBuilder, ContextMenuBuilder.CurrentGraph);

	if (!ContextMenuBuilder.FromPin && FFlowGraphUtils::GetFlowGraphEditor(ContextMenuBuilder.CurrentGraph)->CanPasteNodes())
	{
		const TSharedPtr<FFlowGraphSchemaAction_Paste> NewAction(new FFlowGraphSchemaAction_Paste(FText::GetEmpty(), LOCTEXT("PasteHereAction", "Paste here"), FText::GetEmpty(), 0));
		ContextMenuBuilder.AddAction(NewAction);
	}
}

void UFlowGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	const UFlowAsset* AssetClassDefaults = GetAssetClassDefaults(&Graph);
	static const FVector2D NodeOffsetIncrement = FVector2D(0, 128);
	FVector2D NodeOffset = FVector2D::ZeroVector;

	// Start node
	CreateDefaultNode(Graph, AssetClassDefaults, UFlowNode_Start::StaticClass(), NodeOffset, AssetClassDefaults->bStartNodePlacedAsGhostNode);

	// Add default nodes for all of the CustomInputs
	if (IsValid(AssetClassDefaults))
	{
		for (const FName& CustomInputName : AssetClassDefaults->CustomInputs)
		{
			NodeOffset += NodeOffsetIncrement;
			const UFlowGraphNode* NewFlowGraphNode = CreateDefaultNode(Graph, AssetClassDefaults, UFlowNode_CustomInput::StaticClass(), NodeOffset, true);

			UFlowNode_CustomInput* CustomInputNode = CastChecked<UFlowNode_CustomInput>(NewFlowGraphNode->GetFlowNode());
			CustomInputNode->SetEventName(CustomInputName);
		}
	}

	CastChecked<UFlowGraph>(&Graph)->GetFlowAsset()->HarvestNodeConnections();
}

UFlowGraphNode* UFlowGraphSchema::CreateDefaultNode(UEdGraph& Graph, const UFlowAsset* AssetClassDefaults, const TSubclassOf<UFlowNode>& NodeClass, const FVector2D& Offset, const bool bPlacedAsGhostNode)
{
	UFlowGraphNode* NewGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(&Graph, nullptr, NodeClass, Offset);
	SetNodeMetaData(NewGraphNode, FNodeMetadata::DefaultGraphNode);

	if (bPlacedAsGhostNode)
	{
		NewGraphNode->MakeAutomaticallyPlacedGhostNode();
	}

	return NewGraphNode;
}

const FPinConnectionResponse UFlowGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	const UFlowGraphNode* OwningNodeA = Cast<UFlowGraphNode>(PinA->GetOwningNodeUnchecked());
	const UFlowGraphNode* OwningNodeB = Cast<UFlowGraphNode>(PinB->GetOwningNodeUnchecked());

	if (!OwningNodeA || !OwningNodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid nodes"));
	}

	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are on the same node"));
	}

	if (PinA->bOrphanedPin || PinB->bOrphanedPin)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot make new connections to orphaned pin"));
	}

	// Compare the directions
	const UEdGraphPin* InputPin = nullptr;
	const UEdGraphPin* OutputPin = nullptr;

	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Directions are not compatible"));
	}

	// Break existing connections on outputs only - multiple input connections are acceptable
	if (OutputPin->LinkedTo.Num() > 0)
	{
		const ECanCreateConnectionResponse ReplyBreakInputs = (OutputPin == PinA ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B);
		return FPinConnectionResponse(ReplyBreakInputs, TEXT("Replace existing connections"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

bool UFlowGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	const bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);

	if (bModified)
	{
		PinA->GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}

	return bModified;
}

bool UFlowGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	return true;
}

FLinearColor UFlowGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor::White;
}

FText UFlowGraphSchema::GetPinDisplayName(const UEdGraphPin* Pin) const
{
	FText ResultPinName;
	check(Pin != nullptr);
	if (Pin->PinFriendlyName.IsEmpty())
	{
		// We don't want to display "None" for no name
		if (Pin->PinName.IsNone())
		{
			return FText::GetEmpty();
		}
		if (GetDefault<UFlowGraphEditorSettings>()->bEnforceFriendlyPinNames) // this option is only difference between this override and UEdGraphSchema::GetPinDisplayName
		{
			ResultPinName = FText::FromString(FName::NameToDisplayString(Pin->PinName.ToString(), true));
		}
		else
		{
			ResultPinName = FText::FromName(Pin->PinName);
		}
	}
	else
	{
		ResultPinName = Pin->PinFriendlyName;

		bool bShouldUseLocalizedNodeAndPinNames = false;
		GConfig->GetBool(TEXT("Internationalization"), TEXT("ShouldUseLocalizedNodeAndPinNames"), bShouldUseLocalizedNodeAndPinNames, GEditorSettingsIni);
		if (!bShouldUseLocalizedNodeAndPinNames)
		{
			ResultPinName = FText::FromString(ResultPinName.BuildSourceString());
		}
	}
	return ResultPinName;
}

void UFlowGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);

	TargetNode.GetGraph()->NotifyGraphChanged();
}

void UFlowGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(LOCTEXT("GraphEd_BreakPinLinks", "Break Pin Links"));

	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);

	if (TargetPin.bOrphanedPin)
	{
		// this calls NotifyGraphChanged()
		Cast<UFlowGraphNode>(TargetPin.GetOwningNode())->RemoveOrphanedPin(&TargetPin);
	}
	else if (bSendsNodeNotification)
	{
		TargetPin.GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}
}

int32 UFlowGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	return FFlowGraphUtils::GetFlowGraphEditor(Graph)->GetNumberOfSelectedNodes();
}

TSharedPtr<FEdGraphSchemaAction> UFlowGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FFlowGraphSchemaAction_NewComment));
}

void UFlowGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const
{
	const FScopedTransaction Transaction(LOCTEXT("CreateFlowRerouteNodeOnWire", "Create Flow Reroute Node"));

	const FVector2D NodeSpacerSize(42.0f, 24.0f);
	const FVector2D KnotTopLeft = GraphPosition - (NodeSpacerSize * 0.5f);

	UEdGraph* ParentGraph = PinA->GetOwningNode()->GetGraph();
	UFlowGraphNode* NewReroute = FFlowGraphSchemaAction_NewNode::CreateNode(ParentGraph, nullptr, UFlowNode_Reroute::StaticClass(), KnotTopLeft, false);

	PinA->BreakLinkTo(PinB);
	PinA->MakeLinkTo((PinA->Direction == EGPD_Output) ? NewReroute->InputPins[0] : NewReroute->OutputPins[0]);
	PinB->MakeLinkTo((PinB->Direction == EGPD_Output) ? NewReroute->InputPins[0] : NewReroute->OutputPins[0]);
}

TArray<TSharedPtr<FString>> UFlowGraphSchema::GetFlowNodeCategories()
{
	if (!bInitialGatherPerformed)
	{
		GatherNodes();
	}

	TSet<FString> UnsortedCategories;
	for (const UClass* FlowNodeClass : NativeFlowNodes)
	{
		if (const UFlowNode* DefaultObject = FlowNodeClass->GetDefaultObject<UFlowNode>())
		{
			UnsortedCategories.Emplace(DefaultObject->GetNodeCategory());
		}
	}

	for (const TPair<FName, FAssetData>& AssetData : BlueprintFlowNodes)
	{
		if (const UBlueprint* Blueprint = GetPlaceableNodeBlueprint(AssetData.Value))
		{
			UnsortedCategories.Emplace(Blueprint->BlueprintCategory);
		}
	}

	TArray<FString> SortedCategories = UnsortedCategories.Array();
	SortedCategories.Sort();

	// create list of categories
	TArray<TSharedPtr<FString>> Result;
	for (const FString& Category : SortedCategories)
	{
		if (!Category.IsEmpty())
		{
			Result.Emplace(MakeShareable(new FString(Category)));
		}
	}

	return Result;
}

UClass* UFlowGraphSchema::GetAssignedGraphNodeClass(const UClass* FlowNodeClass)
{
	TArray<UClass*> FoundParentClasses;
	UClass* ReturnClass = nullptr;

	// Collect all possible parents and their corresponding GraphNodeClasses
	for (const TPair<UClass*, UClass*>& GraphNodeByFlowNode : GraphNodesByFlowNodes)
	{
		if (FlowNodeClass == GraphNodeByFlowNode.Key)
		{
			return GraphNodeByFlowNode.Value;
		}

		if (FlowNodeClass->IsChildOf(GraphNodeByFlowNode.Key))
		{
			FoundParentClasses.Add(GraphNodeByFlowNode.Key);
		}
	}

	// Of only one parent found set the return to its GraphNodeClass
	if (FoundParentClasses.Num() == 1)
	{
		ReturnClass = GraphNodesByFlowNodes.FindRef(FoundParentClasses[0]);
	}
	// If multiple parents found, find the closest one and set the return to its GraphNodeClass
	else if (FoundParentClasses.Num() > 1)
	{
		TPair<int32, UClass*> ClosestParentMatch = {1000, nullptr};
		for (const auto& ParentClass : FoundParentClasses)
		{
			int32 StepsTillExactMatch = 0;
			const UClass* LocalParentClass = FlowNodeClass;

			while (IsValid(LocalParentClass) && LocalParentClass != ParentClass && LocalParentClass != UFlowNode::StaticClass())
			{
				StepsTillExactMatch++;
				LocalParentClass = LocalParentClass->GetSuperClass();
			}

			if (StepsTillExactMatch != 0 && StepsTillExactMatch < ClosestParentMatch.Key)
			{
				ClosestParentMatch = {StepsTillExactMatch, ParentClass};
			}
		}

		ReturnClass = GraphNodesByFlowNodes.FindRef(ClosestParentMatch.Value);
	}

	return IsValid(ReturnClass) ? ReturnClass : UFlowGraphNode::StaticClass();
}

void UFlowGraphSchema::ApplyNodeFilter(const UFlowAsset* AssetClassDefaults, const UClass* FlowNodeClass, TArray<UFlowNode*>& FilteredNodes)
{
	if (FlowNodeClass == nullptr)
	{
		return;
	}

	UFlowNode* NodeDefaults = FlowNodeClass->GetDefaultObject<UFlowNode>();

	// UFlowNode class limits which UFlowAsset class can use it
	{
		for (const UClass* DeniedAssetClass : NodeDefaults->DeniedAssetClasses)
		{
			if (DeniedAssetClass && AssetClassDefaults->GetClass()->IsChildOf(DeniedAssetClass))
			{
				return;
			}
		}

		if (NodeDefaults->AllowedAssetClasses.Num() > 0)
		{
			bool bAllowedInAsset = false;
			for (const UClass* AllowedAssetClass : NodeDefaults->AllowedAssetClasses)
			{
				if (AllowedAssetClass && AssetClassDefaults->GetClass()->IsChildOf(AllowedAssetClass))
				{
					bAllowedInAsset = true;
					break;
				}
			}
			if (!bAllowedInAsset)
			{
				return;
			}
		}
	}

	// UFlowAsset class can limit which UFlowNode classes can be used
	{
		for (const UClass* DeniedNodeClass : AssetClassDefaults->DeniedNodeClasses)
		{
			if (DeniedNodeClass && FlowNodeClass->IsChildOf(DeniedNodeClass))
			{
				return;
			}
		}

		if (AssetClassDefaults->AllowedNodeClasses.Num() > 0)
		{
			bool bAllowedInAsset = false;
			for (const UClass* AllowedNodeClass : AssetClassDefaults->AllowedNodeClasses)
			{
				if (AllowedNodeClass && FlowNodeClass->IsChildOf(AllowedNodeClass))
				{
					bAllowedInAsset = true;
					break;
				}
			}
			if (!bAllowedInAsset)
			{
				return;
			}
		}
	}

	FilteredNodes.Emplace(NodeDefaults);
}

void UFlowGraphSchema::GetFlowNodeActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UFlowAsset* AssetClassDefaults, const FString& CategoryName)
{
	if (!bInitialGatherPerformed)
	{
		GatherNodes();
	}

	// Flow Asset type might limit which nodes are placeable 
	TArray<UFlowNode*> FilteredNodes;
	{
		FilteredNodes.Reserve(NativeFlowNodes.Num() + BlueprintFlowNodes.Num());

		for (const UClass* FlowNodeClass : NativeFlowNodes)
		{
			ApplyNodeFilter(AssetClassDefaults, FlowNodeClass, FilteredNodes);
		}

		for (const TPair<FName, FAssetData>& AssetData : BlueprintFlowNodes)
		{
			if (const UBlueprint* Blueprint = GetPlaceableNodeBlueprint(AssetData.Value))
			{
				ApplyNodeFilter(AssetClassDefaults, Blueprint->GeneratedClass, FilteredNodes);
			}
		}

		FilteredNodes.Shrink();
	}

	for (const UFlowNode* FlowNode : FilteredNodes)
	{
		if ((CategoryName.IsEmpty() || CategoryName.Equals(FlowNode->GetNodeCategory())) && !UFlowGraphSettings::Get()->NodesHiddenFromPalette.Contains(FlowNode->GetClass()))
		{
			TSharedPtr<FFlowGraphSchemaAction_NewNode> NewNodeAction(new FFlowGraphSchemaAction_NewNode(FlowNode));
			ActionMenuBuilder.AddAction(NewNodeAction);
		}
	}
}

void UFlowGraphSchema::GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph /*= nullptr*/)
{
	if (!ActionMenuBuilder.FromPin)
	{
		const bool bIsManyNodesSelected = CurrentGraph ? (FFlowGraphUtils::GetFlowGraphEditor(CurrentGraph)->GetNumberOfSelectedNodes() > 0) : false;
		const FText MenuDescription = bIsManyNodesSelected ? LOCTEXT("CreateCommentAction", "Create Comment from Selection") : LOCTEXT("AddCommentAction", "Add Comment...");
		const FText ToolTip = LOCTEXT("CreateCommentToolTip", "Creates a comment.");

		const TSharedPtr<FFlowGraphSchemaAction_NewComment> NewAction(new FFlowGraphSchemaAction_NewComment(FText::GetEmpty(), MenuDescription, ToolTip, 0));
		ActionMenuBuilder.AddAction(NewAction);
	}
}

bool UFlowGraphSchema::IsFlowNodePlaceable(const UClass* Class)
{
	if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_NotPlaceable | CLASS_Deprecated))
	{
		return false;
	}

	if (const UFlowNode* DefaultObject = Class->GetDefaultObject<UFlowNode>())
	{
		return !DefaultObject->bNodeDeprecated;
	}

	return true;
}

void UFlowGraphSchema::OnBlueprintPreCompile(UBlueprint* Blueprint)
{
	if (Blueprint && Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(UFlowNode::StaticClass()))
	{
		bBlueprintCompilationPending = true;
	}
}

void UFlowGraphSchema::OnBlueprintCompiled()
{
	if (bBlueprintCompilationPending)
	{
		GatherNodes();
	}

	bBlueprintCompilationPending = false;
}

void UFlowGraphSchema::OnHotReload(EReloadCompleteReason ReloadCompleteReason)
{
	GatherNodes();
}

void UFlowGraphSchema::GatherNativeNodes()
{
	// collect C++ nodes once per editor session
	if (NativeFlowNodes.Num() > 0)
	{
		return;
	}

	TArray<UClass*> FlowNodes;
	GetDerivedClasses(UFlowNode::StaticClass(), FlowNodes);
	for (UClass* Class : FlowNodes)
	{
		if (Class->ClassGeneratedBy == nullptr && IsFlowNodePlaceable(Class))
		{
			NativeFlowNodes.Emplace(Class);
		}
	}

	TArray<UClass*> GraphNodes;
	GetDerivedClasses(UFlowGraphNode::StaticClass(), GraphNodes);
	for (UClass* GraphNodeClass : GraphNodes)
	{
		const UFlowGraphNode* GraphNodeCDO = GraphNodeClass->GetDefaultObject<UFlowGraphNode>();
		for (UClass* AssignedClass : GraphNodeCDO->AssignedNodeClasses)
		{
			if (AssignedClass->IsChildOf(UFlowNode::StaticClass()))
			{
				GraphNodesByFlowNodes.Emplace(AssignedClass, GraphNodeClass);
			}
		}
	}
}

void UFlowGraphSchema::GatherNodes()
{
	// prevent asset crunching during PIE
	if (GEditor && GEditor->PlayWorld)
	{
		return;
	}

	bInitialGatherPerformed = true;

	GatherNativeNodes();

	// retrieve all blueprint nodes
	FARFilter Filter;
	Filter.ClassPaths.Add(UFlowNodeBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> FoundAssets;
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistryModule.Get().GetAssets(Filter, FoundAssets);
	for (const FAssetData& AssetData : FoundAssets)
	{
		AddAsset(AssetData, true);
	}

	OnNodeListChanged.Broadcast();
}

void UFlowGraphSchema::OnAssetAdded(const FAssetData& AssetData)
{
	AddAsset(AssetData, false);
}

void UFlowGraphSchema::AddAsset(const FAssetData& AssetData, const bool bBatch)
{
	if (!BlueprintFlowNodes.Contains(AssetData.PackageName))
	{
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
		if (AssetRegistryModule.Get().IsLoadingAssets())
		{
			return;
		}

		if (AssetData.GetClass()->IsChildOf(UFlowNodeBlueprint::StaticClass()))
		{
			BlueprintFlowNodes.Emplace(AssetData.PackageName, AssetData);

			if (!bBatch)
			{
				OnNodeListChanged.Broadcast();
			}
		}
	}
}

void UFlowGraphSchema::OnAssetRemoved(const FAssetData& AssetData)
{
	if (BlueprintFlowNodes.Contains(AssetData.PackageName))
	{
		BlueprintFlowNodes.Remove(AssetData.PackageName);
		BlueprintFlowNodes.Shrink();

		OnNodeListChanged.Broadcast();
	}
}

UBlueprint* UFlowGraphSchema::GetPlaceableNodeBlueprint(const FAssetData& AssetData)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
	if (Blueprint && IsFlowNodePlaceable(Blueprint->GeneratedClass))
	{
		return Blueprint;
	}

	return nullptr;
}

const UFlowAsset* UFlowGraphSchema::GetAssetClassDefaults(const UEdGraph* Graph)
{
	const UClass* AssetClass = UFlowAsset::StaticClass();

	if (Graph)
	{
		if (const UFlowAsset* FlowAsset = Graph->GetTypedOuter<UFlowAsset>())
		{
			AssetClass = FlowAsset->GetClass();
		}
	}

	return AssetClass->GetDefaultObject<UFlowAsset>();
}

#undef LOCTEXT_NAMESPACE
